/*
This code handles the sniffing of bpp packets using the ESP32 in WiFi sniffer mode. No bytes are
transmitted over WiFi anywhere here; this is a purely passive action.

It contains both the logic to 'peel away' all the layers of the network protocol in order to get
to the UDP payload of a bpp-carrying UDP packet, and will feed that into the first step (sign checking)
of the bpp protocol stack. It also contains a WiFi monitoring thread. The idea of that is that the 
code will initially not know anything about the network or SSIDs or whatever. It will start scanning
the ether for any channel carrying a bpp-packet that is valid (=is signed with the private key belonging
to the public key we know). It'll make a note of the BSSID, resolve that to a SSID, and from then
on try to look for that SSID. If the receive quality deteriorates, it will try to find a different 
channel to connect to a possibly different AP.

This code kinda is a mess, because we have the WiFi sniffer callback do work for the monitor thread,
and this is communicated using atomic variables... it could all be structured better. But hey, the workings
of the monitor thread are easily adjusted now, that's at least something.

ToDo: Get rid of a gazillion behaviour-influencing magic numbers and turn them into #defines or something.
*/
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "hexdump.h"


#include "structs.h"
#include "chksign.h"
#include "defec.h"
#include "serdec.h"
#include "hldemux.h"

#include "subtitle.h"
#include "blockdecode.h"
#include "bd_emu.h"
#include "bd_flatflash.h"
#include "hkpackets.h"

#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "nvs.h"
#include "powerdown.h"

#include "freertos/ringbuf.h"
#include "bpp_udp.h"

//#define BADGE_BPP_SERVER "badge-bpp.sha2017.org"
#define BADGE_BPP_SERVER "badge-bpp.j0h.nl"
//#define BADGE_BPP_SERVER "10.23.45.55"

typedef struct {
	uint8_t mac[6];
} __attribute__((packed)) MacAddr;

typedef struct {
	int16_t fctl;
	int16_t duration;
	MacAddr addr1;
	MacAddr addr2;
	MacAddr addr3;
	int16_t seqctl;
	MacAddr addr4;
	unsigned char payload[];
} __attribute__((packed)) WifiHdr;


typedef struct {
	int16_t fctl;
	int16_t duration;
	MacAddr da;
	MacAddr sa;
	MacAddr bssid;
	int16_t seqctl;
	unsigned char payload[];
} __attribute__((packed)) WifiMgmtHdr;


typedef struct {
	int64_t timestamp;
	int16_t interval;
	int16_t capability;
	uint8_t elementdata[];
} __attribute__((packed)) WifiBeaconHdr;


typedef struct {
	uint8_t dsap;
	uint8_t ssap;
	uint8_t control1;
	uint8_t control2;
	unsigned char payload[];
} __attribute__((packed)) LlcHdr;

typedef struct {
	uint8_t oui[3];
	uint16_t proto;
	unsigned char payload[];
} __attribute__((packed)) LlcHdrSnap;

typedef struct {
	MacAddr src;
	MacAddr dst;
	uint16_t len;
	unsigned char payload[];
} __attribute__((packed)) EthHdr;

typedef struct {
	uint8_t verihl;
	uint8_t tos;
	uint16_t len;
	uint16_t id;
	uint16_t flag;
	uint8_t ttl;
	uint8_t proto;
	uint16_t hdrcsum;
	uint32_t src;
	uint32_t dst;
	unsigned char payload[];
} __attribute__((packed)) IpHdr;


typedef struct {
	uint16_t srcPort;
	uint16_t dstPort;
	uint16_t len;
	uint16_t chs;
	unsigned char payload[];
} __attribute__((packed)) UdpHdr;



#define WORK_IDLE 0
#define WORK_CAPT_BSSID 1
#define WORK_BSSID_TO_ESSID 2
#define WORK_GET_HIGHEST_RSSI 3
static volatile int needWork=WORK_IDLE; //We use this as an atomic, so no mux needed.
static MacAddr validBssId;
static char ssidForBssid[32];
static volatile int highestRssi;
static volatile int highestRssiChan;

typedef struct {
	MacAddr bssid;
	uint8_t unused[2];
	uint8_t data[]; //try to keep this 32-bit aligned
} RecvedWifiPacket;

typedef struct {
	int channel;
	char ssid[33];
} WifiSavedStatus;

//Kept over deep sleep periods
static RTC_DATA_ATTR  WifiSavedStatus wifiSavedStatus;

#if 0
void printmac(MacAddr *mac) {
	int x;
	for (x=0; x<6; x++) printf("%02X%s", mac->mac[x], x!=5?":":"");
	printf(" ");
}

void printip(uint32_t ip) {
	uint8_t *p=(uint8_t*)&ip;
	printf("%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
}

#endif

RingbufHandle_t *packetRingbuf;

/*
This is the monitor callback. If it receives a beacon packet and needs to do something for the monitor
thread, it will. If it receives a data packet, it'll see if it looks like an UDP packet to port 2017 and
if so, pass the UDP payload (plus some meta-info) on to the parser thread using a ring buffer.
*/
static void sniffcb(void *buf, wifi_promiscuous_pkt_type_t type) {
//	printf("Sniffed: type %d\n", type);
	if (type==WIFI_PKT_MGMT && (needWork==WORK_BSSID_TO_ESSID || needWork==WORK_GET_HIGHEST_RSSI)) {
		wifi_promiscuous_pkt_t *p=(wifi_promiscuous_pkt_t*)buf;
		int len=p->rx_ctrl.sig_len;
		WifiMgmtHdr *wh=(WifiMgmtHdr*)p->payload;
		len-=sizeof(WifiMgmtHdr);
		if (len<0) return;
		int fctl=ntohs(wh->fctl);
		if ((fctl&0xFF00) == 0x8000) { //beacon
			WifiBeaconHdr *bcn=(WifiBeaconHdr*)wh->payload;
			len-=sizeof(WifiBeaconHdr);
			uint8_t *ep=bcn->elementdata;
			while (len>0) {
				if (ep[0]==0 && ep[1]<=32) { //SSID element
					if (needWork==WORK_BSSID_TO_ESSID && memcmp(&wh->bssid, &validBssId, 6)==0) {
						for (int i=0; i<ep[1]; i++) ssidForBssid[i]=ep[2+i];
						ssidForBssid[ep[1]]=0;
						needWork=WORK_IDLE;
					} else if (needWork==WORK_GET_HIGHEST_RSSI) {
						if (strlen(ssidForBssid)==ep[1] && memcmp(ssidForBssid, &ep[2], ep[1])==0) {
							if (p->rx_ctrl.rssi > highestRssi) {
								highestRssi=p->rx_ctrl.rssi;
								highestRssiChan=p->rx_ctrl.channel;
							}
						}
					}
				}
				//Next element plz
				len-=ep[1]+2;
				ep+=ep[1]+2;
			}
		}
	}
	if (type==WIFI_PKT_DATA) {
		//The buffer consists of many layers of headers. Peel this onion until we've
		//arrived at the juicy UDP inner load.
		wifi_promiscuous_pkt_t *p=(wifi_promiscuous_pkt_t*)buf;
		int len=p->rx_ctrl.sig_len;
		WifiHdr *wh=(WifiHdr*)p->payload;
		len-=sizeof(WifiHdr);
		if (len<0) return;
		int fctl=ntohs(wh->fctl);
		if (fctl&0x0040) return; //Encrypted, can't handle this.
		if ((fctl&0xF00)!=0x800) return; //we only want data packets
		LlcHdr *lch=(LlcHdr*)wh->payload;
		uint8_t *pl=(uint8_t*)lch->payload;
		if ((lch->control1&3)==3) {
			pl--; //only has 8-bit control header; payload starts earlier
			len-=sizeof(LlcHdr)-1;
		} else {
			len-=sizeof(LlcHdr);
		}
		if (len<0) return;
		IpHdr *iph;
		if (lch->dsap==0xAA) {
			//Also has SNAP data
			LlcHdrSnap *lchs=(LlcHdrSnap*)pl;
			len-=sizeof(LlcHdrSnap);
			if (len<0) return;
			iph=(IpHdr*)lchs->payload;
		} else {
			iph=(IpHdr*)pl;
		}


		len-=sizeof(IpHdr);
		if (len<0) return;

		if ((iph->verihl>>4)!=4) return; //discard non-ipv4 packets
		int ip_ihl=(iph->verihl&0xf);
		if (ip_ihl<5) return; //invalid

		len-=(ip_ihl-5)*4;
		if (len<0) return;
		if (iph->proto != 17) return; // discard non-UDP packets

//		hexdump(&iph->payload[(ip_ihl-5)*4], len-0x20);
		if (len<sizeof(UdpHdr)) return;
		UdpHdr *uh=(UdpHdr*)&iph->payload[(ip_ihl-5)*4];
		if (ntohs(uh->len) < len-4) return; //-4 because WiFi packets have 4-byte CRC appended
		int udppllen=ntohs(uh->len)-sizeof(UdpHdr);
#if 0
		printf("Rem len %d udp len %d ", len-4, ntohs(uh->len));
		printf("Packet ");
		printip(iph->src);
		printf(":%d -> ", ntohs(uh->srcPort));
		printip(iph->dst);
		printf(":%d\n", ntohs(uh->dstPort));
#endif

		if (ntohs(uh->dstPort)!=2017) return;
		
		//Make this into a RecvedWifiPacket, with payload being the UDP payload. We overwrite the
		//lower bits with the RecvedWifiPacket header.
		RecvedWifiPacket *op=(RecvedWifiPacket*)(uh->payload-sizeof(RecvedWifiPacket));
		if ((fctl&0x3)==0) {
			memcpy(&op->bssid, &wh->addr3, sizeof(MacAddr));
		} else if ((fctl&0x3)==2) {
			memcpy(&op->bssid, &wh->addr2, sizeof(MacAddr));
		} else if ((fctl&0x3)==1) {
			memcpy(&op->bssid, &wh->addr1, sizeof(MacAddr));
		} else {
			memset(&op->bssid, 0, sizeof(MacAddr));
		}
		xRingbufferSend(packetRingbuf, op, udppllen+sizeof(RecvedWifiPacket), 0);
	}
}


extern BlockDecodeHandle *otablockdecoder;
extern BlockDecodeHandle *ropartblockdecoder;

//Parser task. This receives the packets from the WiFi sniffer and uses the bpp stack to parse them.
//Any bpp callbacks/operations run in the context of this stack.
static void parseTask(void *arg) {
	int t=0;
	printf("ParseTask started.\n");
	while(1) {
		size_t len;
		RecvedWifiPacket *p=(RecvedWifiPacket*)xRingbufferReceive(packetRingbuf, &len, portMAX_DELAY);
		if (len==1) {
			//Meta-packet to free up ringbuffer and exit thread.
			vRingbufferReturnItem(packetRingbuf, p);
			vRingbufferDelete(packetRingbuf);
			vTaskDelete(NULL);
		}
		int success=chksignRecv(&p->data[0], len-sizeof(RecvedWifiPacket));
		if (success && needWork==WORK_CAPT_BSSID) {
			//Got a bssid that seems to send out valid badge packets. Mark the bssid.
			memcpy(&validBssId, &p->bssid, sizeof(MacAddr));
			needWork=WORK_IDLE;
		}
		vRingbufferReturnItem(packetRingbuf, p);
		t++;
		if ((t&127)==0) blockdecodeStatus(otablockdecoder);
		if ((t&127)==63) blockdecodeStatus(ropartblockdecoder);
	}
}


/*
This will scan all channels and see if there's a valid (signed with the key we compiled in)
bpp packet incoming somewhere. As soon as we have found it, we grab the BSSID and then try
to grab a beacon packet in order to convert it to a SSID.

Ssd is stored in ssidForBssid as a zero-terminated string.
*/
static bool wifiMonLookForBppPacket() {
	int tout;
	printf("wifiMonTask: Scanning for valid bpp packets to figure out bssid...\n");
	needWork=WORK_CAPT_BSSID;
	int chan=1;
	tout=12*3;
	while (needWork!=WORK_IDLE && --tout) {
		chan++;
		if (chan==14) chan=1;
		ESP_ERROR_CHECK(esp_wifi_set_channel(chan,WIFI_SECOND_CHAN_NONE));
		printf("wifiMonTask: Scanning chan %d\n", chan);
		vTaskDelay(1000/portTICK_RATE_MS);
	}
	if (!tout) {
		printf("wifiMonTask: failed to find bpp packets!\n");
		return false;
	}
	printf("wifiMonTask: Have found bpp packets on chan %d.\n", chan);
	needWork=WORK_BSSID_TO_ESSID;
	tout=20;
	while (needWork!=WORK_IDLE && --tout) {
		vTaskDelay(100/portTICK_RATE_MS);
	}
	if (tout==0) {
		printf("wifiMonTask: failed to find SSID!\n");
		return false;
	} else {
		printf("wifiMonTask: SSID sending out packets is %s\n", ssidForBssid);
		memcpy(wifiSavedStatus.ssid, ssidForBssid, 33);
		return true;
	}
}

/*
Will scan all channels for beacon packets for the SSID saved in ssidForBssid. Will return the
channel with the packet with the highest RSSI in highestRssiChan. Will return false when
no beacon with that SSID is received.
*/
static bool wifiMonGetHighestRssi() {
	highestRssi=-255;
	highestRssiChan=-1;
	needWork=WORK_GET_HIGHEST_RSSI;
	for (int chan=1; chan<14; chan++) {
		ESP_ERROR_CHECK(esp_wifi_set_channel(chan,WIFI_SECOND_CHAN_NONE));
		vTaskDelay(200/portTICK_RATE_MS);
	}
	return highestRssiChan!=-1;
}

static void wifiMonTask(void *arg) {
	FecStatus fecSt, fecStNw;
	int timeToCheck;
	int gotZeroPackets=0;
#if 0
	//hack: Immediate active UDP mode on fixed SSID for testing
	vTaskDelay(100); 
	esp_wifi_set_promiscuous(0);
	vTaskDelay(20); //give everything time to shut down
	xRingbufferSend(packetRingbuf, "q", 1, 0); //tell parser task to clean itself up
	bppConnectUsingUdp("badge", BADGE_BPP_SERVER, 2017, (int)wifiMonTask);
#endif


	//Make sure to not shutdown directly.
	powerHold((int)wifiMonTask, 10*1000);
	//Okay, where were we? Do we already have a bssid?
	if (wifiSavedStatus.ssid[0]==0) {
		//Nope. Go look for it.
		bool ret=wifiMonLookForBppPacket();
		if (!ret) {
			//Can't be helped. Try again in 5 minutes.
			powerCanSleepFor((int)wifiMonTask, 5*60*1000);
			vTaskDelete(NULL);
		}
		strncpy(wifiSavedStatus.ssid, ssidForBssid, 33);
		wifiSavedStatus.channel=0;
	} else {
		printf("wifiMonTask: Connecting to saved ssid %s\n", wifiSavedStatus.ssid);
	}
	//We now have the ssid to connect to in wifiSavedStatus.ssid.
	//Do we already have a channel we had good results with?
	if (wifiSavedStatus.channel!=0) {
		//Yes. Go there and see if we get anything
		printf("wifiMonTask: Re-using last known good channel %d\n", wifiSavedStatus.channel);
		ESP_ERROR_CHECK(esp_wifi_set_channel(wifiSavedStatus.channel,WIFI_SECOND_CHAN_NONE));
		vTaskDelay(4000/portTICK_RATE_MS);
		defecGetStatus(&fecSt);
		if (fecSt.packetsInTotal>0) {
			//Yep, we're receiving packets here.
			timeToCheck=20;
			powerCanSleep((int)wifiMonTask);
		} else {
			//No packets here. Go scan.
			timeToCheck=0;
		}
	} else {
		printf("wifiMonTask: No last good channel for %s!\n", ssidForBssid);
		timeToCheck=0;
		powerCanSleep((int)wifiMonTask);
	}

	while(1) {
		printf("wifiMonTask: Sleeping for %d seconds.\n", (int)timeToCheck);
		if (timeToCheck) vTaskDelay((timeToCheck*1000)/portTICK_RATE_MS);
		//Figure out what happened in the time we slept
		defecGetStatus(&fecStNw);
		fecStNw.packetsInTotal-=fecSt.packetsInTotal;
		fecStNw.packetsInMissed-=fecSt.packetsInMissed;
		defecGetStatus(&fecSt);
		
		if (fecStNw.packetsInTotal==0) gotZeroPackets++; else gotZeroPackets=0;
		if (gotZeroPackets>=2) {
			//Hmm. Connected to a good AP, but don't get any packets. Maybe AP doesn't have any clients and as such doesn't send bpp stuff.
			//Try to fix that by being a client for a while. connectUsingUdp should only return when an error happened.
			printf("wifiMonTask: Attempting active connection.\n");
			esp_wifi_set_promiscuous(0);
			vTaskDelay(20); //give everything time to shut down
			xRingbufferSend(packetRingbuf, "q", 1, 0); //tell parser task to clean itself up
			bppConnectUsingUdp(ssidForBssid,BADGE_BPP_SERVER, 2017, (int)wifiMonTask);
			//I got nuthin'. Sleep for a long time and retry.
			printf("wifiMonTask: Active connection failed. I give up. Sleeping.\n");
			powerCanSleepFor((int)wifiMonTask, 10*60*1000);
			vTaskDelete(NULL);
		}
		int missedPct=fecStNw.packetsInTotal?(fecStNw.packetsInMissed*100)/fecStNw.packetsInTotal:0;
		printf("wifiMonTask: Of the last %d packets, %d (%d pct) were not received.\n", fecStNw.packetsInTotal, fecStNw.packetsInMissed, missedPct);
		if (fecStNw.packetsInTotal<2 || missedPct>35) {
			//Need to re-scan to see if we can find a better AP.
			printf("wifiMonTask: Re-scanning for AP...\n");
			bool ret=wifiMonGetHighestRssi();
			if (ret) {
				printf("wifiMonTask: Found highest rssi %d at chan %d.\n", highestRssi, highestRssiChan);
				ESP_ERROR_CHECK(esp_wifi_set_channel(highestRssiChan,WIFI_SECOND_CHAN_NONE));
				wifiSavedStatus.channel=highestRssiChan;
				timeToCheck=20;
			} else {
				//Errm... no SSID found. Now what?
				if (fecStNw.packetsInTotal && missedPct<30) {
					//Guess we can stay at the current AP, reception here is okay... ish.
					timeToCheck=20;
				} else {
					//Current channel is also worthless, and we have no new option.
					//Can't be helped. Go to sleep and try again in 5 minutes.
					wifiSavedStatus.channel=0; //scan again directly after wakeup
					powerCanSleepFor((int)wifiMonTask, 5*60*1000);
					vTaskDelete(NULL);
				}
			}
		}
		powerCanSleep((int)wifiMonTask);
	}
}


void bppWifiSnifferStart() {
	packetRingbuf=xRingbufferCreate(12*1024, RINGBUF_TYPE_NOSPLIT);
	const wifi_promiscuous_filter_t filt={
		.filter_mask=WIFI_PROMIS_FILTER_MASK_MGMT|WIFI_PROMIS_FILTER_MASK_DATA
	};
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_rx_cb(sniffcb));
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous(1));
	ESP_ERROR_CHECK(esp_wifi_set_promiscuous_filter(&filt));
	ESP_ERROR_CHECK(esp_wifi_set_channel(1,WIFI_SECOND_CHAN_NONE));
	
	xTaskCreatePinnedToCore(parseTask, "bppparse", 8192, NULL, 3, NULL, 1);
	xTaskCreatePinnedToCore(wifiMonTask, "bppwifimon", 8192, NULL, 3, NULL, 1);
}


#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"

#include "chksign.h"
#include "powerdown.h"
#include "subtitle.h"
#include "blockdecode.h"
#include "bd_emu.h"
#include "bd_flatflash.h"
#include "hkpackets.h"


#if 0
static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
	switch(event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently
		   auto-reassociate. */
		esp_wifi_connect();
		xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
		break;
	default:
		break;
	}
	return ESP_OK;
}
#endif

static struct sockaddr_in serveraddr;

static int connectBppServer(const char *servername, int port) {
	int sockfd;
	int serverlen=0;
	struct hostent *server;

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		printf("connectBppServer: can't create socket\n");
		return -1;
	}

	/* gethostbyname: get the server's DNS entry */
	server = gethostbyname(servername);
	if (server == NULL) {
		printf("connectBppServer: error looking up %s\n", servername);
		return -1;
	}

	/* build the server's Internet address */
	bzero((char *) &serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr, server->h_length);
	serveraddr.sin_port = htons(port);

	return sockfd;
}

static void pokeServer(int fd) {
	char buff[1]="C";
	int n=sendto(fd, buff, 1, 0, &serveraddr, sizeof(serveraddr));
	if (n < 0) printf("ERROR in sendto");
}

#define BUF_MAX 1500

void bppConnectUsingUdp(const char *ssid, const char *server, int port, int powerHandle) {
	esp_err_t r;
	powerHold(powerHandle, 10*1000);
	printf("bppConnectUsingUdp: connecting to %s\n", ssid);
#if 0
	wifi_event_group = xEventGroupCreate();
	r=esp_event_loop_init(wifi_event_handler, NULL);
	if (r!=ESP_OK) {
		printf("bppConnectUsingUdp: esp_event_loop_init failed\n");
		return;
	}
#endif
	wifi_config_t wifi_config = {
		.sta = {
			.password = "",
		},
	};
	strcpy((char*)wifi_config.sta.ssid, ssid);
	r=esp_wifi_set_mode(WIFI_MODE_STA);
	if (r!=ESP_OK) {
		printf("bppConnectUsingUdp: wifi_set_mode failed\n");
		return;
	}
	r=esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
	if (r!=ESP_OK) {
		printf("bppConnectUsingUdp: wifi_set_config failed\n");
		return;
	}
	r=esp_wifi_start();
	if (r!=ESP_OK) {
		printf("bppConnectUsingUdp: wifi_start failed\n");
		return;
	}
	vTaskDelay(10);
	esp_wifi_connect();
#if 0
	int b=xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, 10000/portTICK_PERIOD_MS);
	if (!b) {
		//Timeout.
		return;
	}
#else
	vTaskDelay(10000/portTICK_PERIOD_MS);
#endif
	
	int s=connectBppServer(server, port);
	if (s<0) return;
	pokeServer(s);
	
	time_t udpStart=time(NULL);
	int errors=0;
	uint8_t *rbuf=malloc(BUF_MAX);
	if (!rbuf) return;
	while(1) {
		struct timeval tv;
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(s, &fds);
		tv.tv_sec=1;
		tv.tv_usec=0;
		if(select(s+1, &fds, NULL, NULL, &tv)) {
			int l=read(s, rbuf, BUF_MAX);
			int success=chksignRecv(rbuf, l);
			if (success) errors=0; else errors++;
		} else {
			pokeServer(s);
			errors++;
		}
		if (errors>=10) {
			printf("bppConnectUsingUdp: Too many errors/timeouts, bailing out\n");
			return;
		}
		time_t timeRunning=time(NULL)-udpStart;
		if (timeRunning>10*60) {
			powerCanSleep(powerHandle);
		}
	}
}

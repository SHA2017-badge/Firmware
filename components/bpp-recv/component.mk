#
# Component Makefile
#


COMPONENT_ADD_INCLUDEDIRS := . common
COMPONENT_SOURCES := . common
COMPONENT_OBJS := bd_flatflash.o blkidcache_mlvl.o blockdecode.o chksign_ed25519.o defec.o hkpackets.o \
					hldemux.o powerdown.o serdec.o subtitle.o crc16-ccitt.o defec_parity.o defec_rs.o \
					bd_ropart.o mountbd.o bma.o



DEFINES+=PROJECT_CONF_H=\"project-conf.h\"

all: contiki-alert

emul8: all
	emul8 contiki-alert.emul8

PROJECT_SOURCEFILES += config.c net-utils.c io-utils.c queue.c

CONTIKI_WITH_IPV6 = 1

APPS += json mqtt mqtt-service ping-service

CONTIKI = ../contiki
include $(CONTIKI)/Makefile.include

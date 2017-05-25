DEFINES+=PROJECT_CONF_H=\"project-conf.h\"

all: contiki-alert

emul8: all
	emul8 contiki-alert.emul8

PROJECT_SOURCEFILES += mqtt-service.c ping-service.c config-service.c net-utils.c io-utils.c queue.c

CONTIKI_WITH_IPV6 = 1

APPS += mqtt json

CONTIKI = ../contiki
include $(CONTIKI)/Makefile.include

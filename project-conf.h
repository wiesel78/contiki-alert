#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define MQTT_BROKER_IP_ADDR "fd00::1"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

#define TOPIC_ADD_JOB_PREFIX "job/add"
#define TOPIC_ADD_JOB "job/add/#"

#define TOPIC_DELETE_JOB_PREFIX "job/delete"
#define TOPIC_DELETE_JOB "job/delete/#"

#define TOPIC_DETAILS_JOB "job/details/%s/%d"
#define TOPIC_PUBLISH_CLIENT_ACTIVE "clients/%s"


#if CONTIKI_TARGET_CC2538DK || CONTIKI_TARGET_OPENMOTE_CC2538 || \
    CONTIKI_TARGET_ZOUL
#define COFFEE_CONF_SIZE              (CC2538_DEV_FLASH_SIZE / 2)
#define COFFEE_CONF_MICRO_LOGS        1
#define COFFEE_CONF_APPEND_ONLY       0
#endif /* CONTIKI_TARGET_CC2538DK || CONTIKI_TARGET_ZOUL */


#endif /* PROJECT_CONF_H_ */

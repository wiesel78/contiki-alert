/*
    MQTT-Service is a abstraction layer to send mqtt message with
    job queue
 */

#ifndef MQTT_SERVICE_H_
#define MQTT_SERVICE_H_

#include "contiki-conf.h"
#include "sys/process.h"
#include "rpl/rpl-private.h"
#include "mqtt.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "./debug.h"

#if CONTIKI_TARGET_CC2538DK
#define PUBLISH_TRIGGER             &button_sensor
#define REBOOT_TRIGGER              &button_down_sensor
#else
#define PUBLISH_TRIGGER NULL
#define REBOOT_TRIGGER NULL
#endif



/* Constants */
#define MQTT_META_BUFFER_SIZE       64 // 64
#define MQTT_DATA_BUFFER_SIZE       512 // 512

#define MQTT_QUEUE_MAX_ITEMS        16 // 16
/* CONFIG */
#define CONFIG_USERNAME_LEN         32
#define CONFIG_PASSWORD_LEN         32
#define CONFIG_TYPE_ID_LEN          32
#define CONFIG_AUTH_TOKEN_LEN       32
#define CONFIG_EVENT_TYPE_ID_LEN    32
#define CONFIG_CMD_TYPE_LEN         8
#define CONFIG_IP_ADDR_STR_LEN      64

#define MAX_JOBS                    16
#define MAX_TCP_SEGMENT_SIZE        32
#define MAX_SUBSCRIBE_REPEAT        10
#define MAX_PUBLISH_REPEAT          10

#define LED_ERROR                   LEDS_RED
#define LED_CONNECTING              LEDS_GREEN

#define RETRY_FOREVER               0xFF
#define RECONNECT_INTERVAL          (CLOCK_SECOND * 2)
#define RECONNECT_ATTEMPTS          RETRY_FOREVER
#define CONNECTION_STABLE_TIME      (CLOCK_SECOND * 5)
#define CONNECTING_LED_DURATION     (CLOCK_SECOND >> 2)
#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)
#define STATE_MACHINE_PERIODIC      (CLOCK_SECOND >> 1)
#define ALERT_CHECK_INTERVAL        (CLOCK_SECOND * 5)



/* Defaults */
#define DEFAULT_TYPE_ID             "cc2538"
#define DEFAULT_EVENT_TYPE_ID       "status"
#define DEFAULT_SUBSCRIBE_CMD_TYPE  "+"
#define DEFAULT_BROKER_PORT         1883
#define DEFAULT_KEEP_ALIVE_TIMER    60




/* possible states for mqtt state machine */
typedef enum {
    MQTT_SERVICE_STATE_INIT,
    MQTT_SERVICE_STATE_REGISTERED,
    MQTT_SERVICE_STATE_CONNECTING,
    MQTT_SERVICE_STATE_CONNECTED,
    MQTT_SERVICE_STATE_READY,
    MQTT_SERVICE_STATE_DISCONNECTED,
    MQTT_SERVICE_STATE_NEWCONFIG,
    MQTT_SERVICE_STATE_CONFIG_ERROR,
    MQTT_SERVICE_STATE_ERROR,
} mqtt_state_t;


/* item that conatins full subscription informations */
typedef struct subscribe_item {
    char topic[MQTT_META_BUFFER_SIZE];
    uint8_t topic_length;
} subscribe_item_t;

/* item that contains a full message to send */
typedef struct publish_item {
    mqtt_qos_level_t qos_level;
    mqtt_retain_t retain;
    uint8_t is_last_will;
    char topic[MQTT_META_BUFFER_SIZE];
    uint8_t topic_length;
    char data[MQTT_DATA_BUFFER_SIZE];
    uint16_t data_length;
} publish_item_t;

/* subscribe job for subscribe a topic */
typedef struct mqtt_subscribe_job {
    char *topic;
    mqtt_qos_level_t qos_level;
    struct ctimer sub_timer;
} mqtt_subscribe_job_t;


typedef struct mqtt_service_state {
    mqtt_state_t state;
    int sequenz_number;
    int connect_attempt;
    int subscribe_attempt;
    struct timer connection_life;
    struct ctimer led_timer;
    struct ctimer publish_timer;
    struct ctimer periodic_timer;
    struct ctimer alert_timer;
    mqtt_subscribe_job_t subscribe_job;
    void (*publish_handler)(const char *, uint16_t, const uint8_t *, uint16_t);
} mqtt_service_state_t;

/* MQTT client config contains all important connection settings */
typedef struct mqtt_client_config {
    char username[CONFIG_USERNAME_LEN];
    char password[CONFIG_PASSWORD_LEN];
    char client_id[MQTT_META_BUFFER_SIZE];
    char type_id[CONFIG_TYPE_ID_LEN];
    char event_type_id[CONFIG_EVENT_TYPE_ID_LEN];
    char broker_ip[CONFIG_IP_ADDR_STR_LEN];
    char cmd_type[CONFIG_CMD_TYPE_LEN];
    uint16_t broker_port;
} mqtt_client_config_t;

extern process_event_t mqtt_event;

/* initialize mqtt-service
 * @param p : pointer to main process
 * @param config : config object for mqtt connection and job queues
 * @param state : state object that contains live informations of this service
 */
extern void mqtt_service_init(  struct process *p,
                                mqtt_client_config_t *config,
                                mqtt_service_state_t *state);

/* check for connectivity
 * @return 1 if conection is established, 0 if connection is down */
extern int mqtt_service_is_connected(void);

/* subscribe a topic
 * @param topic : string that represent the topic of subscription
 * @param qos_level : the Quality of Service level 0-2
 */
extern void mqtt_service_subscribe(char *topic, mqtt_qos_level_t qos_level);

/* publish by a Message by a publish_item
 * @param pub_item : object that contains publish data and informations */
extern void mqtt_service_publish(publish_item_t *pub_item);


PROCESS_NAME(mqtt_service_process);


#endif /* MQTT_SERVICE_H_ */

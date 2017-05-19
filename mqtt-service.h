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

/* Constants */
#define MQTT_META_BUFFER_SIZE       64
#define MQTT_DATA_BUFFER_SIZE       512
/* CONFIG */
#define CONFIG_USERNAME_LEN         32
#define CONFIG_PASSWORD_LEN         32
#define CONFIG_TYPE_ID_LEN          32
#define CONFIG_AUTH_TOKEN_LEN       32
#define CONFIG_EVENT_TYPE_ID_LEN    32
#define CONFIG_CMD_TYPE_LEN         8
#define CONFIG_IP_ADDR_STR_LEN      64
#define MAX_STATUS_JOBS             6
#define MAX_ALERT_JOBS              6
#define MAX_TCP_SEGMENT_SIZE        32
#define MAX_SUBSCRIBE_REPEAT        10
#define MAX_PUBLISH_REPEAT          10

#define LED_ERROR                   LEDS_RED
#define LED_CONNECTING              LEDS_GREEN
/* Defaults */
#define DEFAULT_TYPE_ID             "cc2538"
#define DEFAULT_EVENT_TYPE_ID       "status"
#define DEFAULT_SUBSCRIBE_CMD_TYPE  "+"
#define DEFAULT_BROKER_PORT         1883
#define DEFAULT_KEEP_ALIVE_TIMER    60

#define PUBLISH_TRIGGER             &button_sensor
#define RETRY_FOREVER               0xFF
#define RECONNECT_INTERVAL          (CLOCK_SECOND * 2)
#define RECONNECT_ATTEMPTS          RETRY_FOREVER
#define CONNECTION_STABLE_TIME      (CLOCK_SECOND * 5)
#define CONNECTING_LED_DURATION     (CLOCK_SECOND >> 2)
#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)
#define STATE_MACHINE_PERIODIC      (CLOCK_SECOND >> 1)

/* temp in mC, 
 * rssi in dBm, 
 * uptime in seconds,
 * power in mV */
#define JSON_STATUS_LIGHT           "\"light\":\"%d\","
#define JSON_STATUS_TEMPERATURE     "\"temperature\":\"%d\","
#define JSON_STATUS_UPTIME          "\"uptime\":\"%d\","
#define JSON_STATUS_POWER           "\"power\":\"%d\","
#define JSON_STATUS_IPV6            "\"ipv6\":\"%s\","
#define JSON_STATUS_RSSI            "\"signal\":\"%d\","
#define JSON_STATUS_CLIENT_ID       "\"id\":\"%s\","
#define JSON_STATUS_SEQUENCE_NUMBER "\"sequence\":\"%d\""

#define GET_UPTIME (clock_seconds())

typedef enum {
    lower = 0x1,
    greater = 0x2,
    equal = 0x4,
} compare_operator_t;

typedef enum {
    DEVICE_STATUS_LIGHT = 1 << 0,
    DEVICE_STATUS_TEMPERATURE = 1 << 1,
    DEVICE_STATUS_UPTIME = 1 << 2,
    DEVICE_STATUS_POWER = 1 << 3,
    DEVICE_STATUS_IPV6 = 1 << 4,
    DEVICE_STATUS_RSSI = 1 << 5,
    DEVICE_STATUS_CLIENT_ID = 1 << 6,
    DEVICE_STATUS_ALL = 
        DEVICE_STATUS_LIGHT |
        DEVICE_STATUS_TEMPERATURE |
        DEVICE_STATUS_UPTIME |
        DEVICE_STATUS_POWER |
        DEVICE_STATUS_IPV6 |
        DEVICE_STATUS_RSSI |
        DEVICE_STATUS_CLIENT_ID,
} device_status_t;

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


typedef struct subscribe_item {
    char topic[MQTT_META_BUFFER_SIZE];
    uint8_t topic_length;
} subscribe_item_t;

typedef struct publish_item {
    char topic[MQTT_META_BUFFER_SIZE];
    uint8_t topic_length;
    char data[MQTT_DATA_BUFFER_SIZE];
    uint16_t data_length;
} publish_item_t;

typedef struct mqtt_subscribe_job {
    char *topic;
    mqtt_qos_level_t qos_level;
    struct ctimer sub_timer;
} mqtt_subscribe_job_t;

typedef struct mqtt_publish_job {
    char *topic;
    uint8_t topic_len;
    char *data;
    uint16_t data_len;
    struct ctimer timer;
} mqtt_publish_job_t;

typedef struct mqtt_publish_status_job {
    char topic[MQTT_META_BUFFER_SIZE];
    device_status_t status;
    int id;
    int interval;
    int time_from;
    int time_to;
    struct ctimer timer;
} mqtt_publish_status_job_t;


/*
 * example light : 
 *      from, 
 *      to, 
 *      if 
 *          (light/temperature/power/rssi):device_status_t
 *          (lower/greater)[equal]:
 *          value
 */
typedef struct mqtt_publish_alert_job {
    char topic[MQTT_META_BUFFER_SIZE];
    device_status_t status;
    compare_operator_t op;
    int id;
    int time_from;
    int time_to;
    int value;
    struct ctimer timer;
} mqtt_publish_alert_job_t;

typedef struct mqtt_service_state {
    mqtt_state_t state;
    int sequenz_number;
    int connect_attempt;
    int subscribe_tries;
    struct timer connection_life; 
    struct ctimer led_timer;
    struct ctimer publish_timer;
    struct etimer periodic_timer;
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
    mqtt_publish_status_job_t status_jobs[MAX_STATUS_JOBS];
    mqtt_publish_alert_job_t alert_jobs[MAX_ALERT_JOBS];
} mqtt_client_config_t;

extern process_event_t mqtt_event;

/* Methods */
extern void mqtt_service_init(  struct process *p, 
                                mqtt_client_config_t *config, 
                                mqtt_service_state_t *state);
extern void mqtt_service_update(process_event_t ev, process_data_t data);
extern int mqtt_service_is_connected(void);

extern void mqtt_service_subscribe(char *topic, mqtt_qos_level_t qos_level);

extern void mqtt_service_publish(publish_item_t *pub_item);



#endif /* MQTT_SERVICE_H_ */

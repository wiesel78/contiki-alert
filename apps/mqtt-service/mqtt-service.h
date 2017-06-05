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

#define MQTT_QUEUE_MAX_ITEMS        16
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
#define REBOOT_TRIGGER              &button_down_sensor
#define RETRY_FOREVER               0xFF
#define RECONNECT_INTERVAL          (CLOCK_SECOND * 2)
#define RECONNECT_ATTEMPTS          RETRY_FOREVER
#define CONNECTION_STABLE_TIME      (CLOCK_SECOND * 5)
#define CONNECTING_LED_DURATION     (CLOCK_SECOND >> 2)
#define NET_CONNECT_PERIODIC        (CLOCK_SECOND >> 2)
#define STATE_MACHINE_PERIODIC      (CLOCK_SECOND >> 1)
#define ALERT_CHECK_INTERVAL        (CLOCK_SECOND * 5)



/* config json keys */
#define JSON_CONFIG_KEY_MQTT            "mqttConf"
#define JSON_CONFIG_KEY_USERNAME        "username"
#define JSON_CONFIG_KEY_PASSWORD        "password"
#define JSON_CONFIG_KEY_CLIENT_ID       "clientId"
#define JSON_CONFIG_KEY_TYPE_ID         "typeId"
#define JSON_CONFIG_KEY_EVENT_TYPE_ID   "eventTypeId"
#define JSON_CONFIG_KEY_BROKER_IP       "brokerIp"
#define JSON_CONFIG_KEY_BROKER_PORT     "brokerPort"
#define JSON_CONFIG_KEY_CMD_TYPE        "cmdType"
#define JSON_CONFIG_KEY_ALERT_CHECK_INTERVAL  "alertCheckInterval"
#define JSON_CONFIG_KEY_CURRENT_JOB_ID  "jobId"

#define JSON_CONFIG_JOB_KEY_STATUS_JOB "statusJob"
#define JSON_CONFIG_JOB_KEY_ALERT_JOB  "alertJob"
#define JSON_CONFIG_JOB_KEY_TOPIC       "topic"
#define JSON_CONFIG_JOB_KEY_STATUS      "status"
#define JSON_CONFIG_JOB_KEY_TYPE        "type"
#define JSON_CONFIG_JOB_KEY_ID          "id"
#define JSON_CONFIG_JOB_KEY_INTERVAL    "interval"
#define JSON_CONFIG_JOB_KEY_OPERATOR    "operator"
#define JSON_CONFIG_JOB_KEY_DURATION    "duration"
#define JSON_CONFIG_JOB_KEY_TIME_ELAPSED "timeElapsed"
#define JSON_CONFIG_JOB_KEY_STATUS_VALUE "value"
#define JSON_CONFIG_JOB_KEY_TIME_FROM   "timeFrom"
#define JSON_CONFIG_JOB_KEY_TIME_TO     "timeTo"

#define JSON_KEY_ID                 "id"
#define JSON_KEY_TOPIC              "topic"
#define JSON_KEY_STATUS             "status"
#define JSON_KEY_INTERVAL           "interval"
#define JSON_KEY_DURATION           "duration"
#define JSON_KEY_TIME_FROM          "timeFrom"
#define JSON_KEY_TIME_TO            "timeTo"
#define JSON_KEY_STATUS_VALUE       "value"
#define JSON_KEY_COMPARE_OPERATOR   "operator"
#define JSON_KEY_IS_ONLINE          "isOnline"
#define JSON_KEY_JOB_ID             "jobId"

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
#define JSON_STATUS_IS_ONLINE       "\"isOnline\":\"%d\","
#define JSON_STATUS_JOB_ID          "\"jobId\":\"%d\","
#define JSON_STATUS_SEQUENCE_NUMBER "\"sequence\":\"%d\""

/* config json keys */
#define JSON_CONFIG_USERNAME        "\"" JSON_CONFIG_KEY_USERNAME "\":\"%s\","
#define JSON_CONFIG_PASSWORD        "\"" JSON_CONFIG_KEY_PASSWORD "\":\"%s\","
#define JSON_CONFIG_CLIENT_ID       "\"" JSON_CONFIG_KEY_CLIENT_ID "\":\"%s\","
#define JSON_CONFIG_TYPE_ID         "\"" JSON_CONFIG_KEY_TYPE_ID "\":\"%s\","
#define JSON_CONFIG_EVENT_TYPE_ID   "\"" JSON_CONFIG_KEY_EVENT_TYPE_ID "\":\"%s\","
#define JSON_CONFIG_BROKER_IP       "\"" JSON_CONFIG_KEY_BROKER_IP "\":\"%s\","
#define JSON_CONFIG_BROKER_PORT     "\"" JSON_CONFIG_KEY_BROKER_PORT "\":%d,"
#define JSON_CONFIG_CMD_TYPE        "\"" JSON_CONFIG_KEY_CMD_TYPE "\":\"%s\","
#define JSON_CONFIG_ALERT_CHECK_INTERVAL  "\"" JSON_CONFIG_KEY_ALERT_CHECK_INTERVAL "\":%d,"
#define JSON_CONFIG_IS_ONLINE       "\"" JSON_KEY_IS_ONLINE "\":%d,"

#define JSON_CONFIG_JOB_TOPIC       "\"" JSON_CONFIG_JOB_KEY_TOPIC "\":\"%s\","
#define JSON_CONFIG_JOB_TYPE        "\"" JSON_CONFIG_JOB_KEY_TYPE "\":%d,"
#define JSON_CONFIG_JOB_STATUS      "\"" JSON_CONFIG_JOB_KEY_STATUS "\":%d,"
#define JSON_CONFIG_JOB_ID          "\"" JSON_CONFIG_JOB_KEY_ID "\":%d,"
#define JSON_CONFIG_JOB_INTERVAL    "\"" JSON_CONFIG_JOB_KEY_INTERVAL "\":%d,"
#define JSON_CONFIG_JOB_OPERATOR    "\"" JSON_CONFIG_JOB_KEY_OPERATOR "\":%d,"
#define JSON_CONFIG_JOB_DURATION    "\"" JSON_CONFIG_JOB_KEY_DURATION "\":%d,"
#define JSON_CONFIG_JOB_TIME_ELAPSED "\"" JSON_CONFIG_JOB_KEY_TIME_ELAPSED "\":%d,"
#define JSON_CONFIG_JOB_STATUS_VALUE "\"" JSON_CONFIG_JOB_KEY_STATUS_VALUE "\":%d,"
#define JSON_CONFIG_JOB_TIME_FROM   "\"" JSON_CONFIG_JOB_KEY_TIME_FROM "\":%d,"
#define JSON_CONFIG_JOB_TIME_TO     "\"" JSON_CONFIG_JOB_KEY_TIME_TO "\":%d"

#define JSON_CONFIG_JOBS "\"job\":"
#define JSON_JOB_ALERT "{" \
    JSON_CONFIG_JOB_TYPE \
    JSON_CONFIG_JOB_TOPIC \
    JSON_CONFIG_JOB_STATUS \
    JSON_CONFIG_JOB_ID \
    JSON_CONFIG_JOB_OPERATOR \
    JSON_CONFIG_JOB_DURATION \
    JSON_CONFIG_JOB_STATUS_VALUE \
    JSON_CONFIG_JOB_TIME_FROM \
    JSON_CONFIG_JOB_TIME_TO \
"},"

#define JSON_JOB_STATUS "{" \
    JSON_CONFIG_JOB_TYPE \
    JSON_CONFIG_JOB_TOPIC \
    JSON_CONFIG_JOB_STATUS \
    JSON_CONFIG_JOB_ID \
    JSON_CONFIG_JOB_INTERVAL \
    JSON_CONFIG_JOB_TIME_FROM \
    JSON_CONFIG_JOB_TIME_TO \
"},"

#define JSON_CLIENT_ACTIVE "{" \
    JSON_CONFIG_IS_ONLINE \
    JSON_CONFIG_CLIENT_ID \
    JSON_STATUS_IPV6 \
    JSON_STATUS_RSSI \
    JSON_STATUS_SEQUENCE_NUMBER \
"}"

#define JSON_HAS_NEXT(json)         ((json).pos < (json).len)
#define JSON_IS_ERROR(json)         ((json).vtype == JSON_TYPE_ERROR)
#define JSON_IS_KEY(json)           ((json).vtype == JSON_TYPE_PAIR_NAME)
#define JSON_IS_VALUE(json)         ( \
                                        (json).vtype == JSON_TYPE_STRING || \
                                        (json).vtype == JSON_TYPE_PAIR || \
                                        (json).vtype == JSON_TYPE_UINT || \
                                        (json).vtype == JSON_TYPE_INT || \
                                        (json).vtype == JSON_TYPE_NUMBER \
                                    )
#define JSON_BACK_IF_NOT_VALUE(json, count) { \
    jsonparse_next(&(json)); \
    if(!(JSON_IS_VALUE((json)))){ \
        if(JSON_IS_ERROR((json))){ \
            printf("BINV error\n\r"); \
        } \
        printf("BINV continue\n\r"); \
        continue; \
    } \
    (count)++; \
}

#define GET_UPTIME (clock_seconds())

typedef enum {
    COMPARE_OPERATOR_LOWER = 0x1,
    COMPARE_OPERATOR_GREATER = 0x2,
    COMPARE_OPERATOR_EQUAL= 0x4,
    COMPARE_OPERATOR_LOWER_EQUAL =
        COMPARE_OPERATOR_LOWER |
        COMPARE_OPERATOR_EQUAL,
    COMPARE_OPERATOR_GREATE_EQUAL =
        COMPARE_OPERATOR_GREATER |
        COMPARE_OPERATOR_EQUAL,
} compare_operator_t;

typedef enum {
    DEVICE_STATUS_LIGHT         = 1 << 0,
    DEVICE_STATUS_TEMPERATURE   = 1 << 1,
    DEVICE_STATUS_UPTIME        = 1 << 2,
    DEVICE_STATUS_POWER         = 1 << 3,
    DEVICE_STATUS_IPV6          = 1 << 4,
    DEVICE_STATUS_RSSI          = 1 << 5,
    DEVICE_STATUS_CLIENT_ID     = 1 << 6,
    DEVICE_STATUS_ALL           =   DEVICE_STATUS_LIGHT |
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

typedef enum {
    JOB_TYPE_STATUS     = 1 << 0,
    JOB_TYPE_ALERT      = 1 << 1,
} job_type_t;


typedef struct subscribe_item {
    char topic[MQTT_META_BUFFER_SIZE];
    uint8_t topic_length;
} subscribe_item_t;

typedef struct publish_item {
    mqtt_qos_level_t qos_level;
    mqtt_retain_t retain;
    uint8_t is_last_will;
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
    int duration;
    int time_elapsed;
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
    int alert_check_interval;
    int job_id;
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

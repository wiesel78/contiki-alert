

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
#define JSON_KEY_TYPE               "type"
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
#define JSON_CONFIG_JOB_TIME_ELAPSED "\"" JSON_CONFIG_JOB_KEY_TIME_ELAPSED "\":%d,"
#define JSON_CONFIG_JOB_STATUS_VALUE "\"" JSON_CONFIG_JOB_KEY_STATUS_VALUE "\":%d,"
#define JSON_CONFIG_JOB_TIME_FROM   "\"" JSON_CONFIG_JOB_KEY_TIME_FROM "\":%d,"
#define JSON_CONFIG_JOB_TIME_TO     "\"" JSON_CONFIG_JOB_KEY_TIME_TO "\":%d"

#define JSON_CONFIG_JOBS "\"job\":"

#define JSON_JOB "{" \
    JSON_CONFIG_JOB_TYPE \
    JSON_CONFIG_JOB_TOPIC \
    JSON_CONFIG_JOB_STATUS \
    JSON_CONFIG_JOB_ID \
    JSON_CONFIG_JOB_OPERATOR \
    JSON_CONFIG_JOB_INTERVAL \
    JSON_CONFIG_JOB_STATUS_VALUE \
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
            PRINTF("BINV error\n\r"); \
        } \
        PRINTF("BINV continue\n\r"); \
        continue; \
    } \
    (count)++; \
}

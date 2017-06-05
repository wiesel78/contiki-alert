#include "contiki.h"
#include "cfs/cfs.h"
#include "jsonparse.h"
#include "jsontree.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"

#include "dev/button-sensor.h"
#include "dev/als-sensor.h"


#include <stdio.h>
#include <stdarg.h>


#include "ping-service.h"
#include "mqtt-service.h"

#include "./project-conf.h"
#include "./device-utils.h"
#include "./config.h"
#include "./net-utils.h"
#include "./io-utils.h"

#define PUBLISH_SECONDS (CLOCK_SECOND * 10)

#define DEFAULT_STATUS_JOB_TOPIC "status/new"
#define DEFAULT_ALERT_JOB_TOPIC "alert/new"

#define SET_IPV6_ADDRESS(ipv6_buf) { \
    memset((ipv6_buf), 0, sizeof((ipv6_buf))); \
    ipaddr_sprintf((ipv6_buf), sizeof((ipv6_buf)), uip_ds6_defrt_choose()); \
}

client_config_t conf;
client_state_t state;

static int is_subscribe = 0;
static char ipv6_addr_str[MQTT_META_BUFFER_SIZE];
static char receive_buffer[MQTT_DATA_BUFFER_SIZE];
static char *buffer_ptr;
static mqtt_publish_status_job_t status_job_buffer;
static mqtt_publish_alert_job_t alert_job_buffer;
static publish_item_t pub_item;
static publish_item_t pub_alert_item;
static device_status_t alert_status_buffer;
static int alert_extend_duration;

static mqtt_publish_status_job_t default_status_job = {
    .id = 0,
    .topic = "status/default",
    .status = DEVICE_STATUS_ALL,
    .interval = 30000,
    .time_from = 36000000,
    .time_to = 72000000
};

static void
show_status_job(mqtt_publish_status_job_t *job)
{
    printf("\n\r");
    printf("## Job #%d -> %p \n\r", job->id, job);
    printf("##   topic : %s \n\r", job->topic);
    printf("##   status : %d \n\r", (int)(job->status));
    printf("##   interval : %d \n\r", job->interval);
    printf("##   time_from : %d \n\r", job->time_from);
    printf("##   time_to : %d \n\r", job->time_to);
    printf("###################\n\r");
}

static void
show_alert_job(mqtt_publish_alert_job_t *job)
{
    printf("\n\r");
    printf("## Alert-Job #%d -> %p \n\r", job->id, job);
    printf("##   topic : %s \n\r", job->topic);
    printf("##   status : %d \n\r", (int)(job->status));
    printf("##   op : %d \n\r", (int)(job->op));
    printf("##   value : %d \n\r", job->value);
    printf("##   duration : %d \n\r", job->duration);
    printf("##   time_from : %d \n\r", job->time_from);
    printf("##   time_to : %d \n\r", job->time_to);
    printf("###################\n\r");
}

/* publish a client active message (to have a client list on server side)*/
static void
publish_client_active(uint8_t is_last_will)
{
    printf("publish client \n\r");

    int remaining = MQTT_DATA_BUFFER_SIZE;

    snprintf(pub_item.topic, MQTT_META_BUFFER_SIZE,
        TOPIC_PUBLISH_CLIENT_ACTIVE, conf.mqtt_conf.client_id);

    pub_item.topic_length = strlen(pub_item.topic);
    buffer_ptr = pub_item.data;

    SET_IPV6_ADDRESS(ipv6_addr_str);

    // payload
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CLIENT_ACTIVE,
        is_last_will ? 0 : 1,
        conf.mqtt_conf.client_id,
        ipv6_addr_str,
        state.ping_state.rssi,
        state.mqtt_state.sequenz_number
    );

    pub_item.data_length = MQTT_DATA_BUFFER_SIZE - remaining;
    pub_item.qos_level = MQTT_QOS_LEVEL_1;
    pub_item.retain = MQTT_RETAIN_ON;
    pub_item.is_last_will = is_last_will;

    mqtt_service_publish(&pub_item);

    if(!is_last_will){
        publish_client_active(1);
    }
}


/* publish a status-job */
static void
publish_status(mqtt_publish_status_job_t *job)
{
    printf("publish status\n\r");

    int remaining = MQTT_DATA_BUFFER_SIZE;

    if(job == NULL){
        return;
    }

    memset(pub_item.topic, 0, MQTT_META_BUFFER_SIZE);
    pub_item.topic_length = snprintf(pub_item.topic, MQTT_META_BUFFER_SIZE, job->topic);
    buffer_ptr = pub_item.data;

    // json begin
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "{");

    // light
    if((job->status) & DEVICE_STATUS_LIGHT){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_LIGHT, GET_VALUE_LIGHT);
    }

    // temperature
    if((job->status) & DEVICE_STATUS_TEMPERATURE){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_TEMPERATURE, GET_VALUE_TEMPERATURE);
    }

    // uptime
    if((job->status) & DEVICE_STATUS_UPTIME){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_UPTIME, GET_UPTIME);
    }

    // power / battery
    if((job->status) & DEVICE_STATUS_POWER){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_POWER, GET_POWER);
    }

    // ipv6
    if((job->status) & DEVICE_STATUS_IPV6){
        SET_IPV6_ADDRESS(ipv6_addr_str);
        // memset(ipv6_addr_str, 0, sizeof(ipv6_addr_str));
        // ipaddr_sprintf(ipv6_addr_str, sizeof(ipv6_addr_str), uip_ds6_defrt_choose());
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_IPV6, ipv6_addr_str);
    }

    // signal strength
    if((job->status) & DEVICE_STATUS_RSSI){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_RSSI, state.ping_state.rssi);
    }

    // client id
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_CLIENT_ID, conf.mqtt_conf.client_id);

    // job id
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_JOB_ID, job->id);

    // job type
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_JOB_TYPE, JOB_TYPE_STATUS);

    // sequence number
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_SEQUENCE_NUMBER, state.mqtt_state.sequenz_number);

    // json end
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "}");

    pub_item.data_length = MQTT_DATA_BUFFER_SIZE - remaining;
    pub_item.qos_level = MQTT_QOS_LEVEL_0;
    pub_item.retain = MQTT_RETAIN_OFF;
    pub_item.is_last_will = 0;

    mqtt_service_publish(&pub_item);
}

/* publish a status-job */
static void
publish_alert(mqtt_publish_alert_job_t *job)
{
    printf("publish alert\n\r");

    int remaining = MQTT_DATA_BUFFER_SIZE;

    if(job == NULL){
        return;
    }

    memset(pub_alert_item.topic, 0, MQTT_META_BUFFER_SIZE);
    pub_alert_item.topic_length = snprintf(pub_alert_item.topic, MQTT_META_BUFFER_SIZE, job->topic);
    buffer_ptr = pub_alert_item.data;

    // json begin
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "{");

    // light
    if((job->status) & DEVICE_STATUS_LIGHT){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_LIGHT, GET_VALUE_LIGHT);
    }

    // temperature
    if((job->status) & DEVICE_STATUS_TEMPERATURE){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_TEMPERATURE, GET_VALUE_TEMPERATURE);
    }

    // uptime
    if((job->status) & DEVICE_STATUS_UPTIME){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_UPTIME, GET_UPTIME);
    }

    // power / battery
    if((job->status) & DEVICE_STATUS_POWER){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_POWER, GET_POWER);
    }

    // signal strength
    if((job->status) & DEVICE_STATUS_RSSI){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_RSSI, state.ping_state.rssi);
    }

    // client id
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_CLIENT_ID, conf.mqtt_conf.client_id);

    // job id
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_JOB_ID, job->id);

    // job type
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_JOB_TYPE, JOB_TYPE_ALERT);

    // sequence number
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_SEQUENCE_NUMBER, state.mqtt_state.sequenz_number);

    // json end
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "}");

    pub_alert_item.data_length = MQTT_DATA_BUFFER_SIZE - remaining;
    pub_alert_item.qos_level = MQTT_QOS_LEVEL_0;
    pub_alert_item.retain = MQTT_RETAIN_OFF;
    pub_alert_item.is_last_will = 0;

    mqtt_service_publish(&pub_alert_item);
}

/* publish a status-job */
static void
publish_status_job_details(mqtt_publish_status_job_t *job)
{
    printf("publish status job %d\n\r", job->id);

    int remaining = MQTT_DATA_BUFFER_SIZE;


    pub_item.topic_length = snprintf(pub_item.topic, MQTT_META_BUFFER_SIZE,
        TOPIC_DETAILS_JOB, conf.mqtt_conf.client_id, job->id);

    buffer_ptr = pub_item.data;

    // json begin
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "{");

    // alert job config
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_JOBS JSON_JOB_STATUS,
        JOB_TYPE_STATUS,
        job->topic,
        job->status,
        job->id,
        job->interval,
        job->time_from,
        job->time_to
    );

    // client id
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_CLIENT_ID, conf.mqtt_conf.client_id);

    // sequence number
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_SEQUENCE_NUMBER, state.mqtt_state.sequenz_number);

    // json end
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "}");

    pub_item.data_length = MQTT_DATA_BUFFER_SIZE - remaining;
    pub_item.qos_level = MQTT_QOS_LEVEL_1;
    pub_item.retain = MQTT_RETAIN_ON;
    pub_item.is_last_will = 0;

    mqtt_service_publish(&pub_item);
}

/* publish a status-job */
static void
publish_alert_job_details(mqtt_publish_alert_job_t *job)
{
    printf("publish alert job %d\n\r", job->id);

    int remaining = MQTT_DATA_BUFFER_SIZE;


    pub_item.topic_length = snprintf(pub_item.topic, MQTT_META_BUFFER_SIZE,
        TOPIC_DETAILS_JOB, conf.mqtt_conf.client_id, job->id);

    buffer_ptr = pub_item.data;

    // json begin
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "{");

    // alert job config
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_JOBS JSON_JOB_ALERT,
        JOB_TYPE_ALERT,
        job->topic,
        job->status,
        job->id,
        job->op,
        job->duration,
        job->value,
        job->time_from,
        job->time_to
    );

    // client id
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_CLIENT_ID, conf.mqtt_conf.client_id);

    // sequence number
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_SEQUENCE_NUMBER, state.mqtt_state.sequenz_number);

    // json end
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "}");

    pub_item.data_length = MQTT_DATA_BUFFER_SIZE - remaining;
    pub_item.qos_level = MQTT_QOS_LEVEL_1;
    pub_item.retain = MQTT_RETAIN_ON;
    pub_item.is_last_will = 0;

    mqtt_service_publish(&pub_item);
}

/* call if status-job-add-request are valid and finished */
static void
status_job_callback(void *ptr){
    if(ptr == NULL){
        return;
    }

    mqtt_publish_status_job_t* job_ptr = (mqtt_publish_status_job_t *) ptr;

    show_status_job(job_ptr);

    if(job_ptr->id < 0 || job_ptr->interval <= 0){
        return;
    }

    publish_status(job_ptr);

    if(!ctimer_expired(&(job_ptr->timer))){
        ctimer_stop(&(job_ptr->timer));
    }

    ctimer_set(&(job_ptr->timer), (job_ptr->interval * CLOCK_SECOND), status_job_callback, job_ptr);
}

/* parsing json to status job and add them to status-job-list */
static void
parse_status_job(char *job_as_json, uint16_t length)
{
    int len = strlen(job_as_json);
    int count = 0;
    int index = -1;

    if(len < 1 || len > MQTT_DATA_BUFFER_SIZE){
        printf("Buffer is to small\n\r");
        return;
    }

    struct jsonparse_state json;
    jsonparse_setup(&json, job_as_json, len);

    snprintf(status_job_buffer.topic, MQTT_META_BUFFER_SIZE, DEFAULT_STATUS_JOB_TOPIC);
    status_job_buffer.id = -1;
    status_job_buffer.status = DEVICE_STATUS_ALL;
    status_job_buffer.interval = 30;
    status_job_buffer.time_from = 0;
    status_job_buffer.time_to = 0;


    while(JSON_HAS_NEXT(json)){
        jsonparse_next(&json);

        if(!(JSON_IS_KEY(json))){
            continue;
        }

        jsonparse_copy_value(&json, receive_buffer, MQTT_DATA_BUFFER_SIZE);

        if(strcasecmp(receive_buffer, JSON_KEY_ID) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            status_job_buffer.id = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TOPIC) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            jsonparse_copy_value(&json, status_job_buffer.topic, MQTT_META_BUFFER_SIZE);
        }else if(strcasecmp(receive_buffer, JSON_KEY_STATUS) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            status_job_buffer.status = (device_status_t)jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_INTERVAL) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            status_job_buffer.interval = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TIME_FROM) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            status_job_buffer.time_from = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TIME_TO) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count);

            status_job_buffer.time_to = jsonparse_get_value_as_int(&json);
        }
    }

    if(count <= 0){
        printf("count == 0\n\r");
        return;
    }

    index = status_job_list_save(&status_job_buffer);

    if(index < 0){
        printf("job konnte nicht hinzugefuegt werden\n\r");
        return;
    }

    publish_status_job_details(&(conf.mqtt_conf.status_jobs[index]));

    status_job_callback(&(conf.mqtt_conf.status_jobs[index]));
}


/* parsing json to alert job and add them to status-job-list */
static void
parse_alert_job(char *job_as_json, uint16_t length)
{
    int len = strlen(job_as_json);
    int count = 0;
    int index = -1;

    if(len < 1 || len > MQTT_DATA_BUFFER_SIZE){
        printf("Buffer is to small\n\r");
        return;
    }

    struct jsonparse_state json;
    jsonparse_setup(&json, job_as_json, len);

    snprintf(alert_job_buffer.topic, MQTT_META_BUFFER_SIZE, DEFAULT_ALERT_JOB_TOPIC);
    alert_job_buffer.id = -1;
    alert_job_buffer.status = DEVICE_STATUS_LIGHT;
    alert_job_buffer.op = COMPARE_OPERATOR_GREATE_EQUAL;
    alert_job_buffer.value = 14000;
    alert_job_buffer.duration = 60;
    alert_job_buffer.time_from = 0;
    alert_job_buffer.time_to = 0;


    while(JSON_HAS_NEXT(json)){
        jsonparse_next(&json);

        if(!(JSON_IS_KEY(json))){
            continue;
        }

        jsonparse_copy_value(&json, receive_buffer, MQTT_DATA_BUFFER_SIZE);

        if(strcasecmp(receive_buffer, JSON_KEY_ID) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            alert_job_buffer.id = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TOPIC) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            jsonparse_copy_value(&json, alert_job_buffer.topic, MQTT_META_BUFFER_SIZE);
        }else if(strcasecmp(receive_buffer, JSON_KEY_STATUS) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            alert_job_buffer.status = (device_status_t)jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_COMPARE_OPERATOR) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            alert_job_buffer.op = (compare_operator_t)jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_STATUS_VALUE) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            alert_job_buffer.value = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_DURATION) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            alert_job_buffer.duration = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TIME_FROM) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            alert_job_buffer.time_from = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TIME_TO) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count);

            alert_job_buffer.time_to = jsonparse_get_value_as_int(&json);
        }
    }

    if(count <= 0){
        printf("count == 0\n\r");
        return;
    }

    index = alert_job_list_save(&alert_job_buffer);
    show_alert_job(&alert_job_buffer);

    if(index < 0){
        printf("job konnte nicht hinzugefuegt werden\n\r");
        return;
    }

    publish_alert_job_details(&(conf.mqtt_conf.alert_jobs[index]));
}

/* parsing json to alert job and add them to status-job-list */
static void
parse_delete_request(char *request, uint16_t length, job_type_t job_type)
{
    int len = strlen(request);
    int count = 0;
    int job_id = -1;

    if(len < 1 || len > MQTT_DATA_BUFFER_SIZE){
        printf("Buffer is to small\n\r");
        return;
    }

    struct jsonparse_state json;
    jsonparse_setup(&json, request, len);

    while(JSON_HAS_NEXT(json)){
        jsonparse_next(&json);

        if(!(JSON_IS_KEY(json))){
            continue;
        }

        jsonparse_copy_value(&json, receive_buffer, MQTT_DATA_BUFFER_SIZE);

        if(strcasecmp(receive_buffer, JSON_KEY_JOB_ID) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            job_id = jsonparse_get_value_as_int(&json);
        }
    }

    if(count <= 0 || job_id < 0){
        printf("count == 0 or no job id was parsed\n\r");
        return;
    }

    switch(job_type){
        case JOB_TYPE_STATUS :
            printf("parse_delete_request status delete\n\r");
            status_job_delete(job_id);
            break;
        case JOB_TYPE_ALERT :
            printf("parse_delete_request alert delete\n\r");
            alert_job_delete(job_id);
            break;
        default:
            break;
    }
}


/* handle incoming publish messages */
static void
subscribe_handler(  const char *topic, uint16_t topic_length,
            const uint8_t *chunk, uint16_t chunk_length)
{
    printf("topic %s (length:%u) chunk length : %u\n\r",
            topic, topic_length, chunk_length);

    if(strncasecmp(topic, TOPIC_ADD_JOB_STATUS, strlen(TOPIC_ADD_JOB_STATUS)) == 0){

        if( strlen(topic) == strlen(TOPIC_ADD_JOB_STATUS)
            || strstr(topic, conf.mqtt_conf.client_id) != NULL)
        {
            printf("create job status\n\r");
            parse_status_job((char *)chunk, chunk_length);
        }

    }else if(strncasecmp(topic, TOPIC_ADD_JOB_ALERT, strlen(TOPIC_ADD_JOB_ALERT)) == 0){

        if( strlen(topic) == strlen(TOPIC_ADD_JOB_ALERT)
            || strstr(topic, conf.mqtt_conf.client_id) != NULL)
        {
            printf("create job alert\n\r");
            parse_alert_job((char *)chunk, chunk_length);
        }
    }else if(strncasecmp(topic, TOPIC_DELETE_JOB_STATUS, strlen(TOPIC_DELETE_JOB_STATUS)) == 0){

        if( strlen(topic) == strlen(TOPIC_DELETE_JOB_STATUS)
            || strstr(topic, conf.mqtt_conf.client_id) != NULL)
        {
            printf("delete job status\n\r");
            parse_delete_request((char *)chunk, chunk_length, JOB_TYPE_STATUS);
        }

    }else if(strncasecmp(topic, TOPIC_DELETE_JOB_ALERT, strlen(TOPIC_DELETE_JOB_ALERT)) == 0){

        if( strlen(topic) == strlen(TOPIC_DELETE_JOB_ALERT)
            || strstr(topic, conf.mqtt_conf.client_id) != NULL)
        {
            printf("delete job alert\n\r");
            parse_delete_request((char *)chunk, chunk_length, JOB_TYPE_ALERT);
        }
    }
}

static int
check_alert_value(int sensorvalue, mqtt_publish_alert_job_t *job){
    switch(job->op){
        case COMPARE_OPERATOR_GREATER:
            return sensorvalue > job->value;
        case COMPARE_OPERATOR_GREATE_EQUAL:
            return sensorvalue >= job->value;
        case COMPARE_OPERATOR_LOWER:
            return sensorvalue < job->value;
        case COMPARE_OPERATOR_LOWER_EQUAL:
            return sensorvalue <= job->value;
        case COMPARE_OPERATOR_EQUAL:
            return sensorvalue == job->value;
        default:
            return -1;
            break;
    }
}

static void
check_for_alerts(){
    for(int i = 0 ; i < MAX_ALERT_JOBS ; i++){
        if(conf.mqtt_conf.alert_jobs[i].id == -1){
            continue;
        }

        printf("alert job index %d\n\r", i);

        alert_extend_duration = 0;
        alert_status_buffer = conf.mqtt_conf.alert_jobs[i].status;

        switch(alert_status_buffer){
            case DEVICE_STATUS_LIGHT:
                if(check_alert_value(GET_VALUE_LIGHT, &(conf.mqtt_conf.alert_jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            case DEVICE_STATUS_TEMPERATURE:
                if(check_alert_value(GET_VALUE_TEMPERATURE, &(conf.mqtt_conf.alert_jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            case DEVICE_STATUS_POWER:
                if(check_alert_value(GET_POWER, &(conf.mqtt_conf.alert_jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            case DEVICE_STATUS_UPTIME:
                if(check_alert_value(GET_UPTIME, &(conf.mqtt_conf.alert_jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            case DEVICE_STATUS_RSSI:
                if(check_alert_value(state.ping_state.rssi, &(conf.mqtt_conf.alert_jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            default:
                break;
        }

        if(alert_extend_duration == 1){
            conf.mqtt_conf.alert_jobs[i].time_elapsed += conf.mqtt_conf.alert_check_interval;
        }else{
            conf.mqtt_conf.alert_jobs[i].time_elapsed = 0;
        }

        if(conf.mqtt_conf.alert_jobs[i].time_elapsed >= (conf.mqtt_conf.alert_jobs[i].duration * CLOCK_SECOND)){
            printf("alarm %d\n\r", conf.mqtt_conf.alert_jobs[i].id);
            publish_alert(&(conf.mqtt_conf.alert_jobs[i]));
            conf.mqtt_conf.alert_jobs[i].time_elapsed = 0;
        }
    }

    if(ctimer_expired(&(state.mqtt_state.alert_timer))){
        ctimer_set(
            &state.mqtt_state.alert_timer,
            conf.mqtt_conf.alert_check_interval,
            check_for_alerts, NULL);
    }
}

PROCESS(mqtt_service_test, "mqtt service process");
AUTOSTART_PROCESSES(&mqtt_service_test);

PROCESS_THREAD(mqtt_service_test, ev, data)
{
	PROCESS_BEGIN();

    if(init_config(&conf, &state) != 1)
    {
        PROCESS_EXIT();
    }

    update_config();

    (state.mqtt_state.publish_handler) = &subscribe_handler;

    ping_service_init(&(conf.ping_conf), &(state.ping_state));
    mqtt_service_init(&mqtt_service_test, &(conf.mqtt_conf), &(state.mqtt_state));


	while(1)
	{
		PROCESS_YIELD();

        // ping_service_update(ev, data);
        // mqtt_service_update(ev, data);

        if(mqtt_service_is_connected())
        {

            if(ev == sensors_event && data == PUBLISH_TRIGGER)
            {
                printf("publish by press button\n\r");
                publish_status(&default_status_job);
                read_config();
            }

            if(ev == sensors_event && data == REBOOT_TRIGGER){
                delete_config();
            }

            if(!is_subscribe)
            {
                mqtt_service_subscribe(TOPIC_ADD_JOB, MQTT_QOS_LEVEL_1);

                PROCESS_WAIT_EVENT_UNTIL(ev == mqtt_event);

                mqtt_service_subscribe(TOPIC_DELETE_JOB, MQTT_QOS_LEVEL_1);

                printf("subscribes done\n\r");
                is_subscribe = 1;

                ctimer_set(
                    &(state.mqtt_state.alert_timer),
                    conf.mqtt_conf.alert_check_interval,
                    check_for_alerts, NULL);

                printf("send client active message\n\r");
                publish_client_active(0);

                for(int i = 0 ; i < MAX_ALERT_JOBS ; i++){
                    if(conf.mqtt_conf.alert_jobs[i].id > -1){
                        publish_alert_job_details(&(conf.mqtt_conf.alert_jobs[i]));
                    }
                }

                for(int i = 0 ; i < MAX_STATUS_JOBS ; i++){
                    if(conf.mqtt_conf.status_jobs[i].id > -1){
                        status_job_callback(&(conf.mqtt_conf.status_jobs[i]));
                        publish_status_job_details(&(conf.mqtt_conf.status_jobs[i]));
                    }
                }
            }
        }
        else
        {
            is_subscribe = 0;
        }
	}

	PROCESS_END();
}

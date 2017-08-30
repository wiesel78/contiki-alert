#include "./contiki-alert.h"
#include "contiki.h"
#include "cfs/cfs.h"
#include "jsonparse.h"
#include "jsontree.h"
#include "json-helper.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"

#include "dev/button-sensor.h"

#if CONTIKI_TARGET_CC2538DK
#include "dev/als-sensor.h"
#endif /* CONTIKI_TARGET_CC2538DK */


#include <stdio.h>
#include <stdarg.h>


#include "ping-service.h"
#include "mqtt-service.h"

#include "./project-conf.h"
#include "./device-utils.h"
#include "./config.h"
#include "./net-utils.h"
#include "./io-utils.h"

#include "./debug.h"

#define PUBLISH_SECONDS (CLOCK_SECOND * 10)

#define DEFAULT_STATUS_JOB_TOPIC "status/new"
#define DEFAULT_ALERT_JOB_TOPIC "alert/new"

#define SET_IPV6_ADDRESS(ipv6_buf) { \
    memset((ipv6_buf), 0, sizeof((ipv6_buf))); \
    ipaddr_sprintf((ipv6_buf), sizeof((ipv6_buf)), uip_ds6_defrt_choose()); \
}

// app config and state instances
client_config_t conf;
client_state_t state;

static int is_subscribe = 0;
static char ipv6_addr_str[MQTT_META_BUFFER_SIZE];
static char *buffer_ptr;
static mqtt_publish_status_job_t job_buffer;
static device_status_t alert_status_buffer;
static int alert_extend_duration;

static publish_item_t pub_item;
static char receive_buffer[MQTT_DATA_BUFFER_SIZE];

static mqtt_publish_status_job_t default_status_job = {
    .id = 0,
    .topic = "status/default",
    .type = JOB_TYPE_STATUS,
    .status = DEVICE_STATUS_ALL,
    .interval = 30000
};

static void
show_status_job(mqtt_publish_status_job_t *job)
{
    printf("\n\r");
    printf("## Job #%d -> %p \n\r", job->id, job);
    printf("##   type : %d \n\r", job->type);
    printf("##   topic : %s \n\r", job->topic);
    printf("##   status : %d \n\r", (int)(job->status));
    printf("##   time_from : %d \n\r", job->time_from);
    printf("##   time_to : %d \n\r", job->time_to);
    printf("##   interval : %d \n\r", job->interval);
    printf("##\n\r");
    printf("##   op : %d \n\r", job->op);
    printf("##   value : %d \n\r", job->value);
    printf("###################\n\r");
}

/* publish a client active message (to have a client list on server side)
 * @param is_last_will : then you call this function, this param is 0 .
 *                       the Method publish the client infos to the server.
 *                       then the Method call it self with this param = 1
 *                       and publish a last will to the server.
 *                       if this clients is dead, the last will will be active
 *                       and set the client entry on server from active to inactive
 */
static void
publish_client_active(uint8_t is_last_will)
{
    PRINTF("publish client \n\r");

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


/* build a json message from job description and publish it
 * @param job : pointer of job description object
 */
static void
publish_job(mqtt_publish_status_job_t *job)
{
    int remaining = MQTT_DATA_BUFFER_SIZE;

    if(job == NULL || job->id < 0){
        return;
    }

    // reset and set the topic
    memset(pub_item.topic, 0, MQTT_META_BUFFER_SIZE);
    pub_item.topic_length = snprintf(pub_item.topic, MQTT_META_BUFFER_SIZE, job->topic);

    // we will write into the job buffer
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
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_JOB_TYPE, job->type);

    // sequence number
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_SEQUENCE_NUMBER, state.mqtt_state.sequenz_number);

    // json end
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "}");

    // set job options
    pub_item.data_length = MQTT_DATA_BUFFER_SIZE - remaining;
    pub_item.qos_level = MQTT_QOS_LEVEL_0;
    pub_item.retain = MQTT_RETAIN_OFF;
    pub_item.is_last_will = 0;

    printf("publish job %d type %d \n\r", job->id, job->type);

    mqtt_service_publish(&pub_item);
}

/* publish pure informations of given job */
static void
publish_job_details(mqtt_publish_status_job_t *job)
{
    PRINTF("publish job details of %d\n\r", job->id);

    int remaining = MQTT_DATA_BUFFER_SIZE;

    pub_item.topic_length = snprintf(pub_item.topic, MQTT_META_BUFFER_SIZE,
        TOPIC_DETAILS_JOB, conf.mqtt_conf.client_id, job->id);

    buffer_ptr = pub_item.data;

    // json begin
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "{");

    // alert job config
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_CONFIG_JOBS JSON_JOB,
        job->type,
        job->topic,
        job->status,
        job->id,
        job->op,
        job->interval,
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

    //show_status_job(job_ptr);

    if(job_ptr->id < 0 || job_ptr->interval <= 0){
        return;
    }

    publish_job(job_ptr);

    if(!ctimer_expired(&(job_ptr->timer))){
        ctimer_stop(&(job_ptr->timer));
    }

    ctimer_set(&(job_ptr->timer), (job_ptr->interval * CLOCK_SECOND), status_job_callback, job_ptr);
}

/* parsing json to status job and add them to status-job-list */
static void
parse_job(char *job_as_json, uint16_t length)
{
    int len = strlen(job_as_json);
    int count = 0;
    int index = -1;

    if(len < 1 || len > MQTT_DATA_BUFFER_SIZE){
        PRINTF("Buffer is to small\n\r");
        return;
    }

    // create ja json parser
    struct jsonparse_state json;
    jsonparse_setup(&json, job_as_json, len);

    // initialize job buffer with default values
    snprintf(job_buffer.topic, MQTT_META_BUFFER_SIZE, DEFAULT_STATUS_JOB_TOPIC);
    job_buffer.id = -1;
    job_buffer.type = JOB_TYPE_STATUS;
    job_buffer.status = DEVICE_STATUS_ALL;
    job_buffer.interval = 30;
    job_buffer.time_from = 0;
    job_buffer.time_to = 0;
    job_buffer.op = COMPARE_OPERATOR_GREATE_EQUAL;
    job_buffer.value = 14000;
    job_buffer.time_elapsed = 0;

    // parse job json and write in job buffer
    while(JSON_HAS_NEXT(json)){
        jsonparse_next(&json);

        if(!(JSON_IS_KEY(json))){
            continue;
        }

        jsonparse_copy_value(&json, receive_buffer, MQTT_DATA_BUFFER_SIZE);

        if(strcasecmp(receive_buffer, JSON_KEY_ID) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            job_buffer.id = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TOPIC) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            jsonparse_copy_value(&json, job_buffer.topic, MQTT_META_BUFFER_SIZE);
        }else if(strcasecmp(receive_buffer, JSON_KEY_STATUS) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            job_buffer.status = (device_status_t)jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_INTERVAL) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            job_buffer.interval = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_COMPARE_OPERATOR) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            job_buffer.op = (compare_operator_t)jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TYPE) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            job_buffer.type = (job_type_t)jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_STATUS_VALUE) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            job_buffer.value = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TIME_FROM) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count)

            job_buffer.time_from = jsonparse_get_value_as_int(&json);
        }else if(strcasecmp(receive_buffer, JSON_KEY_TIME_TO) == 0){
            JSON_BACK_IF_NOT_VALUE(json, count);

            job_buffer.time_to = jsonparse_get_value_as_int(&json);
        }
    }

    if(count <= 0){
        PRINTF("count == 0\n\r");
        return;
    }

    // save job
    index = job_list_save(&job_buffer);

    if(index < 0){
        PRINTF("job konnte nicht hinzugefuegt werden\n\r");
        return;
    }

    printf("save job %d type %d\n\r", job_buffer.id, job_buffer.type);

    show_status_job(&(conf.job_conf.jobs[index]));

    // send data of job to the broker
    publish_job_details(&(conf.job_conf.jobs[index]));

    // if status job, begin the periodic event
    if(conf.job_conf.jobs[index].type == JOB_TYPE_STATUS)
        status_job_callback(&(conf.job_conf.jobs[index]));
}

/* parsing json to alert job and add them to status-job-list */
static void
parse_delete_request(char *request, uint16_t length)
{
    int len = strlen(request);
    int count = 0;
    int job_id = -1;

    if(len < 1 || len > MQTT_DATA_BUFFER_SIZE){
        PRINTF("Buffer is to small\n\r");
        return;
    }

    // create jsonparser
    struct jsonparse_state json;
    jsonparse_setup(&json, request, len);

    // get the job id to delete
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
        PRINTF("count == 0 or no job id was parsed\n\r");
        return;
    }

    printf("delete job %d\n\r", job_id);

    // delete job
    job_delete(job_id);
}


/* handle incoming publish messages */
static void
subscribe_handler(  const char *topic, uint16_t topic_length,
            const uint8_t *chunk, uint16_t chunk_length)
{
    PRINTF("topic %s (length:%u) chunk length : %u\n\r",
            topic, topic_length, chunk_length);

    // create job request
    if(strncasecmp(topic, TOPIC_ADD_JOB_PREFIX, strlen(TOPIC_ADD_JOB_PREFIX)) == 0){
        if( strlen(topic) == strlen(TOPIC_ADD_JOB_PREFIX)
            || strstr(topic, conf.mqtt_conf.client_id) != NULL)
        {
            PRINTF("create job status\n\r");
            parse_job((char *)chunk, chunk_length);
        }

    // delete job request
    }else if(strncasecmp(topic, TOPIC_DELETE_JOB_PREFIX, strlen(TOPIC_DELETE_JOB_PREFIX)) == 0){

        if( strlen(topic) == strlen(TOPIC_DELETE_JOB_PREFIX)
            || strstr(topic, conf.mqtt_conf.client_id) != NULL)
        {
            PRINTF("delete job status\n\r");
            parse_delete_request((char *)chunk, chunk_length);
        }

    }
}

/* check value for its border values
* @param sensorvalue : value to check
* @param job : job of this value
* @return 1 if alert, 0 if no alert, -1 if error */
static int
check_alert_value(int sensorvalue, mqtt_publish_status_job_t *job){
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

/* check current jobs for alert */
static void
check_for_alerts(){
    int i;

    // set the clock of your choice
    int clock = 0;

    // visit all jobs
    for(i = 0 ; i < MAX_JOBS ; i++){
        if(conf.job_conf.jobs[i].id == -1 ||
           conf.job_conf.jobs[i].type != JOB_TYPE_ALERT){
            continue;
        }

        if(conf.job_conf.jobs[i].time_from > clock || conf.job_conf.jobs[i].time_to < clock)
            continue;

        PRINTF("alert job index %d\n\r", i);

        alert_extend_duration = 0;
        alert_status_buffer = conf.job_conf.jobs[i].status;

        switch(alert_status_buffer){
            case DEVICE_STATUS_LIGHT:
                if(check_alert_value(GET_VALUE_LIGHT, &(conf.job_conf.jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            case DEVICE_STATUS_TEMPERATURE:
                if(check_alert_value(GET_VALUE_TEMPERATURE, &(conf.job_conf.jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            case DEVICE_STATUS_POWER:
                if(check_alert_value(GET_POWER, &(conf.job_conf.jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            case DEVICE_STATUS_UPTIME:
                if(check_alert_value(GET_UPTIME, &(conf.job_conf.jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            case DEVICE_STATUS_RSSI:
                if(check_alert_value(state.ping_state.rssi, &(conf.job_conf.jobs[i]))){
                    alert_extend_duration = 1;
                }
                break;
            default:
                break;
        }

        if(alert_extend_duration == 1){
            conf.job_conf.jobs[i].time_elapsed += conf.job_conf.alert_check_interval;
        }else{
            conf.job_conf.jobs[i].time_elapsed = 0;
        }

        if(conf.job_conf.jobs[i].time_elapsed >= (conf.job_conf.jobs[i].interval * CLOCK_SECOND)){
            PRINTF("alarm %d\n\r", conf.job_conf.jobs[i].id);
            publish_job(&(conf.job_conf.jobs[i]));
            conf.job_conf.jobs[i].time_elapsed = 0;
        }
    }

    if(ctimer_expired(&(state.mqtt_state.alert_timer))){
        ctimer_set(
            &state.mqtt_state.alert_timer,
            conf.job_conf.alert_check_interval,
            check_for_alerts, NULL);
    }
}

PROCESS(contiki_alert_process, "mqtt service process");
AUTOSTART_PROCESSES(&contiki_alert_process, &ping_service_process, &mqtt_service_process);

// entry point
PROCESS_THREAD(contiki_alert_process, ev, data)
{
    int i;

	PROCESS_BEGIN();

    if(init_config(&conf, &state) != 1)
    {
        PROCESS_EXIT();
    }

    update_config();

    // set callback for subscribe messages
    (state.mqtt_state.publish_handler) = &subscribe_handler;

    // initialize the services (ping for periodic ping, mqtt for mqtt api)
    ping_service_init(&(conf.ping_conf), &(state.ping_state));
    mqtt_service_init(&contiki_alert_process, &(conf.mqtt_conf), &(state.mqtt_state));

	while(1)
	{
		PROCESS_YIELD();

        if(!mqtt_service_is_connected()){
            is_subscribe = 0;
            continue;
        }

        //press select button to send a full status report
        if(ev == sensors_event && data == PUBLISH_TRIGGER)
        {
            PRINTF("publish by press button\n\r");
            publish_job(&default_status_job);
        }

        // press down button to delete the persistent saved config
        if(ev == sensors_event && data == REBOOT_TRIGGER){
            printf("delete config and restart\n\r");
            delete_config();
        }

        // first time we are here, subscribe the api topics
        // send send device and job informations
        if(!is_subscribe)
        {
            mqtt_service_subscribe(TOPIC_ADD_JOB, MQTT_QOS_LEVEL_1);
            PROCESS_WAIT_EVENT_UNTIL(ev == mqtt_event);
            mqtt_service_subscribe(TOPIC_DELETE_JOB, MQTT_QOS_LEVEL_1);

            PRINTF("subscribes done\n\r");
            is_subscribe = 1;

            ctimer_set(
                &(state.mqtt_state.alert_timer),
                conf.job_conf.alert_check_interval,
                check_for_alerts, NULL);

            PRINTF("send client active message\n\r");
            publish_client_active(0);

            // send job information and trigger first time all status jobs
            for(i = 0 ; i < MAX_JOBS ; i++){
                if(conf.job_conf.jobs[i].id < 0)
                    continue;

                if(conf.job_conf.jobs[i].type == JOB_TYPE_STATUS)
                    status_job_callback(&(conf.job_conf.jobs[i]));

                publish_job_details(&(conf.job_conf.jobs[i]));
            }
        }
	}

	PROCESS_END();
}

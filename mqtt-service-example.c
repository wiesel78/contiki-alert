#include "contiki.h"
#include "jsonparse.h"
#include "jsontree.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"

#include "dev/button-sensor.h"
#include "dev/als-sensor.h"


#include <stdio.h>
#include <stdarg.h>

#include "./device-utils.h"
#include "./ping-service.h"
#include "./mqtt-service.h"
#include "./config-service.h"
#include "./net-utils.h"
#include "./io-utils.h"

#define PUBLISH_SECONDS (CLOCK_SECOND * 10)

#define SUBSCRIBE_CREATE_JOB_ALERT "job/add/alert"
#define SUBSCRIBE_CREATE_JOB_STATUS "job/add/status"
#define DEFAULT_STATUS_JOB_TOPIC "status/new"

#define JSON_KEY_ID "id"
#define JSON_KEY_TOPIC "topic"
#define JSON_KEY_STATUS "status"
#define JSON_KEY_INTERVAL "interval"
#define JSON_KEY_TIME_FROM "timeFrom"
#define JSON_KEY_TIME_TO "timeTo"
#define JSON_KEY_SENSOR "sensor"
#define JSON_KEY_SENSOR_VALUE "sensorValue"
#define JSON_KEY_COMPARE_OPERATOR "compareOperator"

#define JSON_HAS_NEXT(json) ((json).pos < (json).len)
#define JSON_IS_ERROR(json) ((json).vtype == JSON_TYPE_ERROR)
#define JSON_IS_KEY(json) ((json).vtype == JSON_TYPE_PAIR_NAME)
#define JSON_IS_VALUE(json) ( \
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


client_config_t conf;
client_state_t state;

static int is_subscribe = 0;
static char receive_buffer[MQTT_DATA_BUFFER_SIZE];
static char *buffer_ptr;
static mqtt_publish_status_job_t status_job_buffer;
static publish_item_t pub_item;

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

/* publish a status-job */
static void 
publish_status(mqtt_publish_status_job_t *job)
{
    printf("begin publish status\n\r");
    
    int remaining = MQTT_DATA_BUFFER_SIZE;
    
    if(job == NULL){
        return;
    }
    
    pub_item.topic_length = strlen(job->topic);
    memcpy(pub_item.topic, job->topic, pub_item.topic_length);
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
        char ipv6_addr_str[64];
        memset(ipv6_addr_str, 0, sizeof(ipv6_addr_str));
        ipaddr_sprintf(ipv6_addr_str, sizeof(ipv6_addr_str), uip_ds6_defrt_choose());
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_IPV6, ipv6_addr_str);
    }
    
    // client-id
    if((job->status) & DEVICE_STATUS_CLIENT_ID){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_CLIENT_ID, conf.mqtt_conf.client_id);
    }
    
    // signal strength
    if((job->status) & DEVICE_STATUS_RSSI){
        buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_RSSI, state.ping_state.rssi);
    }
    
    // sequence number
    buffer_ptr = bcprintf(buffer_ptr, &remaining, JSON_STATUS_SEQUENCE_NUMBER, state.mqtt_state.sequenz_number);
    
    // json end
    buffer_ptr = bcprintf(buffer_ptr, &remaining, "}");
    
    pub_item.data_length = MQTT_DATA_BUFFER_SIZE - remaining;
    
    mqtt_service_publish(&pub_item);
}


static void
job_callback(void *ptr){
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
    
    ctimer_set(&(job_ptr->timer), (job_ptr->interval * CLOCK_SECOND), job_callback, job_ptr);
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
    
    job_callback(&(conf.mqtt_conf.status_jobs[index]));
}



static void 
subscribe(  const char *topic, uint16_t topic_length, 
            const uint8_t *chunk, uint16_t chunk_length)
{
    printf("blabla publish handler handle topic %s (length:%u) chunk length : %u\n\r", 
            topic, topic_length, chunk_length);
    
    if(strncasecmp(topic, SUBSCRIBE_CREATE_JOB_STATUS, strlen(SUBSCRIBE_CREATE_JOB_STATUS)) == 0){
        printf("create job status\n\r");
        
        parse_status_job((char *)chunk, chunk_length);
        
    }else if(strncasecmp(topic, SUBSCRIBE_CREATE_JOB_ALERT, strlen(SUBSCRIBE_CREATE_JOB_ALERT)) == 0){
        printf("create job alert\n\r");
        
        
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
    
    (state.mqtt_state.publish_handler) = &subscribe;
    
    ping_service_init(&(conf.ping_conf), &(state.ping_state));
    mqtt_service_init(&mqtt_service_test, &(conf.mqtt_conf), &(state.mqtt_state));
    
    
    
	while(1)
	{
		PROCESS_YIELD();
        
        ping_service_update(ev, data);
		mqtt_service_update(ev, data);
        
        
        
        if(mqtt_service_is_connected())
        {
            if(ev == sensors_event && data == PUBLISH_TRIGGER)
            {
                printf("publish by press button\n\r");
                publish_status(&default_status_job);
            }
            
            if(!is_subscribe)
            {
                mqtt_service_subscribe(SUBSCRIBE_CREATE_JOB_STATUS, MQTT_QOS_LEVEL_1);
                printf("first subscribe\n\r");
                
                PROCESS_WAIT_EVENT_UNTIL(ev == mqtt_event);
                
                mqtt_service_subscribe(SUBSCRIBE_CREATE_JOB_ALERT, MQTT_QOS_LEVEL_1);
                printf("second subscribe\n\r");
                
                is_subscribe = 1;
            }
        }
        else
        {
            is_subscribe = 0;
        }
	}
	
	PROCESS_END();
}


#include "contiki.h"


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

#define SUBSCRIBE_CREATE_JOB_ALERT "job/add/alert"
#define SUBSCRIBE_CREATE_JOB_STATUS "job/add/status"

client_config_t conf;
client_state_t state;

static int is_subscribe = 0;
static char app_buffer[MQTT_DATA_BUFFER_SIZE];
static char *buffer_ptr;
//static mqtt_publish_status_job_t status_job_list[MAX_STATUS_JOBS];

static mqtt_publish_status_job_t default_status_job = {
    .id = 0,
    .topic = "status/default",
    .status = DEVICE_STATUS_ALL,
    .interval = 30000,
    .time_from = 36000000,
    .time_to = 72000000
};

/* parsing json to status job and add them to status-job-list */


/* publish a status-job */
static void 
publish_status(mqtt_publish_status_job_t *job)
{
    int remaining = MQTT_DATA_BUFFER_SIZE;
    
    if(job == NULL){
        return;
    }
    
    buffer_ptr = app_buffer;
    
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
    
    mqtt_service_publish(job->topic, app_buffer);
}


static void 
subscribe(  const char *topic, uint16_t topic_length, 
            const uint8_t *chunk, uint16_t chunk_length)
{
    DBG("blabla publish handler handle topic %s (length:%u) chunk length : %u\n\r", 
            topic, topic_length, chunk_length);
    
    if(strncasecmp(topic, SUBSCRIBE_CREATE_JOB_STATUS, strlen(SUBSCRIBE_CREATE_JOB_STATUS))){
        DBG("create job status\n\r");
        
        
    }else if(strncasecmp(topic, SUBSCRIBE_CREATE_JOB_ALERT, strlen(SUBSCRIBE_CREATE_JOB_ALERT))){
        DBG("create job status\n\r");
        
        
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
                DBG("publish by press button\n\r");
                publish_status(&default_status_job);
            }
            
            if(!is_subscribe)
            {
                mqtt_service_subscribe(SUBSCRIBE_CREATE_JOB_STATUS, MQTT_QOS_LEVEL_1);
                DBG("first subscribe\n\r");
                
                PROCESS_WAIT_EVENT_UNTIL(ev == mqtt_event);
                
                mqtt_service_subscribe(SUBSCRIBE_CREATE_JOB_ALERT, MQTT_QOS_LEVEL_1);
                DBG("second subscribe\n\r");
                
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


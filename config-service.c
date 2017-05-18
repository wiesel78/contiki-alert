#include "contiki-conf.h"
#include "rpl/rpl-private.h"
#include "./ping-service.h"
#include "./config-service.h"
#include <stdio.h>


/* Broker IP */


client_config_t *app_conf;
client_state_t *app_state;

/* get the biggest status job id */
int status_job_max_id(void)
{
    int max = 0;
    
    for(int i = 0 ; i < MAX_STATUS_JOBS ; i++){
        if(app_conf->mqtt_conf.status_jobs[i].id > max){
            max = app_conf->mqtt_conf.status_jobs[i].id;
        }
    }
    
    return max;
}

/* -1 if job is not exists or index of job if job exists */
int status_job_exists(mqtt_publish_status_job_t *job)
{
    for(int i = 0 ; i < MAX_STATUS_JOBS ; i++){
        if(app_conf->mqtt_conf.status_jobs[i].id == job->id && job->id != -1){
            return i;
        }
    }
    
    return -1;
}

/* get the first element with id == -1 (empty element and place for new one)*/
int status_job_list_get_free_slot(void)
{
    for(int i = 0 ; i < MAX_STATUS_JOBS ; i++)
    {
        if(app_conf->mqtt_conf.status_jobs[i].id == -1){
            return i;
        }
    }
    
    return -1;
}

/* add new element (id == -1) or save element (id >= 0) */
int status_job_list_save(mqtt_publish_status_job_t *job)
{
    int index = status_job_exists(job);
    printf("save : index %d\n\r", index);
    index = index <= -1 ? status_job_list_get_free_slot() : index;
    
    printf("save : index %d\n\r", index);
    
    if(index >= 0){
        
        printf("save : memcpy\n\r");
        
        memcpy( &(app_conf->mqtt_conf.status_jobs[index]), 
                job, 
                sizeof(mqtt_publish_status_job_t));
                
        app_conf->mqtt_conf.status_jobs[index].id = status_job_max_id() + 1;
    }
    
    printf("index in list_save %d\n\r", index);
    
    return index;
}



/* initialize status job array */
void status_job_list_init(void)
{
    for(int i = 0 ; i < MAX_STATUS_JOBS ; i++)
    {
        app_conf->mqtt_conf.status_jobs[i].id = -1;
    }
}

/* Configuration */
int
init_config(client_config_t *config, client_state_t *state)
{
    app_conf = config;
    app_state = state;
    
    /* Populate configuration with default values */
    memset(app_conf, 0, sizeof(client_config_t));

    memcpy(app_conf->mqtt_conf.username, MQTT_USERNAME, strlen(MQTT_USERNAME));
    memcpy(app_conf->mqtt_conf.password, MQTT_PASSWORD, strlen(MQTT_PASSWORD));
    memcpy(app_conf->mqtt_conf.type_id, DEFAULT_TYPE_ID, strlen(DEFAULT_TYPE_ID));
    memcpy(app_conf->mqtt_conf.event_type_id, DEFAULT_EVENT_TYPE_ID, strlen(DEFAULT_EVENT_TYPE_ID));
    memcpy(app_conf->mqtt_conf.broker_ip, MQTT_BROKER_IP_ADDR, strlen(MQTT_BROKER_IP_ADDR));
    memcpy(app_conf->mqtt_conf.cmd_type, DEFAULT_SUBSCRIBE_CMD_TYPE, 1);
    
    app_conf->mqtt_conf.broker_port = DEFAULT_BROKER_PORT;
    app_conf->ping_conf.interval = DEFAULT_PING_INTERVAL;
    
    status_job_list_init();
    
    memset(app_state, 0, sizeof(client_state_t));
    app_state->ping_state.rssi = 0;
    app_state->mqtt_state.publish_handler = NULL;

    return 1;
}

/* Config */
/* Build client id */
int
construct_client_id(void)
{
  int len = snprintf(app_conf->mqtt_conf.client_id, MQTT_DATA_BUFFER_SIZE, 
                     "d:%s:%02x%02x%02x%02x%02x%02x",
                     app_conf->mqtt_conf.type_id,
                     linkaddr_node_addr.u8[0], linkaddr_node_addr.u8[1],
                     linkaddr_node_addr.u8[2], linkaddr_node_addr.u8[5],
                     linkaddr_node_addr.u8[6], linkaddr_node_addr.u8[7]);

  /* len < 0: Error. Len >= MQTT_DATA_BUFFER_SIZE: Buffer too small */
  if(len < 0 || len >= MQTT_DATA_BUFFER_SIZE) {
    printf("Client ID: %d, Buffer %d\n", len, MQTT_DATA_BUFFER_SIZE);
    return 0;
  }

  return 1;
}

/* reset the mqtt config */
void
update_mqtt_config(void)
{
    /* Fatal error if Client ID larger than the buffer */
    if(construct_client_id() == 0) {
        app_state->mqtt_state.state = MQTT_SERVICE_STATE_CONFIG_ERROR;
        return;
    }
    
    app_state->mqtt_state.sequenz_number = 0;
    app_state->mqtt_state.state = MQTT_SERVICE_STATE_INIT;
    
    etimer_set(&(app_state->mqtt_state.periodic_timer),0);
        
    return;
}

/* reset the ping config */
void
update_ping_config(void)
{    
    app_conf->ping_conf.interval = DEFAULT_PING_INTERVAL;
    app_state->ping_state.rssi = 0x8000000;
        
    return;
}

/* reset all configs */
void 
update_config(void)
{
    update_mqtt_config();
    update_ping_config();
}

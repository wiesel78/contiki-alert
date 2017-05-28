#include "contiki-conf.h"
#include "cfs/cfs.h"
#include "rpl/rpl-private.h"
#include "jsonparse.h"
#include "jsontree.h"
#include "./ping-service.h"
#include "./config-service.h"
#include "./io-utils.h"
#include <stdio.h>


client_config_t *app_conf;
client_state_t *app_state;

// static char config_buffer[CONFIG_BUFFER_SIZE];
// static char config_receive_buffer[MQTT_DATA_BUFFER_SIZE];
// static char *config_buffer_ptr;
static ini_state_t config_state;
static mqtt_publish_alert_job_t alert_job_buffer;
static mqtt_publish_status_job_t status_job_buffer;
static uint8_t can_save = 1;

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
    index = index <= -1 ? status_job_list_get_free_slot() : index;

    if(index >= 0){
        memcpy( &(app_conf->mqtt_conf.status_jobs[index]),
                job,
                sizeof(mqtt_publish_status_job_t));

        if(app_conf->mqtt_conf.status_jobs[index].id < 0)
            app_conf->mqtt_conf.status_jobs[index].id = status_job_max_id() + 1;
    }

    printf("index in list_save %d\n\r", index);

    save_config();

    return index;
}

/* delete job by id */
void status_job_delete(int id)
{
    mqtt_publish_status_job_t *job = NULL;

    for(int i = 0 ; i < MAX_STATUS_JOBS ; i++){
        if(app_conf->mqtt_conf.status_jobs[i].id == id && job->id != -1){
            job = &(app_conf->mqtt_conf.status_jobs[i]);

            if(!ctimer_expired(&(job->timer))){
                ctimer_stop(&(job->timer));
            }

            memset(job->topic, 0, MQTT_META_BUFFER_SIZE);
            job->id = -1;
            job->interval = 30000;
            job->status = DEVICE_STATUS_ALL;
            job->time_from = 0;
            job->time_to = 0;

            printf("job deleted by id %d\n\r", id);
        }
    }

    save_config();
}



/* initialize status job array */
void status_job_list_init(void)
{
    for(int i = 0 ; i < MAX_STATUS_JOBS ; i++)
    {
        app_conf->mqtt_conf.status_jobs[i].id = -1;
    }
}

/* get the biggest status job id */
int alert_job_max_id(void)
{
    int max = 0;

    for(int i = 0 ; i < MAX_STATUS_JOBS ; i++){
        if(app_conf->mqtt_conf.alert_jobs[i].id > max){
            max = app_conf->mqtt_conf.alert_jobs[i].id;
        }
    }

    return max;
}

/* -1 if job is not exists or index of job if job exists */
int alert_job_exists(mqtt_publish_alert_job_t *job)
{
    for(int i = 0 ; i < MAX_ALERT_JOBS ; i++){
        if(app_conf->mqtt_conf.alert_jobs[i].id == job->id && job->id != -1){
            return i;
        }
    }

    return -1;
}

/* get the first element with id == -1 (empty element and place for new one)*/
int alert_job_list_get_free_slot(void)
{
    for(int i = 0 ; i < MAX_ALERT_JOBS ; i++)
    {
        if(app_conf->mqtt_conf.alert_jobs[i].id == -1){
            return i;
        }
    }

    return -1;
}


/* add new element (id == -1) or save element (id >= 0) */
int alert_job_list_save(mqtt_publish_alert_job_t *job)
{
    int index = alert_job_exists(job);
    index = index <= -1 ? alert_job_list_get_free_slot() : index;

    if(index >= 0){
        memcpy( &(app_conf->mqtt_conf.alert_jobs[index]),
                job,
                sizeof(mqtt_publish_alert_job_t));

        if(app_conf->mqtt_conf.alert_jobs[index].id < 0)
            app_conf->mqtt_conf.alert_jobs[index].id = alert_job_max_id() + 1;

        app_conf->mqtt_conf.alert_jobs[index].time_elapsed = 0;
    }

    printf("index in list_save %d\n\r", index);

    save_config();

    return index;
}



/* delete job by id */
void alert_job_delete(int id)
{
    mqtt_publish_alert_job_t *job = NULL;

    for(int i = 0 ; i < MAX_ALERT_JOBS ; i++){
        if(app_conf->mqtt_conf.alert_jobs[i].id == id && job->id != -1){
            job = &(app_conf->mqtt_conf.alert_jobs[i]);

            if(!ctimer_expired(&(job->timer))){
                ctimer_stop(&(job->timer));
            }

            memset(job->topic, 0, MQTT_META_BUFFER_SIZE);
            job->id = -1;
            job->status = DEVICE_STATUS_LIGHT;
            job->time_from = 0;
            job->time_to = 0;
            job->op = COMPARE_OPERATOR_GREATE_EQUAL;
            job->duration = 30;
            job->time_elapsed = 0;
            job->value = 0;

            printf("job deleted by id %d\n\r", id);
        }
    }

    save_config();
}

/* initialize status job array */
void alert_job_list_init(void)
{
    for(int i = 0 ; i < MAX_ALERT_JOBS ; i++)
    {
        app_conf->mqtt_conf.alert_jobs[i].id = -1;
        app_conf->mqtt_conf.alert_jobs[i].time_elapsed = 0;
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
    app_conf->mqtt_conf.alert_check_interval = ALERT_CHECK_INTERVAL;
    app_conf->ping_conf.interval = DEFAULT_PING_INTERVAL;

    read_config();

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
                     "%s%02x%02x%02x%02x%02x%02x",
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

void
save_config(){

    if(can_save == 0)
        return;

    mqtt_publish_status_job_t *status_job = NULL;
    mqtt_publish_alert_job_t *alert_job = NULL;

    ini_open(&config_state, CONFIG_FILE_PATH);

    ini_write_group(&config_state, JSON_CONFIG_KEY_PING);
    ini_write_int(&config_state, JSON_CONFIG_KEY_INTERVAL, app_conf->ping_conf.interval);

    ini_write_group(&config_state, JSON_CONFIG_KEY_MQTT);
    ini_write_string(&config_state, JSON_CONFIG_KEY_USERNAME, app_conf->mqtt_conf.username);
    ini_write_string(&config_state, JSON_CONFIG_KEY_PASSWORD, app_conf->mqtt_conf.password);
    ini_write_string(&config_state, JSON_CONFIG_KEY_CLIENT_ID, app_conf->mqtt_conf.client_id);
    ini_write_string(&config_state, JSON_CONFIG_KEY_TYPE_ID, app_conf->mqtt_conf.type_id);
    ini_write_string(&config_state, JSON_CONFIG_KEY_EVENT_TYPE_ID, app_conf->mqtt_conf.event_type_id);
    ini_write_string(&config_state, JSON_CONFIG_KEY_BROKER_IP, app_conf->mqtt_conf.broker_ip);
    ini_write_int(&config_state, JSON_CONFIG_KEY_BROKER_PORT, app_conf->mqtt_conf.broker_port);
    ini_write_string(&config_state, JSON_CONFIG_KEY_CMD_TYPE, app_conf->mqtt_conf.cmd_type);
    ini_write_int(&config_state, JSON_CONFIG_KEY_ALERT_CHECK_INTERVAL, app_conf->mqtt_conf.alert_check_interval);

    for(int i = 0 ; i < MAX_STATUS_JOBS ; i++){
        if(app_conf->mqtt_conf.status_jobs[i].id == -1)
            continue;

        status_job = &(app_conf->mqtt_conf.status_jobs[i]);

        ini_write_group(&config_state, JSON_CONFIG_JOB_KEY_STATUS_JOB);
        ini_write_string(&config_state, JSON_CONFIG_JOB_KEY_TOPIC, status_job->topic);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_STATUS, status_job->status);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_ID, status_job->id);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_INTERVAL, status_job->interval);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TIME_FROM, status_job->time_from);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TIME_TO, status_job->time_to);

    }

    for(int i = 0 ; i < MAX_ALERT_JOBS ; i++){
        if(app_conf->mqtt_conf.alert_jobs[i].id == -1)
            continue;

        alert_job = &(app_conf->mqtt_conf.alert_jobs[i]);

        ini_write_group(&config_state, JSON_CONFIG_JOB_KEY_ALERT_JOB);
        ini_write_string(&config_state, JSON_CONFIG_JOB_KEY_TOPIC, alert_job->topic);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_STATUS, alert_job->status);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_OPERATOR, alert_job->op);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_ID, alert_job->id);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_DURATION, alert_job->duration);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TIME_ELAPSED, alert_job->time_elapsed);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_STATUS_VALUE, alert_job->value);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TIME_FROM, alert_job->time_from);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TIME_TO, alert_job->time_to);

    }

    ini_close(&config_state);
}

void
read_config(){

    if(!ini_open(&config_state, CONFIG_FILE_PATH)){
        return;
    }

    can_save = 0;

    status_job_list_init();
    alert_job_list_init();

    while(ini_read_next(&config_state) != INI_TYPE_EOF){

        if(config_state.type != INI_TYPE_KEYVALUE){
            continue;
        }

        if(strcmp(config_state.group, INI_VALUE_GROUP JSON_CONFIG_KEY_MQTT) == 0){
            if(strcmp(config_state.key, JSON_CONFIG_KEY_USERNAME) == 0){
                snprintf(app_conf->mqtt_conf.username, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_PASSWORD) == 0){
                snprintf(app_conf->mqtt_conf.password, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_CLIENT_ID) == 0){
                snprintf(app_conf->mqtt_conf.client_id, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_TYPE_ID) == 0){
                snprintf(app_conf->mqtt_conf.type_id, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_EVENT_TYPE_ID) == 0){
                snprintf(app_conf->mqtt_conf.event_type_id, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_BROKER_IP) == 0){
                snprintf(app_conf->mqtt_conf.broker_ip, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_BROKER_PORT) == 0){
                app_conf->mqtt_conf.broker_port = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_CMD_TYPE) == 0){
                snprintf(app_conf->mqtt_conf.cmd_type, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_ALERT_CHECK_INTERVAL) == 0){
                app_conf->mqtt_conf.alert_check_interval = atoi(config_state.data);
            }

        }else if(strcmp(config_state.group, INI_VALUE_GROUP JSON_CONFIG_KEY_PING) == 0){
            if(strcmp(config_state.key, JSON_CONFIG_KEY_INTERVAL) == 0){
                app_conf->ping_conf.interval = atoi(config_state.data);
            }

        }else if(strcmp(config_state.group, INI_VALUE_GROUP JSON_CONFIG_JOB_KEY_ALERT_JOB) == 0){

            if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TOPIC) == 0){
                snprintf(alert_job_buffer.topic, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_STATUS) == 0){
                alert_job_buffer.status = (device_status_t)atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_OPERATOR) == 0){
                alert_job_buffer.op = (compare_operator_t)atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_ID) == 0){
                alert_job_buffer.id = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_DURATION) == 0){
                alert_job_buffer.duration = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_STATUS_VALUE) == 0){
                alert_job_buffer.value = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TIME_FROM) == 0){
                alert_job_buffer.time_from = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TIME_TO) == 0){
                alert_job_buffer.time_to = atoi(config_state.data);

                alert_job_list_save(&alert_job_buffer);
            }

        }else if(strcmp(config_state.group, INI_VALUE_GROUP JSON_CONFIG_JOB_KEY_STATUS_JOB) == 0){
            if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TOPIC) == 0){
                snprintf(status_job_buffer.topic, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_STATUS) == 0){
                status_job_buffer.status = (device_status_t)atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_ID) == 0){
                status_job_buffer.id = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_INTERVAL) == 0){
                status_job_buffer.interval = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TIME_FROM) == 0){
                status_job_buffer.time_from = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TIME_TO) == 0){
                status_job_buffer.time_to = atoi(config_state.data);

                status_job_list_save(&status_job_buffer);
            }

        }

    }

    ini_close(&config_state);
    can_save = 1;
}

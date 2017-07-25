#include "contiki-conf.h"
#include "cfs/cfs.h"
#include "dev/watchdog.h"
#include "rpl/rpl-private.h"
#include "jsonparse.h"
#include "jsontree.h"
#include "ping-service.h"
#include "./config.h"
#include "./io-utils.h"
#include <stdio.h>
#include "./debug.h"


client_config_t *app_conf;
client_state_t *app_state;

static ini_state_t config_state;
static mqtt_publish_status_job_t job_buffer;
static uint8_t can_save = 1;


/* -1 if job is not exists or index of job if job exists */
int job_exists(mqtt_publish_status_job_t *job)
{
    int i;

    for(i = 0 ; i < MAX_STATUS_JOBS ; i++){
        if(app_conf->mqtt_conf.jobs[i].id == job->id && job->id != -1){
            return i;
        }
    }

    return -1;
}

/* get the first element with id == -1 (empty element and place for new one)*/
int job_list_get_free_slot(void)
{
    int i;

    for(i = 0 ; i < MAX_STATUS_JOBS ; i++)
    {
        if(app_conf->mqtt_conf.jobs[i].id == -1){
            return i;
        }
    }

    return -1;
}

/* add new element (id == -1) or save element (id >= 0) */
int job_list_save(mqtt_publish_status_job_t *job)
{
    int index = job_exists(job);
    index = index <= -1 ? job_list_get_free_slot() : index;

    if(index >= 0){
        if(!ctimer_expired(&(app_conf->mqtt_conf.jobs[index].timer))){
            ctimer_stop(&(app_conf->mqtt_conf.jobs[index].timer));
        }

        memcpy( &(app_conf->mqtt_conf.jobs[index]),
                job,
                sizeof(mqtt_publish_status_job_t));

        if(app_conf->mqtt_conf.jobs[index].id < 0)
            app_conf->mqtt_conf.jobs[index].id = ++(app_conf->mqtt_conf.job_id);
    }

    PRINTF("index in list_save %d\n\r", index);

    save_config();

    return index;
}

/* delete job by id */
void job_delete(int id)
{
    int i;
    mqtt_publish_status_job_t *job = NULL;

    for(i = 0 ; i < MAX_STATUS_JOBS ; i++){
        if(app_conf->mqtt_conf.jobs[i].id == id && job->id != -1){
            job = &(app_conf->mqtt_conf.jobs[i]);

            if(!ctimer_expired(&(job->timer))){
                ctimer_stop(&(job->timer));
            }

            memset(job->topic, 0, MQTT_META_BUFFER_SIZE);
            job->id = -1;
            job->interval = 30000;
            job->status = DEVICE_STATUS_ALL;
            job->op = COMPARE_OPERATOR_GREATE_EQUAL;
            job->time_elapsed = 0;
            job->value = 0;
            job->time_from = 0;
            job->time_to = 0;

            PRINTF("job deleted by id %d\n\r", id);
        }
    }

    save_config();
}



/* initialize status job array */
void job_list_init(void)
{
    int i;

    for(i = 0 ; i < MAX_STATUS_JOBS ; i++)
    {
        app_conf->mqtt_conf.jobs[i].id = -1;
        app_conf->mqtt_conf.jobs[i].time_elapsed = 0;
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
    app_conf->mqtt_conf.job_id = 1;
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
    PRINTF("Client ID: %d, Buffer %d\n", len, MQTT_DATA_BUFFER_SIZE);
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

    return;
}

/* reset the ping config */
void
update_ping_config(void)
{
    app_conf->ping_conf.interval = DEFAULT_PING_INTERVAL;
    app_state->ping_state.rssi = 0; //0x8000000;

    return;
}

/* reset all configs */
void
update_config(void)
{
    update_mqtt_config();
    update_ping_config();
}

/* Save current config data */
void
save_config()
{
    int i;

    // check config lock
    if(can_save == 0)
        return;

    // initialize job buffer
    mqtt_publish_status_job_t *job = NULL;

    // open config file
    ini_open(&config_state, CONFIG_FILE_PATH);

    // write ping group
    ini_write_group(&config_state, JSON_CONFIG_KEY_PING);
    ini_write_int(&config_state, JSON_CONFIG_KEY_INTERVAL, app_conf->ping_conf.interval);

    // write mqtt group
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
    ini_write_int(&config_state, JSON_CONFIG_KEY_CURRENT_JOB_ID, app_conf->mqtt_conf.job_id);

    for(i = 0 ; i < MAX_STATUS_JOBS ; i++){
        if(app_conf->mqtt_conf.jobs[i].id == -1)
            continue;

        job = &(app_conf->mqtt_conf.jobs[i]);

        // write job group
        ini_write_group(&config_state, JSON_CONFIG_JOB_KEY_STATUS_JOB);
        ini_write_string(&config_state, JSON_CONFIG_JOB_KEY_TOPIC, job->topic);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_STATUS, job->status);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_ID, job->id);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TYPE, job->type);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_INTERVAL, job->interval);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_OPERATOR, job->op);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TIME_ELAPSED, job->time_elapsed);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_STATUS_VALUE, job->value);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TIME_FROM, job->time_from);
        ini_write_int(&config_state, JSON_CONFIG_JOB_KEY_TIME_TO, job->time_to);

    }

    ini_close(&config_state);
}

/* read and parse the config file and write its data into app_conf */
void
read_config(){

    // open the ini file (config file)
    if(!ini_open(&config_state, CONFIG_FILE_PATH)){
        return;
    }

    // block config file
    can_save = 0;

    // initialize the job list with dump-zero-jobs ...
    job_list_init();

    // read every line in config file and write its data part into its conf part
    while(ini_read_next(&config_state) != INI_TYPE_EOF){

        // visit only KEYVALUE types
        if(config_state.type != INI_TYPE_KEYVALUE){
            continue;
        }

        // Group MQTT
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

            }else if(strcmp(config_state.key, JSON_CONFIG_KEY_CURRENT_JOB_ID) == 0){
                app_conf->mqtt_conf.job_id = atoi(config_state.data);
            }


        // Group PING
        }else if(strcmp(config_state.group, INI_VALUE_GROUP JSON_CONFIG_KEY_PING) == 0){
            if(strcmp(config_state.key, JSON_CONFIG_KEY_INTERVAL) == 0){
                app_conf->ping_conf.interval = atoi(config_state.data);
            }

        // Group Jobs
        }else if(strcmp(config_state.group, INI_VALUE_GROUP JSON_CONFIG_JOB_KEY_STATUS_JOB) == 0){
            if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TOPIC) == 0){
                snprintf(job_buffer.topic, INI_DATA_SIZE, config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_STATUS) == 0){
                job_buffer.status = (device_status_t)atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_ID) == 0){
                job_buffer.id = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_OPERATOR) == 0){
                job_buffer.op = (compare_operator_t)atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_STATUS_VALUE) == 0){
                job_buffer.value = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TYPE) == 0){
                job_buffer.op = (job_type_t)atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_INTERVAL) == 0){
                job_buffer.interval = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TIME_FROM) == 0){
                job_buffer.time_from = atoi(config_state.data);

            }else if(strcmp(config_state.key, JSON_CONFIG_JOB_KEY_TIME_TO) == 0){
                job_buffer.time_to = atoi(config_state.data);

                job_list_save(&job_buffer);
            }

        }

    }

    // close config file
    ini_close(&config_state);

    // get free
    can_save = 1;
}

/* delete the config file */
void
delete_config(){
    cfs_remove(CONFIG_FILE_PATH);
    watchdog_reboot();
}

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

// static char config_buffer[MQTT_DATA_BUFFER_SIZE];
// static char config_receive_buffer[MQTT_DATA_BUFFER_SIZE];
// static char *config_buffer_ptr;

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

    // save_config();

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

    // save_config();
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
        if(app_conf->mqtt_conf.status_jobs[i].id > max){
            max = app_conf->mqtt_conf.status_jobs[i].id;
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
    printf("save : index %d\n\r", index);
    index = index <= -1 ? alert_job_list_get_free_slot() : index;

    printf("save : index %d\n\r", index);

    if(index >= 0){

        printf("save : memcpy\n\r");

        memcpy( &(app_conf->mqtt_conf.alert_jobs[index]),
                job,
                sizeof(mqtt_publish_alert_job_t));

        app_conf->mqtt_conf.alert_jobs[index].id = alert_job_max_id() + 1;
    }

    printf("index in list_save %d\n\r", index);

    // save_config();

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

    // save_config();
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

    status_job_list_init();
    alert_job_list_init();

    // read_config();

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

// void
// save_config(){
//
//     printf("save config\n\r");
//     int fd;
//     int remaining = MQTT_DATA_BUFFER_SIZE;
//     mqtt_publish_status_job_t *status_job = NULL;
//     mqtt_publish_alert_job_t *alert_job = NULL;
//
//     fd = cfs_open(CONFIG_FILE_PATH, CFS_READ | CFS_WRITE);
//     printf("fd = %d\n\r", fd);
//     if(fd < 0)
//         return;
//
//     printf("save config file is open\n\r");
//     config_buffer_ptr = config_buffer;
//     // Config
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, "{");
//
//     // Ping Config
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,   "\"%s\":{", JSON_CONFIG_KEY_PING);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_PING_INTERVAL, app_conf->ping_conf.interval);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,   "},");
//
//     // MQTT Config
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,   "\"%s\":{", JSON_CONFIG_KEY_MQTT);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_USERNAME, app_conf->mqtt_conf.username);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_PASSWORD, app_conf->mqtt_conf.password);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_CLIENT_ID, app_conf->mqtt_conf.client_id);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_TYPE_ID, app_conf->mqtt_conf.type_id);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_EVENT_TYPE_ID, app_conf->mqtt_conf.event_type_id);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_BROKER_IP, app_conf->mqtt_conf.broker_ip);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_BROKER_PORT, app_conf->mqtt_conf.broker_port);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_CMD_TYPE, app_conf->mqtt_conf.cmd_type);
//     config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining,     JSON_CONFIG_ALERT_CHECK_INTERVAL, app_conf->mqtt_conf.alert_check_interval);
//
//     WRITE_AND_RESET(fd, config_buffer, config_buffer_ptr, remaining);
//
//     // Status Jobs
//     bcprintf(config_buffer_ptr, &remaining,     "\"%s\":[", JSON_CONFIG_JOB_KEY_STATUS_JOBS);
//
//     for(int i = 0 ; i < MAX_STATUS_JOBS ; i++){
//         if(app_conf->mqtt_conf.status_jobs[i].id != -1){
//             if(status_job != NULL){
//                 config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, ",");
//             }
//
//             status_job = &(app_conf->mqtt_conf.status_jobs[i]);
//
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, "{");
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_TOPIC, status_job->topic);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_STATUS, status_job->status);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_ID, status_job->id);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_INTERVAL, status_job->interval);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_TIME_FROM, status_job->time_from);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_TIME_TO, status_job->time_to);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, "}");
//
//             WRITE_AND_RESET(fd, config_buffer, config_buffer_ptr, remaining);
//         }
//     }
//
//     bcprintf(config_buffer_ptr, &remaining,     "],");
//
//     // Alert jobs
//     bcprintf(config_buffer_ptr, &remaining,     "\"%s\":[", JSON_CONFIG_JOB_KEY_ALERT_JOBS);
//
//     for(int i = 0 ; i < MAX_ALERT_JOBS ; i++){
//         if(app_conf->mqtt_conf.alert_jobs[i].id != -1){
//             if(alert_job != NULL){
//                 config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, ",");
//             }
//
//             alert_job = &(app_conf->mqtt_conf.alert_jobs[i]);
//
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, "{");
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_TOPIC, alert_job->topic);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_STATUS, alert_job->status);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_OPERATOR, alert_job->op);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_ID, alert_job->id);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_DURATION, alert_job->duration);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_TIME_ELAPSED, alert_job->time_elapsed);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_STATUS_VALUE, alert_job->value);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_TIME_FROM, alert_job->time_from);
//             config_buffer_ptr = bcprintf(config_buffer_ptr, &remaining, JSON_CONFIG_JOB_TIME_TO, alert_job->time_to);
//             config_buffer_ptr = (char*)bcprintf(config_buffer_ptr, &remaining, "}");
//
//             WRITE_AND_RESET(fd, config_buffer, config_buffer_ptr, remaining);
//         }
//     }
//
//     bcprintf(config_buffer_ptr, &remaining,     "]");
//
//     // End Mqtt Config
//     bcprintf(config_buffer_ptr, &remaining,   "}");
//
//     // End Config
//     bcprintf(config_buffer_ptr, &remaining, "}\0");
//
//     WRITE_AND_RESET(fd, config_buffer, config_buffer_ptr, remaining);
//     cfs_close(fd);
//
// }
//
// void
// read_config(){
//
//     printf("read_config\n\r");
//
//     int fd;
//     int len = 0;
//
//     fd = cfs_open(CONFIG_FILE_PATH, CFS_READ);
//     printf("fd = %d\n\r", fd);
//     if(fd < 0){
//         return;
//     }
//
//     printf("file is open\n\r");
//     len = cfs_read(fd, config_buffer, MQTT_DATA_BUFFER_SIZE);
//     if(len <= 0){
//         return;
//     }
//
//     struct jsonparse_state json;
//     jsonparse_setup(&json, config_buffer, len);
//
//
//     printf("json parse\n\r");
//     while(JSON_HAS_NEXT(json)){
//         jsonparse_next(&json);
//
//         printf("pos %d, len %d, depth %d, vstart %d, vlen %d, vtype %c, error %c \n\r",
//             json.pos, json.len, json.depth, json.vstart, json.vlen, json.vtype, json.error);
//
//         if(!(JSON_IS_KEY(json))){
//             continue;
//         }
//
//         jsonparse_copy_value(&json, config_receive_buffer, MQTT_DATA_BUFFER_SIZE);
//
//         printf("value %s\n\r", config_receive_buffer);
//
//         // if(strcasecmp(receive_buffer, JSON_CONFIG_KEY_MQTT) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count)
//         //     jsonparse_copy_value(&json, app_conf->mqtt_conf.username, MQTT_META_BUFFER_SIZE);
//         // }
//         //
//         // if(strcasecmp(receive_buffer, JSON_CONFIG_USERNAME) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count)
//         //     jsonparse_copy_value(&json, app_conf->mqtt_conf.username, MQTT_META_BUFFER_SIZE);
//         // }else if(strcasecmp(receive_buffer, JSON_CONFIG_PASSWORD) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count)
//         //     jsonparse_copy_value(&json, app_conf->mqtt_conf.password, MQTT_META_BUFFER_SIZE);
//         // }else if(strcasecmp(receive_buffer, JSON_CONFIG_CLIENT_ID) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count)
//         //     jsonparse_copy_value(&json, app_conf->mqtt_conf.password, MQTT_META_BUFFER_SIZE);
//         // }else
//
//
//
//             // char username[CONFIG_USERNAME_LEN];
//             // char password[CONFIG_PASSWORD_LEN];
//             // char client_id[MQTT_META_BUFFER_SIZE];
//             // char type_id[CONFIG_TYPE_ID_LEN];
//             // char event_type_id[CONFIG_EVENT_TYPE_ID_LEN];
//             // char broker_ip[CONFIG_IP_ADDR_STR_LEN];
//             // char cmd_type[CONFIG_CMD_TYPE_LEN];
//             // uint16_t broker_port;
//             // int alert_check_interval;
//             // mqtt_publish_status_job_t status_jobs[MAX_STATUS_JOBS];
//             // mqtt_publish_alert_job_t alert_jobs[MAX_ALERT_JOBS];
//
//
//         // if(strcasecmp(receive_buffer, JSON_KEY_TOPIC) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count)
//         //
//         //     jsonparse_copy_value(&json, status_job_buffer.topic, MQTT_META_BUFFER_SIZE);
//         // }else if(strcasecmp(receive_buffer, JSON_KEY_STATUS) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count)
//         //
//         //     status_job_buffer.status = (device_status_t)jsonparse_get_value_as_int(&json);
//         // }else if(strcasecmp(receive_buffer, JSON_KEY_INTERVAL) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count)
//         //
//         //     status_job_buffer.interval = jsonparse_get_value_as_int(&json);
//         // }else if(strcasecmp(receive_buffer, JSON_KEY_TIME_FROM) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count)
//         //
//         //     status_job_buffer.time_from = jsonparse_get_value_as_int(&json);
//         // }else if(strcasecmp(receive_buffer, JSON_KEY_TIME_TO) == 0){
//         //     JSON_BACK_IF_NOT_VALUE(json, count);
//         //
//         //     status_job_buffer.time_to = jsonparse_get_value_as_int(&json);
//         // }
//     }
//
//     cfs_close(fd);
// }

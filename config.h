#ifndef CONFIG_SERVICE_H_
#define CONFIG_SERVICE_H_

#include "contiki-conf.h"
#include "ping-service.h"
#include "mqtt-service.h"

#define CONFIG_BUFFER_SIZE 100
#define CONFIG_FILE_PATH "conf.ini"


#define GET_UPTIME (clock_seconds())

/* possible op values in a alert job */
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

/* specify flags for different values of sensor node */
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


/* status jobs are periodic send the device status
 * alert jobs send alert message if a sensor value is pass a border value
 * for specific time */
typedef enum {
    JOB_TYPE_STATUS     = 1 << 0,
    JOB_TYPE_ALERT      = 1 << 1,
} job_type_t;

#define WRITE_AND_RESET(fd, buf, buf_ptr, remain) { \
    cfs_write((fd), (buf), MQTT_DATA_BUFFER_SIZE - (remain)); \
    (buf_ptr) = (buf); \
    (remain) = MQTT_DATA_BUFFER_SIZE; \
}


/* specific job type to send a status or alert message
 * @param topic : MQTT Topic string to address a publish job
 * @param status : wich kind of sensor value you will send or observe
 * @param type : type=1 - status job send periodically sensor node values
 *               type=2 - alert job observe a sensor value and publish a alert
 *                        if this value are to long greater or lower as the
 *                        value in this struct
 * @param op : only type=2, define the compare operation in alert jobs
 *             if you will observe light and send a alert then
 *             light value greater 10000, the op value is 2 and mean "greater"
 * @param id : unique identifier for this job in job queue. -1 => autoincrement
 * @param interval : time between to status jobs or time to observe a value
 *                   before send a alert message
 * @param time_from : clock time from when this job is active
 * @param time_to : clock time from when this job is inactive
 * @param time_elapsed : only type = 2, this value describes how long a value
 *                       has already been observe without the alarm being struck
 * @param value : only type = 2, border value of a alarm job
 * @param timer : only type = 1, interval timer
 *
 * @example status job for all values every 30 seconds :
 *  {
 *      .id = -1,
 *      .topic = "status/default",
 *      .status = DEVICE_STATUS_ALL,
 *      .interval = 30000,
 *      .type = JOB_TYPE_STATUS
 *  };
 *
 * @example alert job - when light is greater then 10000 for 10 minutes :
 *  {
 *      .id = -1,
 *      .topic = "alert/light",
 *      .status = DEVICE_STATUS_LIGHT,
 *      .interval = 10*60,
 *      .type = JOB_TYPE_ALERT,
 *      .op = COMPARE_OPERATOR_GREATER,
 *      .value = 10000
 *  };
 */
typedef struct mqtt_publish_status_job {
    char topic[MQTT_META_BUFFER_SIZE];
    device_status_t status;
    job_type_t type;
    compare_operator_t op;
    int id;
    int interval;
    int time_from;
    int time_to;
    int time_elapsed;
    int value;
    struct ctimer timer;
} mqtt_publish_status_job_t;

typedef struct job_config {
    mqtt_publish_status_job_t jobs[MAX_JOBS];
    int alert_check_interval;
    int job_id;
} job_config_t;

typedef struct client_config {
    mqtt_client_config_t mqtt_conf;
    ping_client_config_t ping_conf;
    job_config_t job_conf;
} client_config_t;

typedef struct client_state {
    mqtt_service_state_t mqtt_state;
    ping_service_state_t ping_state;
} client_state_t;

/* initialize config instance */
extern int init_config(client_config_t *config, client_state_t *state);
extern void update_mqtt_config(void);
extern void update_ping_config(void);
extern void update_config(void);

/* delete a job by its id */
extern void job_delete(int id);

/* check for job in job list
* @param job : job instance that will be find in list
* @return 1 if exists, 0 otherwise
*/
extern int job_exists(mqtt_publish_status_job_t *job);

/* get the next free job slot */
extern int job_list_get_free_slot(void);

/* save a job to the job list
* @param job : job instance that will you save
* @return 1 if save was successful, 0 if error occours
*/
extern int job_list_save(mqtt_publish_status_job_t *job);

/* initialize the job list with default data */
extern void job_list_init(void);

/* save current config data */
extern void save_config();

/* read and parse config file and write its data to app_conf instance */
extern void read_config();

/* delete the config file */
extern void delete_config();

#endif /* CONFIG_SERVICE_H_ */

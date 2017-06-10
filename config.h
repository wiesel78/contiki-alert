#ifndef CONFIG_SERVICE_H_
#define CONFIG_SERVICE_H_

#include "contiki-conf.h"
#include "ping-service.h"
#include "mqtt-service.h"

#define CONFIG_BUFFER_SIZE 128
#define CONFIG_FILE_PATH "blabla"

#define WRITE_AND_RESET(fd, buf, buf_ptr, remain) { \
    cfs_write((fd), (buf), MQTT_DATA_BUFFER_SIZE - (remain)); \
    (buf_ptr) = (buf); \
    (remain) = MQTT_DATA_BUFFER_SIZE; \
}


typedef struct client_config {
    mqtt_client_config_t mqtt_conf;
    ping_client_config_t ping_conf;
} client_config_t;

typedef struct client_state {
    mqtt_service_state_t mqtt_state;
    ping_service_state_t ping_state;
} client_state_t;

extern int init_config(client_config_t *config, client_state_t *state);
extern void update_mqtt_config(void);
extern void update_ping_config(void);
extern void update_config(void);

extern void job_delete(int id);
extern int job_exists(mqtt_publish_status_job_t *job);
extern int job_list_get_free_slot(void);
extern int job_list_save(mqtt_publish_status_job_t *job);
extern void job_list_init(void);

extern void save_config();
extern void read_config();
extern void delete_config();

#endif /* CONFIG_SERVICE_H_ */

#ifndef CONFIG_SERVICE_H_
#define CONFIG_SERVICE_H_


#include "contiki-conf.h"
#include "./ping-service.h"
#include "./mqtt-service.h"


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

extern int status_job_max_id(void);
extern int status_job_exists(mqtt_publish_status_job_t *job);
extern int status_job_list_get_free_slot(void);
extern int status_job_list_save(mqtt_publish_status_job_t *job);
extern void status_job_list_init(void);

#endif /* CONFIG_SERVICE_H_ */

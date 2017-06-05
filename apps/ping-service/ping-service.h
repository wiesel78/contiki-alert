#ifndef PING_SERVICE_H_
#define PING_SERVICE_H_

#include "sys/etimer.h"

#define PING_PAYLOAD_LEN   20

#define JSON_CONFIG_KEY_INTERVAL "interval"
#define JSON_CONFIG_KEY_PING "pingConf"

#define JSON_CONFIG_PING_INTERVAL "\"" JSON_CONFIG_KEY_INTERVAL "\":%d"


/* PING client config */
typedef struct ping_client_config {
    int interval;
} ping_client_config_t;

typedef struct ping_service_state {
    int rssi;
} ping_service_state_t;

#define DEFAULT_PING_INTERVAL       (CLOCK_SECOND * 30)


extern void ping_service_init(  ping_client_config_t *config,
                                ping_service_state_t *state);
void ping_service_update(process_event_t ev, process_data_t data);
extern void ping(void);

#endif /* PING_SERVICE_H_ */

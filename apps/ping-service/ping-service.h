/*
    Ping-Service send periodically ping reuqests to the next hop

    if it is include, its start a process.
    the initialize method can call after PROCESS_BEGIN() in your main process
*/

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

/* current state of ping service */
typedef struct ping_service_state {
    int rssi;
} ping_service_state_t;

/* periodic send of ping by this time */
#define DEFAULT_PING_INTERVAL       (CLOCK_SECOND * 30)

/* initialize the ping service
 * by example, this method can call before the main loop is called
 * @param config : pointer of config item with ping-service-settings
 * @param state : state object with runtime informations (for example : rssi)
 */
extern void ping_service_init(  ping_client_config_t *config,
                                ping_service_state_t *state);

/* send a ping manually */
extern void ping(void);

#endif /* PING_SERVICE_H_ */

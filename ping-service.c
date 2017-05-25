
#include "contiki-conf.h"
#include "rpl/rpl-private.h"
#include "mqtt.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"
#include "sys/etimer.h"

#include "./config-service.h"
#include "./ping-service.h"


ping_client_config_t *ping_conf;
ping_service_state_t *ping_state;

struct uip_icmp6_echo_reply_notification ping_notification;
struct etimer echo_request_timer;



/* send a ping request */
void
ping(void)
{
    if(uip_ds6_get_global(ADDR_PREFERRED) == NULL) {
        return;
    }

    uip_icmp6_send(uip_ds6_defrt_choose(), ICMP6_ECHO_REQUEST, 0,
                 PING_PAYLOAD_LEN);
}



/* then receive a ping_response */
void
ping_handler( uip_ipaddr_t *source, uint8_t ttl,
                    uint8_t *data, uint16_t datalen)
{
    if(uip_ip6addr_cmp(source, uip_ds6_defrt_choose())) {
        ping_state->rssi = sicslowpan_get_last_rssi();
    }

    printf("icmp response was receive with signal strenght : %d\n\r", ping_state->rssi);
}



/* Initialize the ping-service */
void
ping_service_init(  ping_client_config_t *config,
                    ping_service_state_t *state)
{
    ping_conf = config;
    ping_state = state;

    uip_icmp6_echo_reply_callback_add(&ping_notification, ping_handler);

    etimer_set(&echo_request_timer, ping_conf->interval);
}



/* Call on every mainloop event and filter the ping-service events */
void
ping_service_update(process_event_t ev, process_data_t data)
{
    /* PING */
    if(ev == PROCESS_EVENT_TIMER && data == &echo_request_timer){
        ping();
        etimer_set(&echo_request_timer, ping_conf->interval);
    }
}

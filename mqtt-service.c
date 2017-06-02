#include "contiki.h"
#include "contiki-conf.h"
#include "lib/memb.h"
#include "sys/process.h"
#include "rpl/rpl-private.h"
#include "mqtt.h"
#include "net/rpl/rpl.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/ipv6/sicslowpan.h"
#include "sys/etimer.h"
#include "sys/ctimer.h"
#include "lib/sensors.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "dev/cc2538-sensors.h"

#include "./mqtt-service.h"
#include "./queue.h"


#define MAX_PUBLISH_QUEUE_ITEMS 16

MEMB(publish_queue_wrap_items, data_queue_item_t, (MAX_PUBLISH_QUEUE_ITEMS));
MEMB(publish_queue_items, publish_item_t, (MAX_PUBLISH_QUEUE_ITEMS)+1);

/* Variables */
mqtt_client_config_t *mqtt_conf;
mqtt_service_state_t *mqtt_state;
data_queue_t publish_queue;
static struct mqtt_connection conn;
static struct mqtt_message *message_pointer;
struct process *app_process;
process_event_t mqtt_event;



void led_handler(void *d)
{
    leds_off(LED_CONNECTING);
}

void connect_to_broker(void)
{
    DBG("broker");

    mqtt_connect(   &conn,
                    mqtt_conf->broker_ip,
                    mqtt_conf->broker_port,
                    DEFAULT_KEEP_ALIVE_TIMER);


    mqtt_state->state = MQTT_SERVICE_STATE_CONNECTING;
}

/* behandelt empfangene Nachrichten.  */
void publish_handler(   const char *topic, uint16_t topic_length,
                        const uint8_t *chunk, uint16_t chunk_length)
{

    if(mqtt_state->publish_handler != NULL)
    {
        (*(mqtt_state->publish_handler))(topic, topic_length, chunk, chunk_length);
    }
}

void
mqtt_service_send(){
    printf("send \n\r");

    mqtt_status_t status;
    publish_item_t *pub;


    if(mqtt_ready(&conn) && conn.out_buffer_sent){

        printf("ready to send \n\r");
        pub = data_queue_dequeue(&publish_queue);

        (mqtt_state->sequenz_number)++;

        if(!pub->is_last_will){
            printf("publish\n\r");
            status = mqtt_publish(  &conn,
                                    NULL,
                                    pub->topic,
                                    (uint8_t *)(pub->data),
                                    pub->data_length,
                                    pub->qos_level,
                                    pub->retain);

            if(status == MQTT_STATUS_OUT_QUEUE_FULL){
                printf("queue publish is full\n\r");

                data_queue_enqueue(&publish_queue, pub);
            }
        }else{
            printf("set last will\n\r");
            mqtt_set_last_will( &conn,
                                pub->topic,
                                pub->data,
                                pub->qos_level);
        }
    }

    if(data_queue_peek(&publish_queue) == NULL){
        printf("queue is empty \n\r");
        return ;
    }


    printf("queue has elements. set timer \n\r");
    ctimer_set( &(mqtt_state->publish_timer),
                NET_CONNECT_PERIODIC,
                mqtt_service_send, NULL);
}

void
mqtt_service_publish(publish_item_t *pub_item)
{
    printf("publish topic %s\n\r", pub_item->topic);

    data_queue_enqueue(&publish_queue, pub_item);

    if( &(mqtt_state->publish_timer) == NULL ||
        ctimer_expired(&(mqtt_state->publish_timer)))
    {
        printf("no or expired timer. Send Message\n\r");
        mqtt_service_send();
    }

}

void
mqtt_service_subscribe_repeat(void *data)
{
    if(mqtt_state->subscribe_tries > MAX_SUBSCRIBE_REPEAT){
        DBG("break up subscribe\n\r");
        return;
    }

    (mqtt_state->subscribe_tries)++;

    mqtt_service_subscribe( mqtt_state->subscribe_job.topic,
                            mqtt_state->subscribe_job.qos_level);
}

void
mqtt_service_subscribe(char *topic, mqtt_qos_level_t qos_level)
{
    mqtt_status_t status;

    status = mqtt_subscribe(&conn, NULL, topic, qos_level);

    DBG("APP - Subscribing! %d\n", status);
    if(status == MQTT_STATUS_OUT_QUEUE_FULL) {
        DBG("APP - Tried to subscribe but command queue was full!\n");

        mqtt_state->subscribe_job.topic = topic;
        mqtt_state->subscribe_job.qos_level = qos_level;

        ctimer_set(
            &(mqtt_state->subscribe_job.sub_timer),
            NET_CONNECT_PERIODIC,
            mqtt_service_subscribe_repeat, NULL);
    }
}



/* behandelt mqtt events */
void
mqtt_event_handler(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
    switch(event)
    {
        case MQTT_EVENT_CONNECTED: {
            timer_set(&(mqtt_state->connection_life), CONNECTION_STABLE_TIME);
            mqtt_state->state = MQTT_SERVICE_STATE_CONNECTED;
            break;
        }
        case MQTT_EVENT_DISCONNECTED: {
            mqtt_state->state = MQTT_SERVICE_STATE_DISCONNECTED;
            process_poll(app_process);
            break;
        }
        case MQTT_EVENT_PUBLISH: {
            message_pointer = data;

            if(message_pointer->first_chunk)
            {
                message_pointer->first_chunk = 0;
            }

            publish_handler(message_pointer->topic,
                            strlen(message_pointer->topic),
                            message_pointer->payload_chunk,
                            message_pointer->payload_length);

            break;
        }
        case MQTT_EVENT_SUBACK:
            DBG("suback \n\r");
            process_post_synch(app_process, mqtt_event, data);
            break;
        case MQTT_EVENT_UNSUBACK:
            DBG("unsuback \n\r");
            process_post_synch(app_process, mqtt_event, data);
            break;
        case MQTT_EVENT_PUBACK:
            DBG("puback \n\r");
            process_post_synch(app_process, mqtt_event, data);
            break;
        default:
            DBG("unhandled mqtt-event\n\r");
            break;
    }
}

void
state_machine()
{
    switch(mqtt_state->state)
    {
        case MQTT_SERVICE_STATE_INIT:
            // register mqtt connection
            mqtt_register(  &conn, app_process, mqtt_conf->client_id,
                            mqtt_event_handler, MAX_TCP_SEGMENT_SIZE);

            if(strlen(mqtt_conf->username) > 0 && strlen(mqtt_conf->password)){
                mqtt_set_username_password(&conn,
                    mqtt_conf->username,
                    mqtt_conf->password);
            }

            conn.auto_reconnect = 0;
            mqtt_state->connect_attempt = 1;
            mqtt_state->state = MQTT_SERVICE_STATE_REGISTERED;
            /* No break - continue */
        case MQTT_SERVICE_STATE_REGISTERED:
            if(uip_ds6_get_global(ADDR_PREFERRED) != NULL)
            {
                /* registered an global ip. now can connect */
                connect_to_broker();
            }
            else
            {
                leds_on(LED_CONNECTING);
                ctimer_set( &(mqtt_state->led_timer),
                        CONNECTING_LED_DURATION,
                        led_handler, NULL);
            }

            etimer_set(&(mqtt_state->periodic_timer),NET_CONNECT_PERIODIC);
            return;
            break;
        case MQTT_SERVICE_STATE_CONNECTING:
            leds_on(LED_CONNECTING);
            ctimer_set( &(mqtt_state->led_timer),
                        CONNECTING_LED_DURATION,
                        led_handler, NULL);
            /* Connecting*/
            break;
        case MQTT_SERVICE_STATE_CONNECTED:
        case MQTT_SERVICE_STATE_READY:
            DBG("STATE_READY\n\r");
            if(timer_expired(&(mqtt_state->connection_life)))
            {
                mqtt_state->connect_attempt = 0;
            }

            if(mqtt_ready(&conn) && conn.out_buffer_sent)
            {
                mqtt_state->state = MQTT_SERVICE_STATE_READY;
                return;
            }
            break;
        case MQTT_SERVICE_STATE_DISCONNECTED:
            DBG("STATE_DISCONNECTED\n\r");
            if( mqtt_state->connect_attempt < RECONNECT_ATTEMPTS ||
                RECONNECT_ATTEMPTS == RETRY_FOREVER)
            {
                /* Disconnect and backoff */
                clock_time_t interval;
                mqtt_disconnect(&conn);
                (mqtt_state->connect_attempt)++;

                interval = mqtt_state->connect_attempt < 3 ?
                    RECONNECT_INTERVAL << mqtt_state->connect_attempt :
                    RECONNECT_INTERVAL << 3;

                DBG("Disconnected. Attempt %u in %lu ticks\n", connect_attempt, interval);

                etimer_set(&(mqtt_state->periodic_timer), interval);

                mqtt_state->state = MQTT_SERVICE_STATE_REGISTERED;
                return;
            }
            else
            {
                mqtt_state->state = MQTT_SERVICE_STATE_ERROR;
            }
            break;
        case MQTT_SERVICE_STATE_CONFIG_ERROR:
            DBG("Configuration error\n\r");
            return;
        case MQTT_SERVICE_STATE_ERROR:
        default:
            leds_on(LED_ERROR);
            DBG("Error or default case in state machine\n\r");
            return;
    }

    etimer_set(&(mqtt_state->periodic_timer), STATE_MACHINE_PERIODIC);
}

/* initialize */
void
mqtt_service_init(  struct process *p,
                    mqtt_client_config_t *config,
                    mqtt_service_state_t *state)
{
    mqtt_event = process_alloc_event();

    app_process = p;
    mqtt_conf = config;
    mqtt_state = state;

    message_pointer = 0;

    data_queue_init(&publish_queue, &publish_queue_wrap_items, &publish_queue_items);
}

/* bind this into the main-loop process_thread */
void
mqtt_service_update(process_event_t ev, process_data_t data)
{
    if(ev == sensors_event && data == PUBLISH_TRIGGER)
    {
        if(mqtt_state->state == MQTT_SERVICE_STATE_ERROR)
        {
            mqtt_state->connect_attempt = 1;
            mqtt_state->state = MQTT_SERVICE_STATE_REGISTERED;
        }
    }

    if( (ev == PROCESS_EVENT_TIMER && data == &(mqtt_state->periodic_timer)) ||
        ev == PROCESS_EVENT_POLL)
    {
        DBG("event fired - activate state-machine\n\r");
        state_machine();
    }
}

int
mqtt_service_is_connected(void)
{
    return mqtt_state->state == MQTT_SERVICE_STATE_READY ? 1 : 0;
}

// PROCESS(mqtt_service, "MQTT Service");
// AUTOSTART_PROCESSES(&mqtt_service);
//
// PROCESS_THREAD(mqtt_service, ev, data)
// {
//     PROCESS_BEGIN();
//
//     while(1){
//         PROCESS_WAIT_EVENT();
//
//
//     }
//
//     PROCESS_END();
// }

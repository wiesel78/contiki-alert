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

#if CONTIKI_TARGET_CC2538DK
#include "dev/cc2538-sensors.h"
#endif /* CONTIKI_TARGET_CC2538DK */

#include "./mqtt-service.h"
#include "./queue.h"

#include "./debug.h"




PROCESS(mqtt_service_process, "MQTT Service");

/* allocation set of queue wrapping items */
MEMB(publish_queue_wrap_items, data_queue_item_t, (MQTT_QUEUE_MAX_ITEMS));
/* allocation set of queued items */
MEMB(publish_queue_items, publish_item_t, (MQTT_QUEUE_MAX_ITEMS)+1);

/* Variables */
mqtt_client_config_t *mqtt_conf;
mqtt_service_state_t *mqtt_state;
data_queue_t publish_queue;
static struct mqtt_connection conn;
static struct mqtt_message *message_pointer;
struct process *app_process;
process_event_t mqtt_event;

static mqtt_status_t status;
static publish_item_t *send_item;


#define LED_SIGNAL(duration) { \
    leds_on(LED_CONNECTING); \
    ctimer_set(&(mqtt_state->led_timer), \
            (duration), \
            led_handler, NULL); \
}

#define SET_STATE_MACHINE_TIMER(duration) { \
    ctimer_set( &(mqtt_state->periodic_timer), \
                (duration), \
                state_machine, NULL); \
}


/* call if the led timer is triggered and take out the light */
void
led_handler(void *d)
{
    leds_off(LED_CONNECTING);
}

/* called if receive a message  */
void
publish_handler(   const char *topic, uint16_t topic_length,
                        const uint8_t *chunk, uint16_t chunk_length)
{
    /* call given callback if exists */
    if(mqtt_state->publish_handler != NULL)
    {
        (*(mqtt_state->publish_handler))(topic, topic_length, chunk, chunk_length);
    }
}

/* send next message in queue if the buffer is free from last send */
void
mqtt_service_send()
{

    /* next message can only send if the buffer is free */
    if(mqtt_ready(&conn) && conn.out_buffer_sent){

        // pop the next item of queue
        send_item = data_queue_dequeue(&publish_queue);

        // increment the sequenz number
        (mqtt_state->sequenz_number)++;

        // normal message (not last will)
        if(!send_item->is_last_will){

            /* publish message */
            status = mqtt_publish(  &conn,
                                    NULL,
                                    send_item->topic,
                                    (uint8_t *)(send_item->data),
                                    send_item->data_length,
                                    send_item->qos_level,
                                    send_item->retain);

            /* check mqtt-buffer and reenque the message if mqtt-buffer is full */
            if(status == MQTT_STATUS_OUT_QUEUE_FULL){
                data_queue_enqueue(&publish_queue, send_item);
            }
        }else{
            // send to all subscribers if connection is lost
            mqtt_set_last_will( &conn,
                                send_item->topic,
                                send_item->data,
                                send_item->qos_level);
        }
    }

    /* check the message queue and return if the queue is empty (all messages out) */
    if(data_queue_peek(&publish_queue) == NULL){
        return ;
    }

    /* set timer to send the next message in queue with duration */
    ctimer_set( &(mqtt_state->publish_timer),
                NET_CONNECT_PERIODIC,
                mqtt_service_send, NULL);
}

/* put new message in the message-queue */
void
mqtt_service_publish(publish_item_t *pub_item)
{
    data_queue_enqueue(&publish_queue, pub_item);

    /* set the timer */
    if( &(mqtt_state->publish_timer) == NULL ||
        ctimer_expired(&(mqtt_state->publish_timer)))
    {
        mqtt_service_send();
    }
}

/* resubscribe until max subscribe counter has occours */
void
mqtt_service_subscribe_send(void *data)
{
    // check subscription-counter (max attemptions)
    if(mqtt_state->subscribe_attempt > MAX_SUBSCRIBE_REPEAT){
        return;
    }

    /* increment subscription counter */
    (mqtt_state->subscribe_attempt)++;

    /* resubscribe */
    mqtt_service_subscribe( mqtt_state->subscribe_job.topic,
                            mqtt_state->subscribe_job.qos_level);
}

/* subscribe a topic */
void
mqtt_service_subscribe(char *topic, mqtt_qos_level_t qos_level)
{
    // subscribe via mqtt-lib
    status = mqtt_subscribe(&conn, NULL, topic, qos_level);

    /* if not subscribe, set a timer for resubscribe */
    if(status != MQTT_STATUS_OUT_QUEUE_FULL)
        return;

    /* try resubscribe in NET_CONNECT_PERIODIC */
    mqtt_state->subscribe_job.topic = topic;
    mqtt_state->subscribe_job.qos_level = qos_level;

    ctimer_set(
        &(mqtt_state->subscribe_job.sub_timer),
        NET_CONNECT_PERIODIC,
        mqtt_service_subscribe_send, NULL);
}

/* set up connectiong to broker */
void connect_to_broker(void)
{
    // connect to broker via mqtt-lib
    mqtt_connect(   &conn,
                    mqtt_conf->broker_ip,
                    mqtt_conf->broker_port,
                    DEFAULT_KEEP_ALIVE_TIMER);

    /* set the statemachine to the next state */
    mqtt_state->state = MQTT_SERVICE_STATE_CONNECTING;
}



/* handle mqtt events */
void
mqtt_event_handler(struct mqtt_connection *m, mqtt_event_t event, void *data)
{
    switch(event)
    {
        case MQTT_EVENT_CONNECTED:
            timer_set(&(mqtt_state->connection_life), CONNECTION_STABLE_TIME);
            mqtt_state->state = MQTT_SERVICE_STATE_CONNECTED;
            break;
        case MQTT_EVENT_DISCONNECTED:
            mqtt_state->state = MQTT_SERVICE_STATE_DISCONNECTED;
            process_poll(app_process);
            break;
        case MQTT_EVENT_PUBLISH:
            // void* to mqtt_message*
            message_pointer = data;

            // ???
            if(message_pointer->first_chunk)
            {
                message_pointer->first_chunk = 0;
            }

            // call callback method if message receive
            publish_handler(message_pointer->topic,
                            strlen(message_pointer->topic),
                            message_pointer->payload_chunk,
                            message_pointer->payload_length);

            break;

        // Acknowledges
        case MQTT_EVENT_SUBACK:
        case MQTT_EVENT_UNSUBACK:
        case MQTT_EVENT_PUBACK:
        default:
            process_post_synch(app_process, mqtt_event, data);
            break;
    }
}

/* state machine for mqtt connection state*/
void
state_machine()
{
    switch(mqtt_state->state)
    {
        case MQTT_SERVICE_STATE_INIT:
            // register mqtt connection
            mqtt_register(  &conn, app_process, mqtt_conf->client_id,
                            mqtt_event_handler, MAX_TCP_SEGMENT_SIZE);

            // if username and password available, set to mqtt-connection
            if(strlen(mqtt_conf->username) > 0 && strlen(mqtt_conf->password)){
                mqtt_set_username_password(&conn,
                    mqtt_conf->username,
                    mqtt_conf->password);
            }

            // ???
            conn.auto_reconnect = 0;

            // count attemptions
            mqtt_state->connect_attempt = 1;

            // set and go to next state
            mqtt_state->state = MQTT_SERVICE_STATE_REGISTERED;
            /* No break - continue */
        case MQTT_SERVICE_STATE_REGISTERED:
            if(uip_ds6_get_global(ADDR_PREFERRED) != NULL)
            {
                /* if ip is available, connect to mqtt-broker */
                connect_to_broker();
            }
            else
            {
                /* if ip is not available. Time for LED-stuff */
                LED_SIGNAL(CONNECTING_LED_DURATION);
            }

            // time to next step in state-machine
            SET_STATE_MACHINE_TIMER(NET_CONNECT_PERIODIC);

            return;
            break;
        case MQTT_SERVICE_STATE_CONNECTING:
            LED_SIGNAL(CONNECTING_LED_DURATION);
            break;
        case MQTT_SERVICE_STATE_CONNECTED:
        case MQTT_SERVICE_STATE_READY:
            if(timer_expired(&(mqtt_state->connection_life)))
            {
                mqtt_state->connect_attempt = 0;
            }

            if(mqtt_ready(&conn) && conn.out_buffer_sent)
            {
                mqtt_state->state = MQTT_SERVICE_STATE_READY;
                process_post_synch(app_process, PROCESS_EVENT_CONTINUE, NULL);
                return;
            }
            break;
        case MQTT_SERVICE_STATE_DISCONNECTED:
            //
            if( mqtt_state->connect_attempt >= RECONNECT_ATTEMPTS &&
                RECONNECT_ATTEMPTS != RETRY_FOREVER)
            {
                mqtt_state->state = MQTT_SERVICE_STATE_ERROR;
                break;
            }

            /* Disconnect and backoff */
            clock_time_t interval;

            // disconnect via mqtt-lib
            mqtt_disconnect(&conn);

            // increment attempt counter
            (mqtt_state->connect_attempt)++;

            // set reconnect interval by attempt-counter
            interval = mqtt_state->connect_attempt < 3 ?
                RECONNECT_INTERVAL << mqtt_state->connect_attempt :
                RECONNECT_INTERVAL << 3;

            DBG("Disconnected. Attempt %u in %lu ticks\n", connect_attempt, interval);

            SET_STATE_MACHINE_TIMER(interval);

            mqtt_state->state = MQTT_SERVICE_STATE_REGISTERED;
            process_post_synch(app_process, PROCESS_EVENT_CONTINUE, NULL);
            return;
        case MQTT_SERVICE_STATE_CONFIG_ERROR:
            DBG("Configuration error\n\r");
            return;
        case MQTT_SERVICE_STATE_ERROR:
        default:
            leds_on(LED_ERROR);
            DBG("Error or default case in state machine\n\r");
            return;
    }

    SET_STATE_MACHINE_TIMER(STATE_MACHINE_PERIODIC);
}

/** initialize the mqtt-service
* @param process : main process
* @param config : mqtt configuration struct
* @param state : runtime state informations of mqtt-service
*/
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

    // initialize message queue
    data_queue_init(&publish_queue, &publish_queue_wrap_items, &publish_queue_items);

    // start state-machine
    SET_STATE_MACHINE_TIMER(STATE_MACHINE_PERIODIC);
}

/* call this in main-loop. press PUBLISH_TRIGGER button to reset state */
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
}

/* 1 if connected, 0 else */
int
mqtt_service_is_connected(void)
{
    return mqtt_state->state == MQTT_SERVICE_STATE_READY ? 1 : 0;
}


PROCESS_THREAD(mqtt_service_process, ev, data)
{
    PROCESS_BEGIN();

    while(1){
		PROCESS_WAIT_EVENT();

        mqtt_service_update(ev, data);
    }

    PROCESS_END();
}

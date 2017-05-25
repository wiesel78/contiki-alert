#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

#define MQTT_BROKER_IP_ADDR "fd00::1"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

#define TOPIC_ADD_JOB_PREFIX "job/add"
#define TOPIC_ADD_JOB "job/add/#"
#define TOPIC_ADD_JOB_ALERT TOPIC_ADD_JOB_PREFIX "/alert"
#define TOPIC_ADD_JOB_STATUS TOPIC_ADD_JOB_PREFIX "/status"

#define TOPIC_DELETE_JOB_PREFIX "job/delete"
#define TOPIC_DELETE_JOB "job/delete/#"
#define TOPIC_DELETE_JOB_ALERT TOPIC_DELETE_JOB_PREFIX "/alert"
#define TOPIC_DELETE_JOB_STATUS TOPIC_DELETE_JOB_PREFIX "/status"

#define TOPIC_PUBLISH_CLIENT_ACTIVE "clients/%s"

#endif /* PROJECT_CONF_H_ */

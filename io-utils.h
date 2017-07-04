#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#include <stdio.h>
#include <stdarg.h>

#define DECIMAL_INT_SIZE 12

#define INI_GROUP '-'
#define INI_SEPERATOR '='
#define INI_NEW_LINE '\n'

#define INI_VALUE_GROUP "-"
#define INI_VALUE_SEPERATOR "="
#define INI_VALUE_NEW_LINE "\n"


#define INI_DATA_SIZE 64
#define INI_KEY_SIZE 32

/* reset the ini parser or serializer state instance for next action */
#define INI_RESET(state) { \
    memset((state)->key, 0, INI_KEY_SIZE); \
    memset((state)->data, 0, INI_DATA_SIZE); \
    (state)->key_len = 0; \
    (state)->data_len = 0; \
    (state)->type = INI_TYPE_EMPTY; \
}

#define INI_FILE_END(state) { \
    INI_RESET((state)); \
    (state)->type = INI_TYPE_EOF; \
}

typedef enum {
    INI_TYPE_GROUP = 1 << 0,
    INI_TYPE_KEYVALUE = 1 << 1,
    INI_TYPE_EMPTY = 1 << 2,
    INI_TYPE_EOF = 1 << 3,
} ini_type_t;

typedef struct ini_state {
    char group[INI_KEY_SIZE];
    char key[INI_KEY_SIZE];
    char data[INI_DATA_SIZE];
    uint8_t key_len;
    uint16_t data_len;
    ini_type_t type;
    int fd;
} ini_state_t;

/* Helper function to write a buffer with text
* @param buf : buffer pointer
* @param remaining : pointer to char-counter
* @param data : format string like a sprintf input string
* @param ... : data arguments will be write in format string
*/
extern char* bcprintf(char *buf, int *remaining, const char *data, ...);

/* open a config file
* @param state : pointer to state instance
* @param file : filename of config file
*/
extern int ini_open(ini_state_t *state, const char *file);

/* close ini file */
extern void ini_close(ini_state_t *state);

/* Write a group line (-groupName\n)
* @param key : the name of the group
*/
extern int ini_write_group(ini_state_t *state, const char *key);

/* write key value pair into the ini file on the current position
* @param key : key as string
* @param key_len : length of key string
* @param data : value as string
* @param data_len : length of data string
* @return ...*/
extern int ini_write_int(ini_state_t *state, const char *key, int data);

/* write key value pair into the ini file on the current position
* @param key : key as string
* @param data : value as string
* @return ...*/
extern int ini_write_string(ini_state_t *state, const char *key, char *data);

/* parse next line in ini file */
extern int ini_read_next(ini_state_t *state);

#endif /* IO_UTILS_H_ */

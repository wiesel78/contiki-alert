#ifndef IO_UTILS_H_
#define IO_UTILS_H_

#define DECIMAL_INT_SIZE 12

#define INI_GROUP '-'
#define INI_SEPERATOR '='
#define INI_NEW_LINE '\n'

#define INI_VALUE_GROUP "-"
#define INI_VALUE_SEPERATOR "="
#define INI_VALUE_NEW_LINE "\n"


#define INI_DATA_SIZE 64
#define INI_KEY_SIZE 32

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


extern char* bcprintf(char *buf, int *remaining, const char *data, ...);

extern int ini_open(ini_state_t *state, const char *file);
extern void ini_close(ini_state_t *state);
extern int ini_write_group(ini_state_t *state, const char *key);
extern int ini_write_int(ini_state_t *state, const char *key, int data);
extern int ini_write_string(ini_state_t *state, const char *key, char *data);
extern int ini_read_next(ini_state_t *state);

#endif /* IO_UTILS_H_ */

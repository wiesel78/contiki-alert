#include "cfs/cfs.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "./io-utils.h"

#include "./debug.h"

/* Helper function to write a buffer with text
* @param buf : buffer pointer
* @param remaining : pointer to char-counter
* @param data : format string like a sprintf input string
* @param ... : data arguments will be write in format string
*/
char* bcprintf(char *buf, int *remaining, const char *data, ...)
{
    va_list argptr;
    int len;
    int r = (*remaining);

    va_start(argptr, data);
    len = vsnprintf(buf, r, data, argptr);
    va_end(argptr);

    if(len < 0 || len >= r){
        PRINTF("Buffer to short. Have %d, need %d + \\0\n", r, len);
        return NULL;
    }

    r -= len;
    (*remaining) = r;
    return buf + len;
}


/* open a config file
* @param state : pointer to state instance
* @param file : filename of config file
*/
int
ini_open(ini_state_t *state, const char *file){

    INI_RESET(state);
    memset(state->group, '\0', INI_KEY_SIZE);

    // check file string
    if(file == NULL || strlen(file) <= 0)
    {
        PRINTF("file path is NULL or empty\n\r");
        return -1;
    }

    // open file
    state->fd = cfs_open(file, CFS_READ | CFS_WRITE);
    if(state->fd < 0){
        PRINTF("can not open file %s\n\r", file);
        return -1;
    }

    return 1;
}

/* Write a group line (-groupName\n)
* @param key : the name of the group
*/
int
ini_write_group(ini_state_t *state, const char *key){

    snprintf(state->key, INI_KEY_SIZE + 1, "%c%s%c", INI_GROUP, key, INI_NEW_LINE);

    state->type = INI_TYPE_GROUP;

    cfs_write(state->fd, state->key, strlen(state->key));
    return 1;
}

/* write key value pair into the ini file on the current position
* @param key : key as string
* @param data : value as string
* @return ...*/
int
ini_write_string(ini_state_t *state, const char *key, char *data){

    snprintf(state->key, INI_KEY_SIZE + 1, "%s%c", key, INI_SEPERATOR);
    snprintf(state->data, INI_DATA_SIZE + 1, "%s%c", data, INI_NEW_LINE);

    state->type = INI_TYPE_KEYVALUE;

    cfs_write(state->fd, state->key, strlen(state->key));
    cfs_write(state->fd, state->data, strlen(state->data));

    return 1;
}

/* write key value pair into the ini file on the current position
* @param key : key as string
* @param key_len : length of key string
* @param data : value as string
* @param data_len : length of data string
* @return ...*/
int
ini_write_int(ini_state_t *state, const char *key, int data){

    snprintf(state->key, INI_KEY_SIZE + 1,
        "%s%s", key, INI_VALUE_SEPERATOR);
    snprintf(state->data, INI_DATA_SIZE + 1, "%d%c", data, INI_NEW_LINE);

    state->type = INI_TYPE_KEYVALUE;

    cfs_write(state->fd, state->key, strlen(state->key));
    cfs_write(state->fd, state->data, strlen(state->data));

    return 1;
}

/* parse a group line
* @param state : ini_state
* @param count : number of bytes that are read into key buffer
* @return 0 if EOF, INI_TYPE_GROUP if group are parsed */
int
ini_parse_group(ini_state_t *state, int count){

    char *ptr = strchr(state->key, INI_NEW_LINE);
    if(ptr == NULL)
    {
        INI_FILE_END(state);
        return 0;
    }

    state->type = INI_TYPE_GROUP;
    state->key_len = ptr - (state->key);
    (*ptr) = '\0';

    cfs_seek(state->fd, (state->key_len - count) + 1, CFS_SEEK_CUR);
    state->key_len = 0;
    state->data_len = 0;

    snprintf(state->group, INI_KEY_SIZE, state->key);
    memset(state->key, 0, INI_KEY_SIZE);
    memset(state->data, 0, INI_DATA_SIZE);

    return (int)INI_TYPE_GROUP;
}

/* parse a key value pair
* @param state : ini_state
* @param count : number of bytes that are read into key buffer
* @return -1 if error, INI_TYPE_KEYVALUE if key value are parsed */
int
ini_parse_keyvalue(ini_state_t *state, int count){
    char *ptr = strchr(state->key, INI_SEPERATOR);
    if(ptr == NULL)
        return -1;

    state->type = INI_TYPE_KEYVALUE;
    state->key_len = ptr - (state->key);
    (*ptr) = '\0';

    cfs_seek(state->fd, (state->key_len - count) + 1, CFS_SEEK_CUR);

    count = cfs_read(state->fd, state->data, INI_DATA_SIZE);
    if(count < 0)
        return -1;

    ptr = strchr(state->data, INI_NEW_LINE);
    if(ptr == NULL){
        state->data_len = count;
        return INI_TYPE_KEYVALUE;
    }

    state->data_len = ptr - (state->data);
    (*ptr) = '\0';

    cfs_seek(state->fd, (state->data_len - count) + 1, CFS_SEEK_CUR);
    return INI_TYPE_KEYVALUE;
}

/* parse next line in ini file */
int
ini_read_next(ini_state_t *state){
    int count = 0;

    count = cfs_read(state->fd, state->key, INI_KEY_SIZE);
    if(count < 0)
        return -1;

    if(count == 0 || (state->key)[0] == '\0')
    {
        INI_FILE_END(state);
        return (int)INI_TYPE_EOF;
    }

    if((state->key)[0] == INI_NEW_LINE){
        INI_RESET(state);
        cfs_seek(state->fd, 1 - count, CFS_SEEK_CUR);
        return (int)INI_TYPE_EMPTY;
    }

    if((state->key)[0] == INI_GROUP){
        return ini_parse_group(state, count);
    }

    return ini_parse_keyvalue(state, count);
}

/* close ini file */
void
ini_close(ini_state_t *state){
    if(state->fd < 0)
        return;

    cfs_close(state->fd);
}

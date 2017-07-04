#ifndef NET_UTILS_H_
#define NET_UTILS_H_

/* write ipv6 address into buffer */
extern int ipaddr_sprintf(char *buf, uint8_t buf_len, const uip_ipaddr_t *addr);

#endif /* NET_UTILS_H_ */

#ifndef DEBUG_H_
#define DEBUG_H_

#ifndef DEBUG
#undef DEBUG
#define DEBUG 0
#endif

#ifndef PRINTF
#undef PRINTF
#if DEBUG == 1
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG == 1 */
#endif /* ifndef PRINTF */

#endif /* DEBUG_H_ */

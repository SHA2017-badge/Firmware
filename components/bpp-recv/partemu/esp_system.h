#include <time.h>
//Dummy


//Warning: pretty crude
static inline int system_get_time() {
	return time(NULL)*1000;
}

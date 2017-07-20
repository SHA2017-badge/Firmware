

//dummy


typedef int TimerHandle_t;

#define xTimerReset( xTimer, xTicksToWait ) void
#define xTimerStart( xTimer, xTicksToWait ) void

#define  xTimerCreate( pcTimerName, xTimerPeriodInTicks, uxAutoReload, pvTimerID, pxCallbackFunction ) 1


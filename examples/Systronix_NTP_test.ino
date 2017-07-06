#include <Systronix_NTP.h>


Systronix_NTP ntp;

time_t	unix_ts;

void setup(void)
	{
	ntp.setup();
	
	if (SUCCESS == ntp.unix_ts_get(unix_ts))
		Serial.printf ("got unix timestamp: %lu\n", unix_ts);
	}
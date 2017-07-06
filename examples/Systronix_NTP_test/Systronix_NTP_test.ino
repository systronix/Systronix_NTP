#include <Systronix_NTP.h>


Systronix_NTP ntp;

time_t	unix_ts;

void setup (void)
	{
	ntp.setup();
	}
	
void loop (void)
	{
	uint8_t	ret_val;
	
	ret_val = ntp.unix_ts_get (&unix_ts);
	
	if (SUCCESS == ret_val)
		{
		Serial.printf ("got unix timestamp: %lu\n", unix_ts);
		ntp.NTP_packet_dump ();
		}
	else if (TIMEOUT == ret_val)
		Serial.printf ("NTP request time out\n");
	else	
		Serial.printf ("error getting unix timestamp\n");
	
	delay (10000);			// wait ten seconds then try again
	}
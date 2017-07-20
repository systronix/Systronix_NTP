// minimal test of Systronix_NTP library

#include <Systronix_NTP.h>

Systronix_NTP ntp;

time_t	unix_ts;

//---------------------------< S E T U P >--------------------------------------------------------------------
//
//
//

void setup (void)
	{
	if (SUCCESS != ntp.setup())				// ntp.setup() defaults to POOL; to specify a server, write ntp.setup(NIST)
		Serial.printf ("ntp.setup() failed");
	}
	

//---------------------------< L O O P >----------------------------------------------------------------------
//
// request an NTP packet
//

void loop (void)
	{
	uint8_t		ret_val;
	uint32_t	loop_delay = 60;			// time between NTP requests in seconds
	
	ret_val = ntp.unix_ts_get (&unix_ts);	// has a default timeout of 1000mS; to set to something else write ntp.unix_ts_get (&unix_ts, 5000) (min is 120mS)
	
	if (SUCCESS == ret_val)
		{
		Serial.printf ("%s: unix timestamp: %lu\n", ntp.time_server_domain_name_ptr[ntp.time_server_name_index], unix_ts);
		Serial.printf ("\tUTC time: %.2d:%.2d:%.2d\n\n",	// print UTC time
			(uint8_t)((unix_ts  % 86400L) / 3600),			// hour
			(uint8_t)((unix_ts % 3600) / 60),				// minute
			(uint8_t)(unix_ts % 60));						// second

		ntp.NTP_packet_dump ();
		}
	else if (TIMEOUT == ret_val)
		Serial.printf ("NTP request time out\n");
	else	
		Serial.printf ("error getting unix timestamp\n");
	
	Serial.printf ("%d second pause between NTP requests\n", loop_delay);
	delay (loop_delay * 1000);
	}
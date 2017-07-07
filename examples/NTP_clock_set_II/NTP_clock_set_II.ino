/*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 modified 02 Sept 2015
 by Arturo Guadalupi

 This code is in the public domain.

 */

/** Revisions

2017Jul05			NTP_clock_set using Systronix_NTP library
2017Jul02			NTP_clock_set
2017Jul01 bboyes  Change to use TeensyID lib for mac address
2017Mar02 bboyes  Testing on Teensy 3.2 and W5500


**** TODO ****
Output sometimes stops after some hours. Why? How to recover? Etnernet.maintain() doesn't?

*/

//---------------------------< I N C L U D E S >--------------------------------------------------------------

#include <SALT_FETs.h>
#include <DS1307RTC.h>

#include <Systronix_NTP.h>


//---------------------------< D E F I N E S >----------------------------------------------------------------

#define	CS_PIN				8			// resistive touch controller XPT2406 uses SPI

#define	TFT_CS				20			// 10 is default, different on ethernet/touch combo
#define	TFT_DC				21			// 9 is default, different on ethernet/touch combo

#define	PERIF_RST			22			// SALT I/O such as PCA9557
#define	ETH_RST				9			// ethernet reset pin
#define	ETH_CS				10			// ethernet chip select

// SALT_FETs is used here because, for reasons unknown, the DS1307RTC include will cause Teensy to hang unless
// some other something that uses the i2c is also started.  Yet another TODO

SALT_FETs		FETs;							// to turn of lights fans, alarm
Systronix_NTP	ntp;

uint32_t new_millis=0;


//---------------------------< S E T U P >--------------------------------------------------------------------
//
//
//

void setup()
	{
	/*
	* When using with PJRC wiz820 sd adapter
	* https://www.pjrc.com/store/wiz820_sd_adaptor.html [sic]
	* Some SD cards can be sensitive to SPI activity while the Ethernet library is initialized before the SD library.
	* For best compatiblity with all SD cards, these 3 lines are recommended at the beginning of setup().
	* Pins 4 and 10 will be reconfigured as outputs by the SD and Ethernet libraries. Making them input
	* pullup mode before initialization guarantees neither device can respond to unintentional signals
	* while the other is initialized.
	*/
	pinMode(4, INPUT_PULLUP);

	pinMode(ETH_RST, OUTPUT);				// This drives the pin low. Don't see any way to avoid that
	pinMode(PERIF_RST, OUTPUT);				// High after POR, low when declared output
	digitalWrite(ETH_RST, LOW);				// assert it deliberately
	digitalWrite(PERIF_RST, LOW);			// low after POR anyway

	pinMode (CS_PIN, INPUT_PULLUP);			// disable resistive touch controller
	pinMode (TFT_CS, INPUT_PULLUP);			// disable LCD display
	pinMode (TFT_DC, INPUT_PULLUP);

	delay(1);								// allow time for pins to settle

	digitalWrite(ETH_RST, HIGH);			// release resets
	digitalWrite(PERIF_RST, HIGH);

	delay(1);								// time for WIZ850io to reset
	FETs.setup (I2C_FET);					// constructor for SALT_FETs, and PCA9557
	FETs.begin ();
	FETs.init ();							// lights, fans, and alarms all off


	Serial.begin(115200);					// Open serial communications and wait for port to open:
	while((!Serial) && (millis()<10000));	// wait until serial monitor is open or timeout,

	new_millis = millis();

	Serial.printf ("NTP clock set and Systroix_NTP library test for Teensy 3\nBuild %s %s\r\n", __TIME__, __DATE__);
	Serial.printf("%u msec to start serial\r\n", new_millis);
	Serial.printf("USB Serialnumber: %u \n", teensyUsbSN());	// print out the USB serial number

	delay(1000);

	teensyMAC(ntp.mac);							// get Teensy MAC address
	Serial.print("MAC from Teensy: ");
	Serial.printf("Array MAC Address: %02X:%02X:%02X:%02X:%02X:%02X \r\n", ntp.mac[0], ntp.mac[1], ntp.mac[2], ntp.mac[3], ntp.mac[4], ntp.mac[5]);  

	new_millis = millis();
	Serial.printf ("configuring ethernet\n");

	if (SUCCESS != ntp.setup(POOL))
		Serial.printf ("ntp.setup() failed");

	Serial.printf("\nSend V/v to toggle verbose\n");
	}


//---------------------------< L O O P >----------------------------------------------------------------------
//
//
//

boolean	clock_set = false;					// flag so we set the clock only once
boolean	verbose = false;					// packet dump after every transaction
int32_t	diff;								// difference between NTP time and RTC time; positive diff: NTP leads RTC
int32_t	min_diff = 0x7FFFFFFF;				// worst case RTC lag time
int32_t	max_diff = 0;						// worst case NTP lag time
uint8_t	inbyte;


void loop()
	{
	uint32_t event_millis = millis();
	uint32_t event_secs = event_millis/1000;
	time_t epoch;
	time_t rtc_ts;
	uint8_t	ret_val;

	Serial.printf ("@%.2d:%.2d:%.2d.%.4d ask for time from %s\r\n",
		(uint8_t)((event_secs % 86400L) / 3600),			// hour
		(uint8_t)((event_secs % 3600) / 60),				// minute
		(uint8_t)(event_secs % 60),							// second
		(uint8_t)(event_millis % 1000),						// millisecond
		ntp.time_server_domain_name_ptr[ntp.time_server_name_index]);

	ret_val = ntp.unix_ts_get (&epoch, 120);				// get the unix timestamp; 120mS timeout
	if (SUCCESS == ret_val)
		{
		if ((0 < ntp.packet_buffer.as_struct.stratum) && (16 > ntp.packet_buffer.as_struct.stratum))
			{
			Serial.printf ("\tUTC time: %.2d:%.2d:%.2d\n\n",	// print UTC time
				(uint8_t)((epoch  % 86400L) / 3600),			// hour
				(uint8_t)((epoch % 3600) / 60),					// minute
				(uint8_t)(epoch % 60));							// second
			Serial.printf ("\tUnix ts: %ld\n", epoch);
			
			if (clock_set)
				{
				rtc_ts = RTC.get();								// get time from RTC
				Serial.printf ("\tRTC ts:  %lu\n", rtc_ts);		// print RTC time stamp
				diff = (int32_t)(epoch - rtc_ts);
				min_diff = min (min_diff, diff);
				max_diff = max (max_diff, diff);
				Serial.printf ("\tNTP Unix/RTC difference: %ld (NTP max lag: %ld; NTP max lead: %ld)\n\n", diff, min_diff, max_diff);
				if (verbose)
					ntp.NTP_packet_dump ();
				}
			else
				{
				if (RTC.set (epoch))
					{
					Serial.printf ("RTC set to %lu\n", epoch);
					clock_set = true;
					ntp.NTP_packet_dump ();
					}
				else
					Serial.printf ("RTC.set() failed\n");		// can't know why; RTC.set() returns boolean
				}
			}
		else if (0 == ntp.packet_buffer.as_struct.stratum)
			Serial.printf ("\tkiss o' death message: %c%c%c%c (%ld)\n\n", ntp.packet_buffer.as_array[12], ntp.packet_buffer.as_array[13], ntp.packet_buffer.as_array[14], ntp.packet_buffer.as_array[15]);
		else // if (15 < ntp.packet_buffer.as_struct.stratum)
			Serial.printf ("\tunsychronized server\n\n");
		}
	else if (TIMEOUT == ret_val)
		Serial.printf ("NTP request timeout\n\n");
	else
		Serial.printf ("NTP request fail\n\n");

	delay (10000);											// wait ten seconds before asking for the time again

	if (0 < Serial.available())
		{
		inbyte = Serial.read();
		if (('v' == (inbyte)) || ('V' == (inbyte)))
			{
			verbose = !verbose;
			Serial.printf("\r\nverbose: %s ", verbose ? "true" : "false");
			}
		}

	Ethernet.maintain();
	}


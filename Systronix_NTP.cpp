// NTP.cpp

//---------------------------< I N C L U D E S >--------------------------------------------------------------

#include <Systronix_NTP.h>

EthernetUDP Udp;						// A UDP instance to let us send and receive packets over UDP


//---------------------------< S E T U P >--------------------------------------------------------------------
//
//
//

uint8_t Systronix_NTP::setup (uint8_t server_name_index = POOL)
	{
	teensyMAC(mac);							// get Teensy MAC address

//	if (0 == Ethernet.begin(mac, 5000))		// 5ish second timeout instead of 60 seconds
	if (0 == Ethernet.begin(mac))			// default 60 seconds
		{
		Serial.printf ("\nFailed to configure Ethernet using DHCP\n");
		return FAIL;
		}

//	Serial.printf ("ethernet begin done\n");

	Ethernet.init (ETH_CS_PIN);

	if (0 == Udp.begin (LOCAL_PORT))		// open a udp socket
		{
		Serial.printf ("\nUdp.begin() failed\n");
		return FAIL;
		}

//	Serial.printf ("Udp.begin(%d) done\n", LOCAL_PORT);

	time_server_name_index = server_name_index;
//	Serial.printf ("getting time from server: %s\n", time_server_domain_name_ptr[time_server_name_index]);
	return SUCCESS;
	}


//---------------------------< B E G I N >--------------------------------------------------------------------
//
//
//

uint8_t Systronix_NTP::begin (void)
	{
	return SUCCESS;
	}


//---------------------------< I N I T >----------------------------------------------------------------------
//
//
//

uint8_t Systronix_NTP::init (void)
	{
	return SUCCESS;
	}


//---------------------------< U N I X _ T S _ G E T >--------------------------------------------------------
//
// request an NTP packet, extract and return a Unix timestamp from the NTP transmit timestamp.
//
// The NTP transmit timestamp starts at byte 40 of the received packet and is four bytes long.
//
// ARM is little endian - ls byte in lowest memory address.  Network protocol is big endian.  In an array of
// bytes, lowest array index holds the most significant byte but when we grab four at once (as if the memory
// address is a unit32_t, the byte order is bass ackwards.  But, we can flip the order to get the correct result.
//
// This is a blocking function.
//
// This function accepts a timeout argument.  The default value is 1000mS.  timeout_arg values less than 120mS
// are not accepted and the timeout_arg value is unconditionally set to 120mS.
//

uint8_t Systronix_NTP::unix_ts_get (time_t* unix_ts_ptr, uint32_t timeout_arg)
	{
	time_t		ntp_xmit_ts;
	uint8_t		ret_val = TIMEOUT;								// assume timeout unless determined otherwise
	uint32_t	NTP_timer;
	uint32_t	NTP_wait_timer_start_time;
	uint32_t	NTP_timeout;
	int32_t		Udp_bytes_read;
	
	uint32_t	parse_ret_val;

	NTP_request (time_server_domain_name_ptr[NIST]);
	NTP_wait_timer_start_time = millis ();						// mark the start time
	NTP_timeout = NTP_wait_timer_start_time + ((120 > timeout_arg) ? 120 : timeout_arg);	// timeout not less than 120mS

	while (NTP_timeout > millis())
		{
		NTP_timer = millis() - NTP_wait_timer_start_time;		// time since request
		if (100 <= NTP_timer)									// don't check for a response until we've waited for 100mS
			{
			parse_ret_val = Udp.parsePacket();
			if (parse_ret_val)									// returns number of bytes in packet or 0
				{
				if (NTP_PACKET_SIZE != parse_ret_val)
					{
					Serial.printf ("parse_ret_val: %ld; expected %ld\n", parse_ret_val, NTP_PACKET_SIZE);
					return FAIL;
					}
					
				Udp_bytes_read = Udp.read(packet_buffer.as_array, NTP_PACKET_SIZE); 	// read the packet into the buffer

				if (-1 == Udp_bytes_read)
					{
					Serial.printf ("no udp data available or receive failed\n");
					return FAIL;
					}
				
				if (NTP_PACKET_SIZE != Udp_bytes_read)			// unexpected packet length
					{
					Serial.printf ("udp read %ld bytes; expected %ld\n", Udp_bytes_read, NTP_PACKET_SIZE);
					return FAIL;
					}

				ntp_xmit_ts = __builtin_bswap32 (packet_buffer.as_struct.transmit_TS_int);	// reorder into little endian time_t

				*unix_ts_ptr = ntp_xmit_ts - SEVENTY_YEARS;		// convert NTP time to Unix time
				ret_val = SUCCESS;
				break;
				}
			}
		}

	return ret_val;
	}


//---------------------------< N T P _ R E Q U E S T >--------------------------------------------------------
//
// send an NTP request packet to the time server at the given domain name
//

void Systronix_NTP::NTP_request (char* domain_name_ptr)
	{
//	int32_t ret_val;
	
	memset (packet_buffer.as_array, 0, NTP_PACKET_SIZE);	// set all bytes in the buffer to 0
												// Initialize values needed to form NTP request (see URL above for details)
	packet_buffer.as_struct.mode = 0b00100011;				// msb to lsb: LI (00 - server message); Version (100 4);  Mode (011 = client) (0x23)

															// all NTP fields have been given values, now send a packet requesting a timestamp
	if (!Udp.beginPacket (domain_name_ptr, 123))			//NTP requests are to port 123; returns 1 if successful
		Serial.printf ("Udp.beginPacket() returned 0\n@domain: %s\n", domain_name_ptr);	// experiment to find out what gets returned on success
	
	if (!Udp.write (packet_buffer.as_array, NTP_PACKET_SIZE))
		Serial.printf ("Udp.write() returned 0\n");			// experiment to find out what gets returned on success
//	Serial.printf ("Udp.write() ok\n");			// experiment to find out what gets returned on success

	if (!Udp.endPacket())
		Serial.printf ("Udp.endPacket() returned 0\n");		// experiment to find out what gets returned on success
//	Serial.printf ("Udp.endPacket() ok\n");			// experiment to find out what gets returned on success
	}


//---------------------------< N T P _ P A C K E T _ D U M P >------------------------------------------------
//
// dump the content of the received NTP packet
//

void Systronix_NTP::NTP_packet_dump (void)
	{
	uint32_t	temp32;

	Serial.printf ("NTP rx packet dump:\n");
	Serial.printf ("\t [0]: 0x%.2X\n", packet_buffer.as_array[0]);
	Serial.printf ("\t\t[0]: %d - leap indicator\n", packet_buffer.as_struct.mode >> 6);
	Serial.printf ("\t\t[0]: %d - version\n", (packet_buffer.as_struct.mode & 0b00111000) >> 3);
	Serial.printf ("\t\t[0]: %d - mode\n", packet_buffer.as_struct.mode & 0b00000111);
	Serial.printf ("\t [1]: 0x%.2X (%d) - stratum\n", packet_buffer.as_struct.stratum, packet_buffer.as_struct.stratum);
	Serial.printf ("\t [2]: 0x%.2X (%d) - poll\n", packet_buffer.as_struct.poll, packet_buffer.as_struct.poll);
	Serial.printf ("\t [3]: 0x%.2X (%d) - precision\n", packet_buffer.as_struct.precision, packet_buffer.as_struct.precision);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.root_delay);
	Serial.printf ("\t [4]: 0x%.8X (%0d.%d) - root delay\n", temp32, (int16_t)(temp32 >> 16), (uint16_t)temp32);	// TODO fix this

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.root_dispersion);
	Serial.printf ("\t [8]: 0x%.8X (%0d.%d) - root dispersion\n", temp32, (uint16_t)(temp32 >> 16), (uint16_t)temp32);	// TODO fix this

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.reference_ID);
	Serial.printf ("\t[12]: 0x%.8X (%lu) - reference ID\n", temp32, temp32);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.reference_TS_int);
	Serial.printf ("\t[16]: 0x%.8X (%lu) - reference TS (int)\n", temp32, temp32);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.reference_TS_frac);
	Serial.printf ("\t[20]: 0x%.8X (%lu) - reference TS (frac)\n", temp32, temp32);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.origin_TS_int);
	Serial.printf ("\t[24]: 0x%.8X (%lu) - origin TS (int)\n", temp32, temp32);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.origin_TS_frac);
	Serial.printf ("\t[28]: 0x%.8X (%lu) - origin TS (frac)\n", temp32, temp32);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.receive_TS_int);
	Serial.printf ("\t[32]: 0x%.8X (%lu) - receive TS (int)\n", temp32, temp32);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.receive_TS_frac);
	Serial.printf ("\t[36]: 0x%.8X (%lu) - receive TS (frac)\n", temp32, temp32);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.transmit_TS_int);
	Serial.printf ("\t[40]: 0x%.8X (%lu) - transmit TS (int)\n", temp32, temp32);

	temp32 = __builtin_bswap32 (packet_buffer.as_struct.transmit_TS_frac);
	Serial.printf ("\t[44]: 0x%.8X (%lu) - transmit TS (frac)\n\n", temp32, temp32);

	if (0 == packet_buffer.as_struct.stratum)								// if stratum is 0
		Serial.printf ("\tkiss o' death message: %c%c%c%c\n\n", packet_buffer.as_array[12], packet_buffer.as_array[13], packet_buffer.as_array[14], packet_buffer.as_array[15]);

	if (1 == packet_buffer.as_struct.stratum)								// if primary stratum
		Serial.printf ("\tprimary stratum ID code: %c%c%c%c\n\n", packet_buffer.as_array[12], packet_buffer.as_array[13], packet_buffer.as_array[14], packet_buffer.as_array[15]);
	}

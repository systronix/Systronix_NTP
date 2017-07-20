#ifndef		NTP_H_
#define		NTP_H_


//---------------------------< I N C L U D E S >--------------------------------------------------------------

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

#include <TeensyID.h>						// https://github.com/systronix/TeensyID


//---------------------------< D E F I N E S >----------------------------------------------------------------

#ifndef	ETH_CS_PIN
#define	ETH_CS_PIN			10
#endif

#define	NTP_PACKET_SIZE		48				// NTP time packet is 48 bytes
#define	SEVENTY_YEARS		2208988800UL	// offset from 1900-01-01T00:00:00 to Unix epoch 1970-01-01T00:00:00

#define	LOCAL_PORT			8888

#define	SUCCESS				0
#define	FAIL				(~SUCCESS)
#define	TIMEOUT				0xF0

enum server_name_indexes					// indexes into time_server_domain_name_ptr array
	{
	POOL,
	NIST,
	XMIS,
	XMIS_IP,
	__ARRAY_SIZE__							// this item must be the last; it is not a server name
	};

//---------------------------< C L A S S >--------------------------------------------------------------------

class Systronix_NTP
	{
	private:

	protected:
		void		NTP_request (char* domain_name_ptr);

	public:
		uint8_t		time_server_name_index = POOL;		// default to the pool

		union
			{
			struct			// !! multibyte members of this struct are big endian !! for arm controllers, these
				{			// members must be converted to little endian before use
				uint8_t		mode;						// [0] leap indicator, version, mode bits
				uint8_t		stratum;					// [1] 0-16
				uint8_t		poll;						// [2]
				int8_t		precision;					// [3]
				uint32_t	root_delay;					// [4]
				uint32_t	root_dispersion;			// [8]
				uint32_t	reference_ID;				// [12]
				uint32_t	reference_TS_int;			// [16]
				uint32_t	reference_TS_frac;			// [20]
				uint32_t	origin_TS_int;				// [24]
				uint32_t	origin_TS_frac;				// [28]
				uint32_t	receive_TS_int;				// [32]
				uint32_t	receive_TS_frac;			// [36]
				uint32_t	transmit_TS_int;			// [40]
				uint32_t	transmit_TS_frac;			// [44]
				} as_struct;
			uint8_t			as_array [NTP_PACKET_SIZE];
			} packet_buffer;


//		uint8_t		packet_buffer[NTP_PACKET_SIZE];		// buffer to hold incoming and outgoing packets

														// time_server_domain_name_ptr is public so that it can be changed on the fly
														// TODO: array of domain names?
		char*		time_server_domain_name_ptr[__ARRAY_SIZE__] = // __ARRAY_SIZE__ is last member of server name_indexes enum
			{
			{(char*)"pool.ntp.org"},					// ntp project pool of servers http://www.pool.ntp.org/zone/us; can return unexpected results
			{(char*)"time.nist.gov"},					// NIST NTP server
			{(char*)"198.60.22.240"},					// xmission ipv4 address
			{(char*)"clock.xmission.com"},				// banging on this one returns a kiss o' death RATE message
			};

		uint8_t		mac[6];								// Teensy mac goes here

		uint8_t		setup (uint8_t time_server_name_index = POOL);
		uint8_t		begin(void);
		uint8_t		init(void);

		uint8_t		unix_ts_get (time_t* unix_ts_ptr, uint32_t timeout_arg = 1000);	// default timeout_arg is 1000ms
		void		NTP_packet_dump (void);
	};

extern Systronix_NTP ntp;

#endif		// NTP_H_
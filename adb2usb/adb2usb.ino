/** \file
 * Convert Apple Desktop Bus protocol to USB.
 *
 * Pinout (female socket, front view):
 *
 *  4 3    
 * 2   1
 *   -
 *
 * 1: ADB Data (black)
 * 2: PSW (brown)
 * 3: +5V (red)
 * 4: GND (orange)
 *
 * Useful documentation:
 * https://developer.apple.com/legacy/library/technotes/hw/hw_01.html
 */

#define ADB_PORT PORTD
#define ADB_DDR DDRD
#define ADB_INPUT PIND
#define ADB_PIN 4


#define ADB_CMD_TALK	0x0C

#define ADB_REG_0	0x00
#define ADB_REG_1	0x01
#define ADB_REG_2	0x02
#define ADB_REG_3	0x03


static void led_state(int x)
{
	if (x)
		PORTD |= 1 << 6;
	else
		PORTD &= ~(1 << 6);
}

static void led_init(void)
{
	DDRD |= 1 << 6;
}


static void trigger_state(int x)
{
	if (x)
		PORTD |= 1 << 3;
	else
		PORTD &= ~(1 << 3);
}

static void trigger_init(void)
{
	DDRD |= 1 << 3;
}

static void
adb_drive(int value)
{
	if (value)
	{
		// activate pull up
		ADB_DDR  &= ~(1 << ADB_PIN);
		ADB_PORT |=  (1 << ADB_PIN);
	} else {
		// drive low
		ADB_DDR  |=  (1 << ADB_PIN);
		ADB_PORT &= ~(1 << ADB_PIN);
	}
}


static void
adb_idle(void)
{
	adb_drive(1);
}


static void
adb_reset(void)
{
	adb_drive(0);
	delayMicroseconds(3000);
	adb_drive(1);
}


static void
adb_send_byte(
	uint8_t byte
)
{
	// eight data bits, pulse width encoded
	for (int i = 0 ; i < 8 ; i++)
	{
		if (byte & 0x80)
		{
			adb_drive(0);
			delayMicroseconds(35);
			adb_drive(1);
			delayMicroseconds(65);
		} else {
			adb_drive(0);
			delayMicroseconds(65);
			adb_drive(1);
			delayMicroseconds(35);
		}
		byte <<= 1;
	}
}


static inline volatile uint8_t
adb_input(void)
{
	return (ADB_INPUT & (1 << ADB_PIN)) ? 1 : 0;
}


static uint8_t
adb_send(
	uint8_t byte
)
{
	cli();

	// attention signal -- low for 800 usec
	adb_drive(0);
	delayMicroseconds(800);

	// sync signal -- high for 70 usec
	adb_drive(1);
	delayMicroseconds(70);

	adb_send_byte(byte);

	// stop bit -- low for 65 usec
	adb_drive(0);
	delayMicroseconds(65);

	// and go back into read mode
	adb_idle();
	sei();

	// if the line is still held low, SRQ has been asserted by
	// some device.  do a quick scan to clear it.
	if (adb_input() == 0)
		return 1;

	return 0;
}



static uint8_t
adb_read_byte(void)
{
	uint8_t byte = 0;

	for (uint8_t i = 0 ; i < 8 ; i++)
	{
		// wait for falling edge; need timeout/watchdog
		while (adb_input())
			;

		// wait 50 usec, sample
		trigger_state(0);
		delayMicroseconds(55);
		const uint8_t bit = adb_input();
		byte = (byte << 1) | bit;

		trigger_state(1);

		// make sure we are back into the high-period
		delayMicroseconds(15);
		while (adb_input() == 0)
			;
	}

	trigger_state(0);
	return byte;
}


static uint8_t
adb_read(
	uint8_t * buf,
	uint8_t len
)
{
	// Wait up to a few hundred usec to see if there is a start bit
	adb_idle();

	cli();
	//uint32_t end_time = micros() + 300;
	//while (micros() != end_time)
	for (int i = 0 ; i < 5000 ; i++)
	{
		const uint8_t bit = adb_input();
		if (bit == 0)
			goto start_bit;
	}

	// no start bit seen
	sei();
	return 0;

start_bit:
	led_state(1);

	// get the start bit
	trigger_state(1);
	delayMicroseconds(90);

	for (uint8_t i = 0 ; i < len ; i++)
		buf[i] = adb_read_byte();

	led_state(0);
	sei();

	return 1;
}



void setup(void)
{
	led_init();
	trigger_init();

	// Configure the pins for pull up
	adb_idle();

	// for now write to the serial port
	Serial.begin(9600);

	// initiate a reset cycle
	adb_reset();
	delayMicroseconds(10000);


	// clear srq
	for (uint8_t dev = 0 ; dev < 16 ; dev++)
	{
		adb_send((dev << 4) | ADB_CMD_TALK | ADB_REG_0);
		delay(10);
	}

#if 0
	// Find the mouse address
	uint8_t resp[2];
	uint8_t len;
	adb_send((0x3<<4) | ADB_CMD_TALK | ADB_REG3);
	len = adb_read(resp, 2);
#endif
}


static void
print_u8(
	uint8_t x
)
{
	Serial.print((x >> 4) & 0xF, HEX);
	Serial.print((x >> 0) & 0xF, HEX);
}


void loop(void)
{
	uint8_t buf[2];

	Serial.println("scanning");
	for(uint8_t i = 0 ; i < 16 ; i++)
	{
		delay(1);
		adb_send((i << 4) | ADB_CMD_TALK | ADB_REG_3);
		if (adb_read(buf, 2) == 0)
			continue;

		Serial.print(i);
		Serial.print(' ');
		print_u8(buf[0]);
		print_u8(buf[1]);
		Serial.println();

/*
		while (Serial.available() == 0)
			;
		Serial.read();
*/
	}
}

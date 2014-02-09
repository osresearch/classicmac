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


#define ADB_CMD_FLUSH	0x01
#define ADB_CMD_LISTEN	0x08
#define ADB_CMD_TALK	0x0C

#define ADB_REG_0	0x00
#define ADB_REG_1	0x01
#define ADB_REG_2	0x02
#define ADB_REG_3	0x03


static void
print_u8(
	uint8_t x
)
{
	Serial.print((x >> 4) & 0xF, HEX);
	Serial.print((x >> 0) & 0xF, HEX);
}


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
	{
		// wait for the line to come back high
		while (adb_input() == 0)
			;
		return 1;
	}

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
		delayMicroseconds(50);
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
	delayMicroseconds(70);

	for (uint8_t i = 0 ; i < len ; i++)
		buf[i] = adb_read_byte();

	led_state(0);
	sei();

	return 1;
}


static void
adb_reset(void)
{
	adb_drive(0);
	delayMicroseconds(3000);
	adb_drive(1);
	delayMicroseconds(3000);

	// Tell all devices to reset
	for (uint8_t dev = 0 ; dev < 16 ; dev++)
	{
		adb_send((dev << 4) | ADB_CMD_FLUSH);
		delayMicroseconds(10000);
	}

	// And attempt to clear any SRQ
	for (uint8_t dev = 0 ; dev < 16 ; dev++)
	{
		adb_send((dev << 4) | ADB_CMD_TALK | ADB_REG_0);
		delayMicroseconds(10000);
	}
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


	Serial.println("scanning");
	uint8_t buf[2];
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
	}
}


void loop(void)
{
	uint8_t buf[2];

	// read from the keyboard
	adb_send(0x2C);
	if (adb_read(buf, 2))
	{
		Serial.print("K:");
		print_u8(buf[0]);
		print_u8(buf[1]);
		Serial.println();
	}
	delayMicroseconds(3000);

	// Poll the mouse
	adb_send(0x3C);
	if (adb_read(buf, 2))
	{
		uint16_t ev = (buf[0] << 8) | buf[1];

		// parsing EM85000 datasheet
		// 15: !M main mouse button
		// 14-8: signed 7-bit value (positive is up)
		//  7: !R right mouse button
		// 6-0: signed 7-bit value (positive is right)
		uint8_t m1 = (buf[0] & 0x80) ? 0 : 1;
		uint8_t m2 = (buf[1] & 0x80) ? 0 : 1;
		int8_t dx = buf[0] & 0x7F;
		int8_t dy = buf[1] & 0x7F;

		// sign extend dx and dy
		dx |= (dx & 0x40) << 1;
		dy |= (dy & 0x40) << 1;

		Serial.print("M:");
		print_u8(buf[0]);
		print_u8(buf[1]);
		Serial.print(' ');
		Serial.print(dx);
		Serial.print(' ');
		Serial.print(dy);
		Serial.print(' ');
		Serial.print(m1);
		Serial.print(m2);
		Serial.println();
	}

	delayMicroseconds(3000);
}

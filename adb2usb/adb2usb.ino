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
 */

#define ADB_PORT PORTD
#define ADB_DDR DDRD
#define ADB_INPUT PIND
#define ADB_PIN 4

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
		ADB_PORT &= ~(1 << ADB_PIN);
		ADB_DDR  |=  (1 << ADB_PIN);
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

	// stop bit -- low for 65 usec
	adb_drive(0);
	delayMicroseconds(65);

	// and go back into read mode
	adb_idle();
	sei();
}



void setup(void)
{
	// Configure the pins for pull up
	adb_idle();

	// for now write to the serial port
	Serial.begin(9600);

	// initiate a reset cycle
	adb_reset();
	delayMicroseconds(3000);
}


void loop(void)
{
/*
	static uint8_t cmd = 0;
	Serial.println(cmd);
	adb_send(cmd++); // read register 0, device 2
*/
	for(uint8_t i = 0 ; i < 16 ; i++)
	{
		uint8_t v = (i << 4) | (0x3 << 2);
		adb_send(v);
		Serial.print(i);
		Serial.print(' ');
		Serial.println(v);

		while (Serial.available() == 0)
			;
		Serial.read();
	}
}

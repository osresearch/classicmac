// \file
 /* PRU based Mac 128/Plus/SE video driver.
 *
 * The ARM writes a 1-bit color 512x384 bitmap into the shared RAM and the
 * PRU writes it to the video output continuously.
 *
 * During the horizontal blanking interval, the PRU loads an entire 512
 * bits of data (16x 32-bit registers).
 */
.origin 0
.entrypoint START

#include "ws281x.hp"

/** Mappings of the GPIO devices */
#define GPIO0 (0x44E07000 + 0x100)
#define GPIO1 (0x4804c000 + 0x100)
#define GPIO2 (0x481AC000 + 0x100)
#define GPIO3 (0x481AE000 + 0x100)

/** Offsets for the clear and set registers in the devices.
 * Since the offsets can only be 0xFF, we deliberately add offsets
 */
#define GPIO_CLRDATAOUT (0x190 - 0x100)
#define GPIO_SETDATAOUT (0x194 - 0x100)

/** Register map */
#define data_addr r0
#define row r1
#define col r2
#define video_pin r3
#define hsync_pin r4
#define vsync_pin r5
#define gpio0_base r6
#define sleep_counter r7 // how long to wait
#define timer_ptr r8
#define pixel_data r10 // the next 16 registers, too
#define tmp1 r28
#define tmp2 r29

/** GPIO0 pin numbers for our outputs */
#define VIDEO_PIN 23
#define HSYNC_PIN 27
#define VSYNC_PIN 22

#define VSYNC_LO SBBO vsync_pin, gpio0_base, GPIO_CLRDATAOUT, 4
#define VSYNC_HI SBBO vsync_pin, gpio0_base, GPIO_SETDATAOUT, 4

#define HSYNC_LO SBBO hsync_pin, gpio0_base, GPIO_CLRDATAOUT, 4
#define HSYNC_HI SBBO hsync_pin, gpio0_base, GPIO_SETDATAOUT, 4

#define VIDEO_LO SBBO video_pin, gpio0_base, GPIO_CLRDATAOUT, 4
#define VIDEO_HI SBBO video_pin, gpio0_base, GPIO_SETDATAOUT, 4

#define NOP ADD r0, r0, 0

/** Wait for the cycle counter to reach a given value; we might
 * overshoot by a bit so we ensure that we always have the same
 * amount of overshoot.
 *
 * wake_time += ns;
 * while (now = read_timer()) < wake_time)
 *	;
 * if (now - wake_time < 1)
 *      nop()
 */
#define WAITNS(ns,lab) \
	MOV tmp1, (ns)/5; \
	ADD sleep_counter, sleep_counter, tmp1; \
lab: ; \
	LBBO tmp2, timer_ptr, 0xC, 4; /* read the cycle counter */ \
	QBGT lab, tmp2, sleep_counter; \
	SUB tmp2, tmp2, sleep_counter; \
	QBLT lab##_2, tmp2, 1; \
	NOP; \
lab##_2: ; \



START:
    // Enable OCP master port
    // clear the STANDBY_INIT bit in the SYSCFG register,
    // otherwise the PRU will not be able to write outside the
    // PRU memory space and to the BeagleBon's pins.
    LBCO	r0, C4, 4, 4
    CLR		r0, r0, 4
    SBCO	r0, C4, 4, 4

    // Configure the programmable pointer register for PRU0 by setting
    // c28_pointer[15:0] field to 0x0120.  This will make C28 point to
    // 0x00012000 (PRU shared RAM).
    MOV		r0, 0x00000120
    MOV		r1, CTPPR_0
    ST32	r0, r1

    // Configure the programmable pointer register for PRU0 by setting
    // c31_pointer[15:0] field to 0x0010.  This will make C31 point to
    // 0x80001000 (DDR memory).
    MOV		r0, 0x00100000
    MOV		r1, CTPPR_1
    ST32	r0, r1

    // Write a 0x1 into the response field so that they know we have started
    MOV r2, #0x1
    SBCO r2, CONST_PRUDRAM, 12, 4

    MOV timer_ptr, 0x22000 /* control register */

    // Configure our output pins
    MOV gpio0_base, GPIO0
    MOV video_pin, 1 << VIDEO_PIN
    MOV hsync_pin, 1 << HSYNC_PIN
    MOV vsync_pin, 1 << VSYNC_PIN

    VIDEO_HI
    HSYNC_HI
    VSYNC_HI

    // Wait for the start condition from the main program to indicate
    // that we have a rendered frame ready to clock out.  This also
    // handles the exit case if an invalid value is written to the start
    // start position.
READ_LOOP:
        // Load the pointer to the buffer from PRU DRAM into r0 and the
        // length (in pixels) into r1.
        LBCO      data_addr, CONST_PRUDRAM, 0, 4

        // Wait for a non-zero command
        QBEQ READ_LOOP, data_addr, #0

        // Command of 0xFF is the signal to exit
        QBEQ EXIT, data_addr, #0xFF

	VSYNC_LO

	// Disable the counter and clear it, then re-enable it
	// This starts our clock at the start of the row.
	LBBO tmp2, timer_ptr, 0, 4
	CLR tmp2, tmp2, 3 // disable counter bit
	SBBO tmp2, timer_ptr, 0, 4 // write it back

	MOV r10, 0
	SBBO r10, timer_ptr, 0xC, 4 // clear the timer

	SET tmp2, tmp2, 3 // enable counter bit
	SBBO tmp2, timer_ptr, 0, 4 // write it back

	// Read the current counter value
	// Should be zero.
	LBBO sleep_counter, timer_ptr, 0xC, 4

	// the hsync pulse starts a bit after the vsync
	WAITNS(8000, wait_start)

	// the hsync keeps running at normal speed for 1.2 ms
	// 28 frames
	MOV row, 29
	VSYNC_LOOP:
		HSYNC_LO
		WAITNS(21500, wait_hsync1)
		HSYNC_HI
		WAITNS(21500, wait_hsync2)
		SUB row, row, 1
		QBNE hsync_skip, row, 25
		VSYNC_HI
		hsync_skip:
		QBNE VSYNC_LOOP, row, 0
		

        MOV row, 384

	ROW_LOOP:
		// start the new row
		HSYNC_LO

		// Load the sixteen pixels worth of data outputs into
		// This takes about 250 ns
		LBBO pixel_data, data_addr, 0, 512/8

		WAITNS(11200, wait_hsync)
		MOV col, 0


#define OUTPUT_COLUMN(rN) \
		QBBC clr_##rN, rN, col; \
			VIDEO_LO; \
			QBA skip_##rN; \
	col_##rN: ; \
		NOP; \
		NOP; \
		NOP; NOP; NOP; NOP; \
		QBBC clr_##rN, rN, col; \
			VIDEO_LO; \
			QBA skip_##rN; \
		clr_##rN:; \
			NOP; \
			VIDEO_HI; \
		skip_##rN:; \
		ADD col, col, 1; \
		AND col, col, 31; \
		QBNE col_##rN, col, 0; \
		NOP; NOP; NOP; NOP; \

		OUTPUT_COLUMN(r10); NOP; NOP;
		OUTPUT_COLUMN(r11); NOP; NOP;
		OUTPUT_COLUMN(r12); NOP; NOP;
		OUTPUT_COLUMN(r13);
		HSYNC_HI

		OUTPUT_COLUMN(r14); NOP; NOP;
		OUTPUT_COLUMN(r15); NOP; NOP;
		OUTPUT_COLUMN(r16); NOP; NOP;
		OUTPUT_COLUMN(r17); NOP; NOP;
		OUTPUT_COLUMN(r18); NOP; NOP;
		OUTPUT_COLUMN(r19); NOP; NOP;
		OUTPUT_COLUMN(r20); NOP; NOP;
		OUTPUT_COLUMN(r21); NOP; NOP;
		OUTPUT_COLUMN(r22); NOP; NOP;
		OUTPUT_COLUMN(r23); NOP; NOP;
		OUTPUT_COLUMN(r24); NOP; NOP;
		OUTPUT_COLUMN(r25); NOP; NOP;

		// Always return the video pin to a high state
		VIDEO_HI

		// Increment our data_offset to point to the next row
		ADD data_addr, data_addr, 512/8

                SUB row, row, 1

/*
		QBNE hold_vsync_lo, row, 150
		VSYNC_HI
		hold_vsync_lo:
*/

		// Be sure that we wait for the right length of time
		// Force each line to be 50 usec
		WAITNS(34000, wait_hsync_end)

                QBNE ROW_LOOP, row, 0
		WAITNS(5000, wait_hsync_end2)
	QBA READ_LOOP
	
EXIT:
#ifdef AM33XX
    // Send notification to Host for program completion
    MOV R31.b0, PRU0_ARM_INTERRUPT+16
#else
    MOV R31.b0, PRU0_ARM_INTERRUPT
#endif

    HALT

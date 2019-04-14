/*
 * "Small Hello World" example.
 *
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example
 * designs. It requires a STDOUT  device in your system's hardware.
 *
 * The purpose of this example is to demonstrate the smallest possible Hello
 * World application, using the Nios II HAL library.  The memory footprint
 * of this hosted application is ~332 bytes by default using the standard
 * reference design.  For a more fully featured Hello World application
 * example, see the example titled "Hello World".
 *
 * The memory footprint of this example has been reduced by making the
 * following changes to the normal "Hello World" example.
 * Check in the Nios II Software Developers Manual for a more complete
 * description.
 *
 * In the SW Application project (small_hello_world):
 *
 *  - In the C/C++ Build page
 *
 *    - Set the Optimization Level to -Os
 *
 * In System Library project (small_hello_world_syslib):
 *  - In the C/C++ Build page
 *
 *    - Set the Optimization Level to -Os
 *
 *    - Define the preprocessor option ALT_NO_INSTRUCTION_EMULATION
 *      This removes software exception handling, which means that you cannot
 *      run code compiled for Nios II cpu with a hardware multiplier on a core
 *      without a the multiply unit. Check the Nios II Software Developers
 *      Manual for more details.
 *
 *  - In the System Library page:
 *    - Set Periodic system timer and Timestamp timer to none
 *      This prevents the automatic inclusion of the timer driver.
 *
 *    - Set Max file descriptors to 4
 *      This reduces the size of the file handle pool.
 *
 *    - Check Main function does not exit
 *    - Uncheck Clean exit (flush buffers)
 *      This removes the unneeded call to exit when main returns, since it
 *      won't.
 *
 *    - Check Don't use C++
 *      This builds without the C++ support code.
 *
 *    - Check Small C library
 *      This uses a reduced functionality C library, which lacks
 *      support for buffering, file IO, floating point and getch(), etc.
 *      Check the Nios II Software Developers Manual for a complete list.
 *
 *    - Check Reduced device drivers
 *      This uses reduced functionality drivers if they're available. For the
 *      standard design this means you get polled UART and JTAG UART drivers,
 *      no support for the LCD driver and you lose the ability to program
 *      CFI compliant flash devices.
 *
 *    - Check Access device drivers directly
 *      This bypasses the device file system to access device drivers directly.
 *      This eliminates the space required for the device file system services.
 *      It also provides a HAL version of libc services that access the drivers
 *      directly, further reducing space. Only a limited number of libc
 *      functions are available in this configuration.
 *
 *    - Use ALT versions of stdio routines:
 *
 *           Function                  Description
 *        ===============  =====================================
 *        alt_printf       Only supports %s, %x, and %c ( < 1 Kbyte)
 *        alt_putstr       Smaller overhead than puts with direct drivers
 *                         Note this function doesn't add a newline.
 *        alt_putchar      Smaller overhead than putchar with direct drivers
 *        alt_getchar      Smaller overhead than getchar with direct drivers
 *
 */

#include <stdio.h>
#include "sys/alt_stdio.h"
#include "altera_avalon_performance_counter.h"
#include "system.h"

#define SWAP_ACCELERATOR_0_BASE 0x810d0

#define TIC PERF_RESET(PERFORMANCE_COUNTER_0_BASE); \
	PERF_START_MEASURING(PERFORMANCE_COUNTER_0_BASE); \
	PERF_BEGIN(PERFORMANCE_COUNTER_0_BASE, 1);

#define TOC PERF_END(PERFORMANCE_COUNTER_0_BASE, 1); \
	PERF_STOP_MEASURING(PERFORMANCE_COUNTER_0_BASE);

#define TOC_PRINT perf_print_formatted_report((void*)PERFORMANCE_COUNTER_0_BASE, ALT_CPU_FREQ, 2, "Sum", "Diff");

inline static int shift(int a) {
	int b = 0;
	b |= (a & 0x000000FF) << 24;
	b |= (a & 0xFF000000) >> 24;
	for (int i = 0; i < 8; i++) {
		b |= (a & (1 << (i + 16))) >> (2 * i + 1);
		b |= (a & (1 << (i + 8))) << (15 - i * 2);
	}
	return b;
}

inline void accelerated_shift(unsigned long *buffer, unsigned short length) {
	IOWR_32DIRECT(SWAP_ACCELERATOR_0_BASE, 0, buffer);
	IOWR_32DIRECT(SWAP_ACCELERATOR_0_BASE, 4, length);
	IOWR_32DIRECT(SWAP_ACCELERATOR_0_BASE, 8, 1);
	while ((IORD_32DIRECT(SWAP_ACCELERATOR_0_BASE, 12) & 0x0001) == 0);
}

void correctness_test() {
	int a = 0x11F0A0FF;
	unsigned long buffer[] = { a };

	int res_custom = ALT_CI_SWAP_0(a, 0);
	int res_c = shift(a);
	accelerated_shift(buffer, 1);

	alt_printf("Result from C is 0x%x \n", res_c);
	alt_printf("Result from custom instruction is 0x%x \n", res_custom);
	alt_printf("Result from accelerator is 0x%x \n", buffer[0]);
}

void perfomance_test() {
	int a = 0x11F0A0FF;
	int sample_batches[] = { 1, 100, 1000 };

	for (int ni = 0; ni < 3; ni++) {
		int n_samples = sample_batches[ni];

		// Custom instruction
		TIC
		for (int i = 0; i < n_samples; i++) {
			volatile int res = ALT_CI_SWAP_0(a, 0);
		}
		TOC
		printf("Custom instruction (%d elements)\n", n_samples);
		TOC_PRINT

		// C shift
		TIC
		for (int i = 0; i < n_samples; i++) {
			volatile int res = shift(a);
		}
		TOC
		printf("C function (%d elements)\n", n_samples);
		TOC_PRINT

		// Accelerator
		volatile int buffer[n_samples];
		TIC
		accelerated_shift(buffer, n_samples);
		alt_dcache_flush_all();
		alt_icache_flush_all();
		TOC
		printf("Accelerated shift (%d elements)\n", n_samples);
		TOC_PRINT
	}
}

int main() {
	perfomance_test();

	return 0;
}

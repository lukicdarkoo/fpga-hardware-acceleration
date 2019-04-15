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

void csv_export() {
	unsigned int time_custom;
	unsigned int time_c;
	unsigned int time_accelerator;
	int sample_batches[] = { 100000, 50000, 10000, 5000, 1000, 500, 100, 50, 10, 5, 1 };

	printf("BatchSize,Custom,CFunction,Accelerator\n");
	for (int ni = 0; ni < sizeof(sample_batches) / 4; ni++) {
		int n_samples = sample_batches[ni];

		// Custom shift
		TIC
		for (int i = 0; i < n_samples; i++) {
			volatile int res = ALT_CI_SWAP_0(i, 0);
		}
		TOC
		time_custom = perf_get_total_time(PERFORMANCE_COUNTER_0_BASE);

		// Accelerator
		volatile int buffer[n_samples];
		TIC
		accelerated_shift(buffer, n_samples);
		alt_dcache_flush_all();
		// alt_icache_flush_all();
		TOC
		time_accelerator = perf_get_total_time(PERFORMANCE_COUNTER_0_BASE);

		// C shift
		TIC
		for (int i = 0; i < n_samples; i++) {
			volatile int res = shift(i);
		}
		TOC
		time_c = perf_get_total_time(PERFORMANCE_COUNTER_0_BASE);

		printf("%d,%d,%d,%d\n", n_samples, time_custom, time_c, time_accelerator);
	}
}

int main() {
	csv_export();;

	return 0;
}

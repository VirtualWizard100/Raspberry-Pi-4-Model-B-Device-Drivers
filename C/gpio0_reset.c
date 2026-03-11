#include "gpio.h"

int main() {

	int fd = open("/dev/mem", O_RDWR|O_SYNC);

	if (fd < 0) {
		fprintf(stderr, "Line %d: Couldn't open file\n", (__LINE__ - 3));
		return 1;
	};

	void *gpio_mmap = mmap(NULL, 0x100, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_BASE_ADDRESS);

	if (!gpio_mmap) {
		fprintf(stderr, "Line %d: mmap() failed\n", (__LINE__ - 3));
		return 1;
	};

	volatile unsigned int *gpio_ptr = (volatile unsigned int *) gpio_mmap;

	*gpio_ptr &= ~7;

	*(gpio_ptr + (GPIO0_SET/4)) &= ~3;

	return 0;
};

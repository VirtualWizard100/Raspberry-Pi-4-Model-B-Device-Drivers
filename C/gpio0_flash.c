#include "gpio.h"

int main() {

        int fd = open("/dev/mem", O_RDWR|O_SYNC);

        void *mmap_addr = mmap(NULL, 0x100, PROT_READ|PROT_WRITE, MAP_SHARED, fd, GPIO_BASE_ADDRESS);

        volatile unsigned int *gpio_ptr = (volatile unsigned int *) mmap_addr;

        *gpio_ptr &= ~7;

        *gpio_ptr |= 1;

        while (1) {
                *(gpio_ptr + (GPIO0_SET/4)) |= 1;

                for (volatile long long i = 0; i < 15000000; i++) {
                };

                *(gpio_ptr + (GPIO0_CLR/4)) |= 1;

                for (volatile long long i = 0; i < 15000000; i++) {
                };
        };
};

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>

#define CLOCK_BASE_ADDRESS 0x7e101000UL
#define GP0_CTL (CLOCK_BASE_ADDRESS + 0x70UL)
#define GPIO_BASE_ADDRESS 0xfe200000
#define GPIO0_SET 0x1c
#define GPIO0_CLR 0x28
#define GPFSEL0 GPIO_BASE_ADDRESS

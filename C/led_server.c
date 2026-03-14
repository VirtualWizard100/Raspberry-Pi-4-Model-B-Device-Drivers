#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define SOCKET int

#define BCM2711_PERI_BASE 0xfe000000
#define BASE_GPIO_ADDRESS (BCM2711_PERI_BASE + 0x200000)
#define GPFSEL0 BASE_GPIO_ADDRESS
#define GPIO0_SET 0x1c
#define GPIO0_CLR 0x28

volatile unsigned int *gpio_ptr;

struct command_function {
	char *command;
	void (*function) (SOCKET);
};



void led_on(SOCKET client_socket) {

	*gpio_ptr &= ~7;

	*gpio_ptr |= 0x1;

	*(gpio_ptr + (GPIO0_SET/4)) |= 0x1;

	char *message = "LED is on\n";

	send(client_socket, message, strlen(message), 0);

};



void led_off(SOCKET client_socket) {

	*(gpio_ptr + (GPIO0_CLR/4)) = 1;

	char *message = "LED is off\n";

	send(client_socket, message, strlen(message), 0);

};

unsigned int flash_toggle = 0;

void *flash_logic() {

	while (1) {

		if (flash_toggle == 0) {
			break;
		};

		*(gpio_ptr + (GPIO0_SET/4)) |= 0x1;

		for (int i = 0; i < 10000000; i++) {
		};

		*(gpio_ptr + (GPIO0_CLR/4)) |= 0x1;

		for (int i = 0; i < 10000000; i++) {
		};
	};

};

void led_flash(SOCKET client_socket) {

	*gpio_ptr |= 1;

	if (flash_toggle == 1) {
		send(client_socket, "LED is already flashing\n", 24, 0);
		return;
	};

	flash_toggle = 1;

	pthread_t thread_id;

	pthread_create(&thread_id, NULL, flash_logic, NULL);

	char *message = "LED is flashing\n";

    send(client_socket, message, strlen(message), 0);

	pthread_detach(thread_id);
};

void led_reset(SOCKET client_socket) {

	*(gpio_ptr + (GPIO0_CLR/4)) = 1;

	*gpio_ptr &= ~7;

	char *message = "LED has been reset\n";

	send(client_socket, message, strlen(message), 0);

};



char *get_errno_message() {
	char *errno_message = strerror(errno);

	static char buffer[100];

	sprintf(buffer, "errno(%d): %s", errno, errno_message);

	return buffer;
};

void help(SOCKET client_socket);

static struct command_function command_list[] = {{"ON", &led_on}, {"OFF", &led_off}, {"FLASH", &led_flash}, {"RESET", &led_reset}, {"HELP", &help}};

void help(SOCKET client_socket) {

	char buffer[100];

	send(client_socket, "Commands:\n", 12, 0);

	for (int i = 0; i < (sizeof(command_list) / sizeof(struct command_function)); i++) {
		sprintf(buffer, "\t%s\n", command_list[i].command);

		send(client_socket, buffer, strlen(buffer), 0);

		memset(buffer, 0, sizeof(buffer));
	};

};

int main() {

	int fd = open("/dev/mem", O_RDWR|O_DSYNC);

	if (fd < 0) {
		fprintf(stderr, "Line %d: Failed to open /dev/mem, are you root?\n", (__LINE__ - 1));
		return 1;
	};

	void *gpio_mmap = mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, BASE_GPIO_ADDRESS);

	if (gpio_mmap < 0) {
		fprintf(stderr, "Line %d: mmap() failed\n", (__LINE__ - 1));
		return 1;
	};

	gpio_ptr = (volatile unsigned int *) gpio_mmap;

	struct addrinfo hints;
	struct addrinfo *bind_address;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(0, "8080", &hints, &bind_address) < 0) {
		fprintf(stderr, "Line %d: %s\n", (__LINE__ - 1), get_errno_message());
		return 1;
	};

	char address_buffer[100];
	char service_buffer[100];

	if (getnameinfo(bind_address->ai_addr, bind_address->ai_addrlen, address_buffer, sizeof(address_buffer), service_buffer, sizeof(service_buffer), NI_NUMERICHOST|NI_NUMERICSERV) < 0) {
		fprintf(stderr, "Line %d: getnameinfo() failed\n", (__LINE__ - 1));
		return 1;
	};

	printf("Local Address: %s\nService: %s\n\n", address_buffer, service_buffer);

	SOCKET s = socket(bind_address->ai_family, bind_address->ai_socktype, bind_address->ai_protocol);

	if (s < 0) {
		fprintf(stderr, "Line %d: %s\n", (__LINE__ - 1), get_errno_message());
		return 1;
	};

	int sock_toggle = 1;

        if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &sock_toggle, sizeof(sock_toggle)) < 0) {
                fprintf(stderr, "Line %d: %s\n", (__LINE__ - 1), get_errno_message());
        	return 1;
        };

	if (bind(s, bind_address->ai_addr, bind_address->ai_addrlen) < 0) {
		fprintf(stderr, "Line %d: %s\n", (__LINE__ - 1), get_errno_message());
		return 1;
	};

	if (listen(s, 10) < 0) {
		fprintf(stderr, "Line %d: %s\n", (__LINE__ - 1), get_errno_message());
		return 1;
	};

	freeaddrinfo(bind_address);

	fd_set master;
	FD_ZERO(&master);
	FD_SET(s, &master);
	FD_SET(0, &master);

	SOCKET max_socket = s;

	while (1) {

		fd_set reads = master;

		if (select(max_socket+1, &reads, 0, 0, 0) < 0) {
			fprintf(stderr, "Line %d: %s\n", (__LINE__ - 1), get_errno_message());
			return 1;
		};

		if (FD_ISSET(0, &reads)) {
			goto end;
		};

		for (SOCKET i = 1; i <= max_socket; i++) {
			if (FD_ISSET(i, &reads)) {
				if (i == s) {
					struct sockaddr_storage client_addr;
					socklen_t client_len = sizeof(client_addr);

					SOCKET client_socket = accept(s, (struct sockaddr *) &client_addr, &client_len);

					if (client_socket < 0) {
						fprintf(stderr, "Line %d: %s\n", (__LINE__ - 1), get_errno_message());
						return 1;
					};

					FD_SET(client_socket, &master);

					if (client_socket > max_socket) {
						max_socket = client_socket;
					};

					char client_address[100];

					if (getnameinfo((struct sockaddr *) &client_addr, client_len, client_address, sizeof(client_address), 0, 0, NI_NUMERICHOST) < 0) {
						fprintf(stderr, "Line %d: %s\n", (__LINE__ - 1), get_errno_message());
						return 1;
					};

					char *message = "Welcome to the LED server\nHELP for help\n";

					send(client_socket, message, strlen(message), 0);

					printf("New connection from: %s on socket %d\n", client_address, client_socket);

				} else {
					char buffer[1024];

					memset(buffer, 0, sizeof(buffer));

					int bytes_recieved = recv(i, buffer, sizeof(buffer), 0);

					if (bytes_recieved < 1) {
						printf("Client on socket %d disconnected\n", i);
						FD_CLR(i, &master);
						close(i);
						break;
					};

					for (int c = 0; c < strlen(buffer); c++) {
						buffer[c] = toupper(buffer[c]);
					};



					int c;

					for (c = 0; c < (sizeof(command_list) / sizeof(struct command_function)); c++) {
						if (strstr(buffer, command_list[c].command)) {
							if (buffer[strlen(command_list[c].command)] == '\n') {
								if ((!strstr(command_list[c].command, "FLASH")) && flash_toggle == 1) {
									flash_toggle = 0;
								};
								command_list[c].function(i);
								break;
							};
						};
					};

					if (c == (sizeof(command_list) / sizeof(struct command_function))) {
						char message[(17 + strlen(buffer))];
						sprintf(message, "Unknown command %s\n", buffer);
						send(i, message, strlen(message), 0);
					};

				};
			};
		};

	};

end:

	close(s);

	printf("Server closed\n");

	return 0;
};

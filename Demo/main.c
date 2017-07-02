//main.c
//authored by Jared Hull
//
//tasks 1 and 2 blink the ACT LED
//main initialises the devices and IP tasks

#include <FreeRTOS.h>
#include <task.h>

#include "interrupts.h"
#include "gpio.h"
#include "video.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

void task1() {
	int i = 0;
	while(1) {
		i++;
		SetGpio(47, 1);
		vTaskDelay(200);
	}
}

void task2() {
	int i = 0;
	while(1) {
		i++;
		vTaskDelay(100);
		SetGpio(47, 0);
		vTaskDelay(100);
	}
}

//server task does not work in this build, it fails to accept a connection
void serverTask(){
	//setup a socket structure
	Socket_t listenfd = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
	struct freertos_sockaddr server;
	server.sin_port = FreeRTOS_htons(4242);
	server.sin_addr = FreeRTOS_inet_addr("192.168.1.42");

	FreeRTOS_bind(listenfd, (struct freertos_sockaddr*)&server, sizeof(server));
	FreeRTOS_listen(listenfd, 3);

	while(1){
loaded = 2;
		Socket_t connectfd = FreeRTOS_accept(listenfd, (struct freertos_sockaddr*)&server, sizeof(server));
loaded = 1;
println("    accepted", 0xFFFFFFFF);
		char recieved[9000];
		FreeRTOS_recv(connectfd, recieved, 9000, 0);
		println(recieved, 0xFFFFFFFF);

		char *message = "Forty-Two";
		FreeRTOS_send(connectfd, message, sizeof(message), 0);
		println(message, 0xFFFFFFFF);

		FreeRTOS_shutdown(connectfd, FREERTOS_SHUT_RDWR);
	}
	FreeRTOS_shutdown(listenfd, FREERTOS_SHUT_RDWR);
}

int main(void) {
	SetGpioFunction(47, 1);			// RDY led

	initFB();
	SetGpio(47, 1);
	//videotest();

	DisableInterrupts();
	InitInterruptController();

	//ensure the IP and gateway match the router settings!
	const unsigned char ucIPAddress[ 4 ] = {192, 168, 1, 42};
	const unsigned char ucNetMask[ 4 ] = {255, 255, 255, 0};
	const unsigned char ucGatewayAddress[ 4 ] = {192, 168, 1, 1};
	const unsigned char ucDNSServerAddress[ 4 ] = {8, 8, 8, 8};
	const unsigned char ucMACAddress[ 6 ] = {0xB8, 0x27, 0xEB, 0x19, 0xAD, 0xA7};
	FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);

	//xTaskCreate(serverTask, "server", 128, NULL, 0, NULL);

	xTaskCreate(task1, "LED_0", 128, NULL, 0, NULL);
	xTaskCreate(task2, "LED_1", 128, NULL, 0, NULL);

	//set to 0 for no debug, 1 for debug, or 2 for GCC instrumentation (if enabled in config)
	loaded = 1;

	vTaskStartScheduler();

	/*
	 *	We should never get here, but just in case something goes wrong,
	 *	we'll place the CPU into a safe loop.
	 */
	while(1) {
		;
	}
}

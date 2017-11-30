// main.c
// Authored by Jared Hull
// Modified by Roope Lindström & Emil Pirinen
//
// Main initialises the devices
// Tasks simulate car lights

#include <FreeRTOS.h>
#include <task.h>
#include <string.h>

#include "interrupts.h"
#include "gpio.h"
#include "video.h"
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"

// Only for debug, normally should not include private header
#include "FreeRTOS_IP_Private.h"

#define SERVER_PORT 8080

#define ACCELERATE_LED_GPIO 23
#define BRAKE_LED_GPIO 24
#define CLUTCH_LED_GPIO 25

#define TICK_LENGTH 500
#define DELAY_SHORT 100

#define MAX_VELOCITY 280
#define CLUTCH_THRESHOLD 5

#define FONT_SCALE 4

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef long int32_t;

int accState = 0;
int brakeState = 1;
int clutchState = 1;
int blinkerState = 0;
int reverseState = 0;

unsigned velocity = 0;
unsigned rpm = 8;
int gear = 0;

void updateGpio() {
    while (1) {
        SetGpio(ACCELERATE_LED_GPIO, accState);
        SetGpio(BRAKE_LED_GPIO, brakeState);
        SetGpio(CLUTCH_LED_GPIO, clutchState);
        vTaskDelay(DELAY_SHORT);
    }
}

void updateLights() {
    int accPrev = -1;
    int brakePrev = -1;
    int clutchPrev = -1;
    int blinkerCycle = 0;

    int row = 5;

    while (1) {
        if (accState != accPrev) {
            drawSquare(10, row, 2, FONT_SCALE, accState ? ACCEL_ON : DARK_TEXT);
        }

        if (brakeState != brakePrev) {
            drawSquare(7, row, 2, FONT_SCALE, brakeState ? BRAKE_ON : DARK_TEXT);            
        }

        if (clutchState != clutchPrev) {
            drawSquare(4, row, 2, FONT_SCALE, clutchState ? CLUTCH_ON : DARK_TEXT);            
        }

        if (!blinkerState) {
            drawSquare(1, row, 2, FONT_SCALE, DARK_TEXT);
            drawSquare(13, row, 2, FONT_SCALE, DARK_TEXT);
        }
        else if (blinkerState == 1) {
            drawSquare(1, row, 2, FONT_SCALE, blinkerCycle ? BLINKER_ON : DARK_TEXT);
            drawSquare(13, row, 2, FONT_SCALE, DARK_TEXT);
        }
        else if (blinkerState == 2) {
            drawSquare(1, row, 2, FONT_SCALE, DARK_TEXT);
            drawSquare(13, row, 2, FONT_SCALE, blinkerCycle ? BLINKER_ON : DARK_TEXT);
        }

        accPrev = accState;
        brakePrev = brakeState;
        clutchPrev = clutchState;
        blinkerCycle = blinkerCycle ? 0 : 1;
        vTaskDelay(TICK_LENGTH);
    }
}

void intToString(unsigned n, uint8_t *str) {
    uint8_t *empty = " ";

    int i1 = n / 100;
    int i2 = (n % 100) / 10;
    int i3 = n % 10;

    uint8_t c1 = i1 + '0';
    uint8_t c2 = i2 + '0';
    uint8_t c3 = i3 + '0';

    uint8_t *s1 = &c1;
    uint8_t *s2 = &c2;
    uint8_t *s3 = &c3;

    strncat(str, i1 ? s1 : empty, 1);
    strncat(str, (i2 || i1) ? s2 : empty, 1);
    strncat(str, s3, 1);
}

void updateVelocity() {
    while (1) {
        if (accState && velocity < MAX_VELOCITY) velocity++;
        else if (brakeState && velocity > 0) velocity--;

        clutchState = velocity < CLUTCH_THRESHOLD ? 1 : 0;
        vTaskDelay(TICK_LENGTH);
    }
}

void printVelocity(uint8_t *str) {
    strcat(str, "Velocity: ");
    intToString(velocity, str);
    strcat(str, " km/h");
    drawStringScaled(str, 1, 1, WHITE_TEXT, FONT_SCALE);
    strcpy(str, "");
}

void updateGear() {
    while (1) {
        if (reverseState) gear = -1;
        else if (!velocity) gear = 0;
        else if (velocity < 15) gear = 1;
        else if (velocity < 30) gear = 2;
        else if (velocity < 45) gear = 3;
        else if (velocity < 80) gear = 4;
        else if (velocity < 120) gear = 5;
        else gear = 6;
        vTaskDelay(TICK_LENGTH);
    }
}

void updateRPM() {
    while (1) {
        rpm = 15;
        vTaskDelay(TICK_LENGTH);
    }
}

void *gearToString(uint8_t *str) {
    uint8_t g = (gear + '0');
    if (gear < 0) g = 'R';
    else if (!gear) g = 'P';
    strncat(str, &g, 1);
}

void printRPM(uint8_t *str) {
    strcat(str, "RPM: ");
    intToString(rpm, str);  
    strcat(str, " x100");
    drawStringScaled(str, 6, 2, WHITE_TEXT, FONT_SCALE);
    strcpy(str, "");
}

void printGear(uint8_t *str) {
    strcat(str, "Gear:   ");
    gearToString(str);
    drawStringScaled(str, 5, 3, WHITE_TEXT, FONT_SCALE);
    strcpy(str, "");
}

void printInfo() {
    drawStringScaled("reverse / forward", 1, 11, WHITE_TEXT, FONT_SCALE);
    drawStringScaled("accel / brake", 1, 12, WHITE_TEXT, FONT_SCALE);
    drawStringScaled("left / right", 1, 13, WHITE_TEXT, FONT_SCALE);
    drawStringScaled("blinker", 1, 14, WHITE_TEXT, FONT_SCALE);
}

void driveTask() {
    vTaskDelay(TICK_LENGTH);
    uint8_t *str = malloc(256);
    drawVertDivider(0, 2);
    printInfo();

    xTaskCreate(updateLights, "lights", 128, NULL, 0, NULL);
    xTaskCreate(updateVelocity, "velocity", 128, NULL, 0, NULL);
    xTaskCreate(updateGear, "gear", 128, NULL, 0, NULL);
    xTaskCreate(updateRPM, "rpm", 128, NULL, 0, NULL);

    int prevVelocity = -1;
    int prevRPM = -1;
    int prevGear = -1;

    while (1) {
        if (velocity != prevVelocity) {
            clearScreen(11, 1, 3, FONT_SCALE);
            printVelocity(str);
        }

        if (rpm != prevRPM) {
            clearScreen(11, 2, 3, FONT_SCALE);
            printRPM(str);            
        }
        
        if (gear != prevGear) {
            clearScreen(11, 3, 3, FONT_SCALE);
            printGear(str);
        }

        prevVelocity = velocity;
        prevRPM = rpm;
        prevGear = gear;
        vTaskDelay(DELAY_SHORT);
    }
    free(str);
}

int checkCommand(uint8_t *cmd1, uint8_t *cmd2) {
    return !strncmp(cmd1, cmd2, strlen((char *)cmd2));
}

int runCommand(uint8_t *cmd) {
    if (checkCommand(cmd, "accel")) {
        accState = 1;
        brakeState = 0;
    }
    else if (checkCommand(cmd, "brake")) {
        accState = 0;
        brakeState = 1;
    }
    else if (checkCommand(cmd, "left")) {
        blinkerState = 1;
    }
    else if (checkCommand(cmd, "right")) {
        blinkerState = 2;
    }
    else if (checkCommand(cmd, "forward") && !velocity) {
        reverseState = 0;
    }
    else if (checkCommand(cmd, "blinker")) {
        blinkerState = 0;
    }
    else if (checkCommand(cmd, "reverse") && !velocity) {
        reverseState = 1;
    }
    else if (checkCommand(cmd, "exit")) return 0;
    return 1;
}

#undef CREATE_SOCK_TASK
// #define CREATE_SOCK_TASK
#define tcpechoSHUTDOWN_DELAY (pdMS_TO_TICKS(5000))

// Server task works in this build
void serverListenTask()
{
    int status = 0;
    static const portTickType xReceiveTimeOut = portMAX_DELAY;
    const portTickType xDelay8s = pdMS_TO_TICKS(8000UL);
    const portTickType xDelay500ms = pdMS_TO_TICKS(500UL);
    const portBASE_TYPE xBacklog = 4;

    portTickType xTimeOnShutdown;
    uint8_t *pucRxBuffer;
    // For debug
    FreeRTOS_Socket_t *sockt;

    // printAddressConfiguration();
    volatile int times = 10;
    println("Server task starting", BLUE_TEXT);
    if (FreeRTOS_IsNetworkUp())
    {
        println("Network is UP", BLUE_TEXT);
    }
    else
    {
        println("Network is Down", BLUE_TEXT);
        while (!FreeRTOS_IsNetworkUp())
        {
            vTaskDelay(xDelay500ms);
        }
    }
    println("Serv tsk done wait net", BLUE_TEXT);

    Socket_t listen_sock;
    listen_sock = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
    printHex("listenfd sock: ", (int)listen_sock, BLUE_TEXT);
    printHex("IPPROTO_TCP val: ", FREERTOS_IPPROTO_TCP, BLUE_TEXT);
    if (listen_sock == FREERTOS_INVALID_SOCKET)
    {
        println("Socket is NOT valid", GREEN_TEXT);
    }
    else
    {
        println("Socket is valid", BLUE_TEXT);
        sockt = (FreeRTOS_Socket_t *)listen_sock;
        if (sockt->ucProtocol == FREERTOS_IPPROTO_TCP)
        {
            println("Proto is TCP", BLUE_TEXT);
        }
        else
        {
            printHex("Proto NOT TCP: ", sockt->ucProtocol, BLUE_TEXT);
        }
    }

    // Set a time out so accept() will just wait for a connection.
    FreeRTOS_setsockopt(listen_sock,
                        0,
                        FREERTOS_SO_RCVTIMEO,
                        &xReceiveTimeOut,
                        sizeof(xReceiveTimeOut));

    // If I dont set REUSE option, accept will never return
    portBASE_TYPE xReuseSocket = pdTRUE;
    FreeRTOS_setsockopt(listen_sock,
                        0,
                        FREERTOS_SO_REUSE_LISTEN_SOCKET,
                        (void *)&xReuseSocket,
                        sizeof(xReuseSocket));

    struct freertos_sockaddr server, client;
    server.sin_port = FreeRTOS_htons((uint16_t)SERVER_PORT);
    // server.sin_addr = FreeRTOS_inet_addr("192.168.1.9");

    socklen_t cli_size = sizeof(client);
    println("Server task about to bind", BLUE_TEXT);
    status = FreeRTOS_bind(listen_sock, &server, sizeof(server));
    printHex("bind status: ", (int)status, BLUE_TEXT);
    sockt = (FreeRTOS_Socket_t *)listen_sock;
    printHex("Bind port: ", (unsigned int)sockt->usLocPort, BLUE_TEXT);

    println("Server task about to listen", BLUE_TEXT);
    status = FreeRTOS_listen(listen_sock, xBacklog);
    printHex("listen status: ", (int)status, BLUE_TEXT);

    int clients = 0;
    int32_t lBytes, lSent, lTotalSent;

    println("Server task accepting", BLUE_TEXT);
    Socket_t connect_sock = FreeRTOS_accept(listen_sock, (struct freertos_sockaddr *)&client, &cli_size);
    println("Connection accepted", ORANGE_TEXT);

    pucRxBuffer = (uint8_t *)pvPortMalloc(ipconfigTCP_MSS);

    while (1)
    {
        int socketStatus = 1;

        memset(pucRxBuffer, 0x00, ipconfigTCP_MSS);
        if ((lBytes = FreeRTOS_recv(connect_sock, pucRxBuffer, ipconfigTCP_MSS, 0)) > 0)
        {
            socketStatus = runCommand(pucRxBuffer);
            printHex("Chars Received: ", (unsigned int)lBytes, ORANGE_TEXT);
            println(pucRxBuffer, RED_TEXT);

            lSent = 0;
            lTotalSent = 0;

            uint8_t *messageBuffer = "From server: ";
            int32_t messageBytes = sizeof(uint8_t) * strlen((char *)messageBuffer);
            int32_t totalBytes = lBytes + messageBytes;

            uint8_t *totalBuffer = malloc(totalBytes + 1);

            strcpy(totalBuffer, messageBuffer);

            if (socketStatus) strcat(totalBuffer, pucRxBuffer);
            else {
                free(totalBuffer);
		        totalBuffer = malloc(64);
                strcpy(totalBuffer, "From server: Socket closed (user exited)");
		        totalBytes = sizeof(uint8_t) * strlen((char *)totalBuffer);
            }

            while ((lSent >= 0) && (lTotalSent < totalBytes))
            {
                lSent = FreeRTOS_send(connect_sock, totalBuffer, totalBytes - lTotalSent, 0);
                lTotalSent += lSent;
            }
	        free(totalBuffer);
        }

        if (!socketStatus) {
            FreeRTOS_shutdown(connect_sock, FREERTOS_SHUT_RDWR);

            // Wait for the shutdown to take effect, indicated by FreeRTOS_recv()
            // returning an error.
            xTimeOnShutdown = xTaskGetTickCount();

            println("Waiting for shutdown", GREEN_TEXT);

            do
            {
                if (FreeRTOS_recv(connect_sock, pucRxBuffer, ipconfigTCP_MSS, 0) < 0)
                    break;
            } while ((xTaskGetTickCount() - xTimeOnShutdown) < tcpechoSHUTDOWN_DELAY);

            println("Shutdown successful", GREEN_TEXT);

            vPortFree(pucRxBuffer);
            FreeRTOS_closesocket(connect_sock);

            println("Socket closed", GREEN_TEXT);
            break;
        }
    }
}

void serverLoop()
{
    while (1)
    {
        serverListenTask();
    }
}

int main(void)
{
    SetGpioFunction(ACCELERATE_LED_GPIO, 1);
    SetGpioFunction(BRAKE_LED_GPIO, 1);
    SetGpioFunction(CLUTCH_LED_GPIO, 1);

    initFB();

    DisableInterrupts();
    InitInterruptController();

    // Ensure the IP and gateway match the router settings
    const unsigned char ucIPAddress[4] = {10, 10, 206, 100};
    const unsigned char ucNetMask[4] = {255, 255, 255, 0};
    const unsigned char ucGatewayAddress[4] = {10, 10, 206, 1};
    const unsigned char ucDNSServerAddress[4] = {10, 10, 206, 1};
    const unsigned char ucMACAddress[6] = {0xB8, 0x27, 0xEB, 0xA0, 0xE8, 0x54};
    FreeRTOS_IPInit(ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress);

    xTaskCreate(serverLoop, "server", 128, NULL, 0, NULL);
    xTaskCreate(driveTask, "drive", 128, NULL, 0, NULL);
    xTaskCreate(updateGpio, "gpio", 128, NULL, 0, NULL);

    // 0 - No debug
    // 1 - Debug
    // 2 - GCC instrumentation (if enabled in config)
    loaded = 1;

    println("Starting task scheduler", GREEN_TEXT);

    vTaskStartScheduler();

    /*
	 *	We should never get here, but just in case something goes wrong,
	 *	we'll place the CPU into a safe loop.
	 */
    while (1)
    {
        ;
    }
}

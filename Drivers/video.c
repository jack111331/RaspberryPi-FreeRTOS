//video.c
//authored by Jared Hull
//
//basic text debugging using the framebuffer

#include <video.h>
#include <mailbox.h>
#include <5x5_font.h>
char loaded = 0;

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

#define COLUMNS 4
#define SCREEN_SPLITS 2

int SCREEN_WIDTH;
int SCREEN_HEIGHT;

void enablelogging(){ loaded = 1;}

//mailbuffer must be 16 byte aligned for GPU
unsigned int mailbuffer[22] __attribute__((aligned (16)));
unsigned int* framebuffer;

void initFB(){
	//get the display size
	/*mailbuffer[0] = 8 * 4;		//mailbuffer size
	mailbuffer[1] = 0;		//response code
	mailbuffer[2] = 0x00040003;	//test display size
	mailbuffer[3] = 8;		//value buffer size
	mailbuffer[4] = 0;		//Req. + value length (bytes)
	mailbuffer[5] = 0;		//width
	mailbuffer[6] = 0;		//height
	mailbuffer[7] = 0;		//terminate buffer

	//spam mail the GPU until the response code is ok
	int attempts = 0;
	while(mailbuffer[1] != 0x80000000){
		mailboxWrite((int)mailbuffer, 8);
		mailboxRead(8);

		//if it keeps failing, just set to default and move along
		if(attempts >= 5){
			//I don't bother breaking; just fake the response
			mailbuffer[1] = 0x80000000;
			mailbuffer[5] = 640;
			mailbuffer[6] = 480;
		}

		attempts++;
	}*/

	SCREEN_WIDTH = 1920;
	SCREEN_HEIGHT = 1080;

	mailbuffer[0] = 22 * 4;		//mail buffer size
	mailbuffer[1] = 0;		//response code

	mailbuffer[2] = 0x00048003;	//set phys display
	mailbuffer[3] = 8;		//value buffer size
	mailbuffer[4] = 8;		//Req. + value length (bytes)
	mailbuffer[5] = SCREEN_WIDTH;	//screen x
	mailbuffer[6] = SCREEN_HEIGHT;	//screen y

	mailbuffer[7] = 0x00048004;	//set virtual display
	mailbuffer[8] = 8;		//value buffer size
	mailbuffer[9] = 8;		//Req. + value length (bytes)
	mailbuffer[10] = SCREEN_WIDTH;	//screen x
	mailbuffer[11] = SCREEN_HEIGHT; //screen y

	mailbuffer[12] = 0x0048005;	//set depth
	mailbuffer[13] = 4;		//value buffer size
	mailbuffer[14] = 4;		//Req. + value length (bytes)
	mailbuffer[15] = 32;		//bits per pixel
	//pixel format is ARGB, 0xFF0000FF is blue at full alpha transparency

	mailbuffer[16] = 0x00040001;	//allocate buffer
	mailbuffer[17] = 8;		//value buffer size
	mailbuffer[18] = 4;		//Req. + value length (bytes)
	mailbuffer[19] = 0;		//framebuffer address
	mailbuffer[20] = 0;		//framebuffer size

	mailbuffer[21] = 0;		//terminate buffer

	//spam mail the GPU until the response code is ok
	while(mailbuffer[1] != 0x80000000){
		mailboxWrite((int)mailbuffer, 8);
		mailboxRead(8);
	}

	//https://github.com/raspberrypi/firmware/wiki/Accessing-mailboxes
	//shift FB by 0x40000000 if L2 cache is enabled, or 0xC0000000 if disabled
	framebuffer = (unsigned int*)(mailbuffer[19] - 0xC0000000);
	loaded = 1;
}

__attribute__((no_instrument_function))
void drawPixel(unsigned int x, unsigned int y, int colour) {
    framebuffer[y * SCREEN_WIDTH + x] = colour;
}

__attribute__((no_instrument_function))
void drawRect(unsigned int x1, unsigned int y1, unsigned int x2, unsigned int y2, int colour) {
    unsigned int i, j = 0;
    for(i = x1; i < x2; i++) {
        for(j = y1; j < y2; j++) {
            drawPixel(i, j, colour);
        }
    }
}

//characters are stored in 5x5_font.h as binary (6x8 font)
//there are 6 bytes which describe 8 pixels in a column
//	{0x7c,	0x24,	0x24,	0x24,	0x7c,	0x00}, // A
//						0
//						0
//	1	1	1	1	1	0
//	1				1	0
//	1				1	0
//	1	1	1	1	1	0
//	1				1	0
//	1				1	0
__attribute__((no_instrument_function))
void drawChar(unsigned char c, int x, int y, int colour){
	int i, j;

	//convert the character to an index
	c = c & 0x7F;
	if (c < ' ') {
		c = 0;
	} else {
		c -= ' ';
	}

	//draw pixels of the character
	for (j = 0; j < CHAR_WIDTH; j++) {
		for (i = 0; i < CHAR_HEIGHT; i++) {
			//unsigned char temp = font[c][j];
			if (font[c][j] & (1<<i)) {
				framebuffer[(y + i) * SCREEN_WIDTH + (x + j)] = colour;
			}
		}
	}
}

__attribute__((no_instrument_function))
void drawCharScaled(unsigned char c, int x, int y, int colour, int scale) {
	int i, j;

	c = c & 0x7F;
	if (c < ' ') {
		c = 0;
	} else {
		c -= ' ';
	}

	for (j = 0; j < CHAR_WIDTH; j++) {
		for (i = 0; i < CHAR_HEIGHT; i++) {
			for (int y2 = 0; y2 < scale; y2++) {
				for (int x2 = 0; x2 < scale; x2++) {
					framebuffer[(y + y2 + (i * scale)) * SCREEN_WIDTH
						+ (x + x2 + (j * scale))]
						= (font[c][j] & (1<<i)) ? colour : BLACK_TEXT;
				}
			}
		}
	}
}

__attribute__((no_instrument_function))
void drawString(const char* str, int x, int y, int colour){
	while (*str) {
		drawChar(*str++, x, y, colour);
		x += CHAR_WIDTH;
	}
}

__attribute__((no_instrument_function))
void drawStringScaled(const char* str, int x, int y, int colour, int scale) {
	int xOffset = (SCREEN_WIDTH / SCREEN_SPLITS);
	x *= (CHAR_WIDTH * scale);
	y *= (CHAR_HEIGHT * scale);

	while (*str) {
		drawCharScaled(*str++, x + xOffset, y, colour, scale);
		x += CHAR_WIDTH * scale;
	}
}

__attribute__((no_instrument_function))
void clearScreen(int x, int y, int n, int scale) {
	int xOffset = (SCREEN_WIDTH / SCREEN_SPLITS);	
	x *= (CHAR_WIDTH * scale);
	x += xOffset;
	y *= (CHAR_HEIGHT * scale);
	int x2 = x + n * (CHAR_WIDTH * scale);

	drawRect(x, y, x2, y + (CHAR_HEIGHT * scale), BLACK_TEXT);
}

void drawSquare(int x, int y, int size, int scale, int colour) {
	int xOffset = (SCREEN_WIDTH / SCREEN_SPLITS);
	x *= (CHAR_WIDTH * scale);
	x += xOffset;
	y *= (CHAR_HEIGHT * scale);
	size *= (CHAR_WIDTH * scale);
	int x2 = x + size;
	int y2 = y + size;

	drawRect(x, y, x2, y2, colour);
}

__attribute__((no_instrument_function))
void drawVertDivider(int x, int width) {
	x += (SCREEN_WIDTH / SCREEN_SPLITS);
	drawRect(x, 0, x + width, SCREEN_HEIGHT, GRAY_TEXT);
}

int position_x = 0;
int position_y = 0;
__attribute__((no_instrument_function))
void println(const char* message, int colour){
	if(loaded == 0) return; //if video isn't loaded don't bother

	int nFlags;
	__asm volatile ("mrs %0, cpsr" : "=r" (nFlags));
	char s_bWereEnabled = nFlags & 0x80 ? 0 : 1; 
	if(s_bWereEnabled) __asm volatile ("cpsid i" : : : "memory");

	drawString(message, position_x, position_y, colour);
	position_y = position_y + CHAR_HEIGHT + 1;
	if(position_y >= SCREEN_HEIGHT){
		if(position_x + 2 * ((SCREEN_WIDTH / SCREEN_SPLITS) / COLUMNS) > (SCREEN_WIDTH / SCREEN_SPLITS)){

			volatile int* timeStamp = (int*)0x3f003004;
			int stop = *timeStamp + 5000 * 1000;
			while (*timeStamp < stop) __asm__("nop");

			for (int y = 0; y < SCREEN_HEIGHT; y++) {
				for (int x = 0; x < (SCREEN_WIDTH / SCREEN_SPLITS); x++) {
					framebuffer[y * SCREEN_WIDTH + x] = BLACK_TEXT;
				}
			}
			position_y = 0;
			position_x = 0;
		}else{
			position_y = 0;
			position_x += (SCREEN_WIDTH / SCREEN_SPLITS) / COLUMNS;
		}
	}

	if(s_bWereEnabled) __asm volatile ("cpsie i" : : : "memory");
}

__attribute__((no_instrument_function))
void printHex(const char* message, int hexi, int colour){
if(loaded == 0) return; //if video isn't loaded don't bother
	char hex[16] = {'0','1','2','3','4','5','6','7',
					'8','9','A','B','C','D','E','F'};
	char m[200];
	int i = 0;
	while (*message){
		m[i] = *message++;
		i++;
	}
	//overwrite the null terminator
	m[i + 0] = hex[(hexi >> 28)&0xF];
	m[i + 1] = hex[(hexi >> 24)&0xF];
	m[i + 2] = hex[(hexi >> 20)&0xF];
	m[i + 3] = hex[(hexi >> 16)&0xF];
	m[i + 4] = hex[(hexi >> 12)&0xF];
	m[i + 5] = hex[(hexi >> 8)&0xF];
	m[i + 6] = hex[(hexi >> 4)&0xF];
	m[i + 7] = hex[(hexi >> 0)&0xF];
	m[i + 8] = 0; //null termination
	println(m, colour);
}

void videotest(){
	//This loop turns on every pixel the screen size allows for.
	//If the shaded area is larger or smaller than your screen, 
	//you have under/over scan issues. Add disable_overscan=1 to your config.txt
	for(int x = 0; x < SCREEN_WIDTH * SCREEN_HEIGHT; x++){
		framebuffer[x] = 0xFF111111;
	}

	//division crashes the system here but not in other places it seems?
	drawString("Forty-Two", SCREEN_WIDTH / 2 - 4.5 * CHAR_WIDTH, SCREEN_HEIGHT / 2 + CHAR_HEIGHT / 2, 0xFF00FF00);
}

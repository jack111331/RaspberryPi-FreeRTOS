#ifndef _VIDEO_H_
#define _VIDEO_H_
char loaded;
void enablelogging();
void initFB();
void drawChar(unsigned char c, int x, int y, int colour);
void drawString(const char* str, int x, int y, int colour);
void println(const char* message, int colour);
void printHex(const char* message, int hexi, int colour);
void videotest();
void dumpDebug();

#define WHITE_TEXT     0xFFFFFFFF
#define BLACK_TEXT     0xFF000000
#define GRAY_TEXT      0xFFE9E9E9
#define RED_TEXT       0xFFFF0000
#define GREEN_TEXT     0xFF00FF00
#define ORANGE_TEXT    0xFFFF9900
#define BLUE_TEXT      0xFF0088FF
#define MAG_TEXT       0xFFFF00FF
#define AQUA_TEXT      0xFF00FFFF
#define VIOLET_TEXT    0xFFCC88CC

#define ACCEL_ON       0xFF00C800
#define ACCEL_OFF      0xFF006400
#define BRAKE_ON       0xFFC80000
#define BRAKE_OFF      0xFF640000
#define CLUTCH_ON      0xFF0000C8
#define CLUTCH_OFF     0xFF000064
#define INDICATOR_ON   0xFFFFCC00
#define INDICATOR_OFF  0xFFFF9900

#endif

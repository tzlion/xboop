
#ifndef MBREDO_PPGB_H
#define MBREDO_PPGB_H

typedef unsigned char		U8;

int initPort(unsigned short basePort, U8 xbooCable, int minDelay, int maxDelay);
void deinitPort();
U8 transferByte(U8 value);

#endif //MBREDO_PPGB_H

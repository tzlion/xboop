
#ifndef PPGB_H
#define PPGB_H

typedef unsigned char U8;

int init(unsigned short basePort, int xbooCable, int minDelay, int maxDelay);
void deinit();
U8 transferByte(U8 value);

#endif //PPGB_H

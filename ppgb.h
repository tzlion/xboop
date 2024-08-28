
#ifndef MBREDO_PPGB_H
#define MBREDO_PPGB_H

typedef unsigned char		U8;

enum {
    STATUS_BUSY				= 0x80,
    STATUS_ACK				= 0x40,
    STATUS_PAPER			= 0x20,
    STATUS_SELECTIN			= 0x10,
    STATUS_ERROR			= 0x08,
    STATUS_NIRQ				= 0x04,
};

enum {
    CTL_MODE_DATAIN			= 0x20,
    CTL_MODE_IRQACK			= 0x10,
    CTL_SELECT				= 0x08,
    CTL_INIT				= 0x04,
    CTL_LINEFEED			= 0x02,
    CTL_STROBE				= 0x01,
};

enum {
    // .... ..cd
    //	c:	clock
    //	d:	data (serial bit)
    D_CLOCK_HIGH	= 0x02,
};

static bool xbooCompat = true;
//unsigned short dataPort = 0xD010;
//unsigned short statusPort = 0xD011;
//unsigned short controlPort = 0xD012;
static unsigned short dataPort = 0x0378;
static unsigned short statusPort = 0x0379;
static unsigned short controlPort = 0x037a;

int init();
void initPort();
void deinitPort();
U8 transferByte(U8 value);

#endif //MBREDO_PPGB_H

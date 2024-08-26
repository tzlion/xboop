#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "windows.h"

// gcc -Wall -o SendSave SendSave.c -lpigpio

//---------------------------------------------------------------------------
uint32_t Spi32(uint32_t w);
uint32_t Spi32Print(uint32_t w, char* msg);
void     Spi32WaitPrint(uint32_t w, uint32_t comp, char* msg);

void     CmdPrint(uint32_t cnt);
void     CmdPut(uint32_t chr);
void     CmdFopen(uint32_t len);
void     CmdFseek(void);
void     CmdFwrite(void);
void     CmdFclose(void);
void     CmdFread(void);
void     CmdFtell(void);

//---------------------------------------------------------------------------
int spi;
DWORD sleep1 = 10;
DWORD sleep2 = 2;

FILE* fpSave;



//======================================================================

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

bool xbooCompat = true;
unsigned short dataPort = 0xD010;
unsigned short statusPort = 0xD011;
unsigned short controlPort = 0xD012;

typedef void(__stdcall *lpOut32)(short, short);
typedef short(__stdcall *lpInp32)(short);
lpOut32 gfpOut32;
lpInp32 gfpInp32;

/******************************************************************************/
unsigned char inportb(unsigned short port)
{
    return gfpInp32(port);
}
void outportb(unsigned short port, unsigned char value)
{
    gfpOut32(port,value);
}

/******************************************************************************/

void writeToGb(U8 value, bool clock)
{
    if (xbooCompat) {
        U8 ctrl = inportb(controlPort) & 0xFC;
        ctrl = ctrl|(!clock);
        ctrl = ctrl|((!value) << 1);
        outportb(controlPort, ctrl);
    } else {
        outportb(dataPort, value|(clock ? D_CLOCK_HIGH : 0));
    }
}

bool readFromGb()
{
    if (xbooCompat) {
        U8 stat = inportb(statusPort);
        return stat&STATUS_ACK;
    } else {
        U8 stat = inportb(statusPort);
        return !(stat&STATUS_BUSY);
    }
}

void initPort()
{
    if (xbooCompat) {
        outportb(controlPort, inportb(controlPort)&(~CTL_MODE_DATAIN));
        writeToGb(1, 1);
        writeToGb(0, 1);
    } else {
        outportb(controlPort, inportb(controlPort)&(~CTL_MODE_DATAIN));
        outportb(dataPort, 0xFF);
        outportb(dataPort, D_CLOCK_HIGH);
    }
}

void deinitPort()
{
    if (xbooCompat) {
        writeToGb(0, 1);
        outportb(controlPort, inportb(controlPort)&(~CTL_MODE_DATAIN));
        writeToGb(1, 1);
    } else {
        outportb(dataPort, D_CLOCK_HIGH);
        outportb(controlPort, inportb(controlPort)&(~CTL_MODE_DATAIN));
        outportb(dataPort, 0xFF);
    }
}

void delayRead()
{
    inportb(dataPort);
}

////////////////////////////////////////////////////////////////////////////////

void lptdelay(int amt)
{
    for(int i=0;i<amt;i++)
        delayRead();
}

U8 gb_sendbyte(U8 value)
{
    U8 read = 0;
    for(int i=7;i>=0;i--) {
        U8 v = (value>>i)&1;

        writeToGb(v, 1);
        writeToGb(v, 0);

        if(readFromGb())
            read |= (1<<i);

        writeToGb(v, 1);
    }
   // lptdelay(64);
    return read;
}

U8 gb_readbyte()
{
    U8 read = 0;
    for(int i=7;i>=0;i--) {
        writeToGb(0, 1);
        writeToGb(0, 0);

        if(readFromGb())
            read |= (1<<i);

        writeToGb(0, 1);
    }
    // delay between bytes
    lptdelay(64);
    return read;
}

//======================================================================



//---------------------------------------------------------------------------
int main(void)
{
    HINSTANCE hInpOutDll;
    hInpOutDll = LoadLibrary("inpout32.dll");
    if (hInpOutDll != NULL) {
        gfpOut32 = (lpOut32)GetProcAddress(hInpOutDll, "Out32");
        gfpInp32 = (lpInp32)GetProcAddress(hInpOutDll, "Inp32");
    } else {
        printf("Unable to load inpout32.dll\n");
        return -1;
    }

	FILE *fp = fopen("VFDump_mb.gba", "rb");

	if(fp == NULL)
	{
		printf("Err: Can't open file\n");
		return 1;
	}

	fseek(fp, 0L, SEEK_END);
	long fsize = (ftell(fp) + 0x0f) & 0xfffffff0;

	if(fsize > 0x40000)
	{
		printf("Err: Max file size 256kB\n");
		return 2;
	}

	fseek(fp, 0L, SEEK_SET);
	long fcnt = 0;

	uint32_t r, w, w2;
	uint32_t i, bit;

	/*if(gpioInitialise() < 0)
	{
		return 3;
	}

	spi = spiOpen(0, 50000, 3);

	if(spi < 0)
	{
		return 4;
	}*/

    initPort();

	Spi32WaitPrint(0x00006202, 0x72026202, "Looking for GBA");

	r = Spi32Print(0x00006202, "Found GBA");
	r = Spi32Print(0x00006102, "Recognition OK");

	printf("                       ; Send Header\n");
	for(i=0; i<=0x5f; i++)
	{
		w = getc(fp);
		w = getc(fp) << 8 | w;
		fcnt += 2;

		r = Spi32(w);
	}

	r = Spi32Print(0x00006200, "Transfer of header data complete");
	r = Spi32Print(0x00006202, "Exchange master/slave info again");

	r = Spi32Print(0x000063d1, "Send palette data");
	r = Spi32Print(0x000063d1, "Send palette data, receive 0x73hh****");

	uint32_t m = ((r & 0x00ff0000) >>  8) + 0xffff00d1;
	uint32_t h = ((r & 0x00ff0000) >> 16) + 0xf;

	r = Spi32Print((((r >> 16) + 0xf) & 0xff) | 0x00006400, "Send handshake data");
	r = Spi32Print((fsize - 0x190) / 4, "Send length info, receive seed 0x**cc****");

	uint32_t f = (((r & 0x00ff0000) >> 8) + h) | 0xffff0000;
	uint32_t c = 0x0000c387;


	printf("                       ; Send encrypted data\n");

	while(fcnt < fsize)
	{
		w = getc(fp);
		w = getc(fp) <<  8 | w;
		w = getc(fp) << 16 | w;
		w = getc(fp) << 24 | w;

		w2 = w;

		for(bit=0; bit<32; bit++)
		{
			if((c ^ w) & 0x01)
			{
				c = (c >> 1) ^ 0x0000c37b;
			}
			else
			{
				c = c >> 1;
			}

			w = w >> 1;
		}

		m = (0x6f646573 * m) + 1;
		Spi32(w2 ^ ((~(0x02000000 + fcnt)) + 1) ^m ^0x43202f2f);

		fcnt = fcnt + 4;
	}
	fclose(fp);

	for(bit=0; bit<32; bit++)
	{
		if((c ^ f) & 0x01)
		{
			c =( c >> 1) ^ 0x0000c37b;
		}
		else
		{
			c = c >> 1;
		}

		f = f >> 1;
	}

	Spi32WaitPrint(0x00000065, 0x00750065, "Wait for GBA to respond with CRC");

	r = Spi32Print(0x00000066, "GBA ready with CRC");
	r = Spi32Print(c,          "Let's exchange CRC!");

//	printf("CRC ...hope they match!\n");
	printf("MulitBoot done\n\n");


	// init
	/*do
	{

        Sleep(sleep1);
		r = Spi32(0);

	} while(r != 0x50525406);*/


	// select cmd
	for(;;)
	{
		switch(r & 0xffffff00)
		{
		// PRINT_CMD
		case 0x50525400:
			CmdPrint(r & 0xff);
			break;

		// DPUTC_CMD
		case 0x44505400:
			CmdPut(r & 0xff);
			break;

		// GETCH_CMD
		case 0x47544300:
			// no USE
			break;

		// FOPEN_CMD
		case 0x4F504E00:
			CmdFopen(r & 0xff);
			break;

		// FSEEK_CMD
		case 0x46534B00:
			CmdFseek();
			break;

		// FWRITE_CMD
		case 0x46575200:
			CmdFwrite();
			break;

		// FCLOSE_CMD
		case 0x434C5300:
			CmdFclose();
			break;

		// FREAD_CMD
		case 0x46524400:
			CmdFread();
			break;

		// FTELL_CMD
		case 0x46544C00:
			CmdFtell();
			break;

		default:
//			printf("%08x\n", r);
			break;
		}

        Sleep(sleep1);
		r = Spi32(0);
	}
}
//---------------------------------------------------------------------------
void CmdPrint(uint32_t cnt)
{
	uint32_t i, r;
	char c;

	for(i=0; i<cnt; i++)
	{
		if(i % 4 == 0)
		{
			r = Spi32(0);
            Sleep(sleep2);
		}

		c = r & 0xff;

		if((c >= 0x20 && c <= 0x7E) || c == 0x0D || c == 0x0A)
		{
			printf("%c", c);
            Sleep(sleep1);
		}

		r >>= 8;
	}
}
//---------------------------------------------------------------------------
void CmdPut(uint32_t chr)
{
	printf("%c", chr);
    Sleep(sleep1);
}
//---------------------------------------------------------------------------
void CmdFopen(uint32_t len)
{
	char fname[80];
	char ftype[3];

	memset(fname, 0x00, sizeof(fname));
	memset(ftype, 0x00, sizeof(ftype));

	uint32_t i, r;

	for(i=0; i<len; i++)
	{
		if(i % 4 == 0)
		{
			r = Spi32(0);
		}

		fname[i] = r & 0xff;
		r >>= 8;
	}

	r = Spi32(0);
	ftype[0] = (r     ) & 0xff;
	ftype[1] = (r >> 8) & 0xff;

//	printf("fopen %s %s\n", fname, ftype);
	fpSave = fopen(fname, ftype);

	Spi32(0);
}
//---------------------------------------------------------------------------
void CmdFseek(void)
{
	uint32_t offset = Spi32(0);
	uint32_t origin = Spi32(0);

//	printf("fseek %d %d\n", offset, origin);
	fseek(fpSave, offset, origin);
}
//---------------------------------------------------------------------------
void CmdFwrite(void)
{
//	printf("fwrite START\n");

	uint32_t i, r;

	uint32_t size  = Spi32(0);
	uint32_t count = Spi32(0);

	for(i=0; i<size*count; i++)
	{
		if(i % 4 == 0)
		{
			r = Spi32(0);
            Sleep(sleep2);
		}

		fputc(r & 0xff, fpSave);
		r >>= 8;
	}

//	printf("fwrite END %d\n", size*count);
}
//---------------------------------------------------------------------------
void CmdFclose(void)
{
//	printf("fclose\n");
	fclose(fpSave);
}
//---------------------------------------------------------------------------
void CmdFread(void)
{
	uint32_t i, r;
	uint32_t d1, d2, d3, d4;

	uint32_t size  = Spi32(0);
	uint32_t count = Spi32(0);

	for(i=0; i<size*count/4; i++)
	{
		d1 = fgetc(fpSave);
		d2 = fgetc(fpSave);
		d3 = fgetc(fpSave);
		d4 = fgetc(fpSave);

		r = 0;

		r += d4;
		r <<= 8;
		r += d3;
		r <<= 8;
		r += d2;
		r <<= 8;
		r += d1;

		Spi32(r);
        Sleep(sleep2);
	}

//	printf("fread %d %d\n", size, count);
}
//---------------------------------------------------------------------------
void CmdFtell(void)
{
	fseek(fpSave, 0, SEEK_END);
	Spi32(ftell(fpSave));

//	printf("ftell %d\n", ftell(fpSave));
}
//---------------------------------------------------------------------------
uint32_t Spi32(uint32_t w)
{
	char send[4];
	char recv[4];

	send[3] = (w & 0x000000ff);
	send[2] = (w & 0x0000ff00) >>  8;
	send[1] = (w & 0x00ff0000) >> 16;
	send[0] = (w & 0xff000000) >> 24;

    recv[0] = gb_sendbyte(send[0]);
    recv[1] = gb_sendbyte(send[1]);
    recv[2] = gb_sendbyte(send[2]);
    recv[3] = gb_sendbyte(send[3]);

    lptdelay(64);
	//spiXfer(spi, send, recv, 4);

	uint32_t ret = 0;

	ret += ((unsigned char)recv[0]) << 24;
	ret += ((unsigned char)recv[1]) << 16;
	ret += ((unsigned char)recv[2]) <<  8;
	ret += ((unsigned char)recv[3]);

	return ret;
}
//---------------------------------------------------------------------------
uint32_t Spi32Print(uint32_t w, char* msg)
{
	uint32_t r = Spi32(w);

	printf("0x%08x 0x%08x  ; %s\n", r, w, msg);
	return  r;
}
//---------------------------------------------------------------------------
void Spi32WaitPrint(uint32_t w, uint32_t comp, char* msg)
{
	printf("%s 0x%08x\n", msg, comp);
	uint32_t r;

	do
	{
		r = Spi32(w);


	} while(r != comp);
}

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "ppgb.h"

//---------------------------------------------------------------------------
uint32_t Transfer(uint32_t w);
uint32_t TransferPrint(uint32_t w, char* msg);
void     TransferWaitPrint(uint32_t w, uint32_t comp, char* msg);

void     CmdPrint(uint32_t cnt);
void     CmdPut(uint32_t chr);
void     CmdFopen(uint32_t len);
void     CmdFseek(uint32_t handle);
void     CmdFwrite(uint32_t handle);
void     CmdFclose(uint32_t handle);
void     CmdFread(uint32_t handle);
void     CmdFtell(uint32_t handle);

//---------------------------------------------------------------------------

FILE* fpSave[255];
int nextHandle = 0;

void printMessage(const char* message)
{
    printf("%s\n", message);
}

//---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    int xbooCompat = 0;
    unsigned short basePort = 0x378;

    if (argc < 2) {
        printf("Filename required\n");
        return -1;
    }
    if (argc >= 3) {
        int argno;
        for (argno = 2; argno < argc; argno++) {
            if ( memcmp(argv[argno],"-x",2) == 0 ) {
                xbooCompat = 1;
            } else if ( memcmp(argv[argno],"-p",2) == 0 && strlen(argv[argno]) == 6) {
                char portStr[5];
                strncpy(portStr, argv[argno]+2, 5);
                char *endptr;
                long port = strtol(portStr, &endptr, 16);
                if (endptr != portStr && *endptr == '\0') {
                    basePort = (unsigned short)port;
                }
            }
        }
    }

    PPGBInit(basePort, xbooCompat, 2, -1, printMessage);

	FILE *fp = fopen(argv[1], "rb");

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

	TransferWaitPrint(0x00006202, 0x72026202, "Looking for GBA");

	TransferPrint(0x00006202, "Found GBA");
	TransferPrint(0x00006102, "Recognition OK");

	printf("                       ; Send Header\n");
	for(i=0; i<=0x5f; i++)
	{
		w = getc(fp);
		w = getc(fp) << 8 | w;
		fcnt += 2;

		Transfer(w);
	}

	TransferPrint(0x00006200, "Transfer of header data complete");
	TransferPrint(0x00006202, "Exchange master/slave info again");

	TransferPrint(0x000063d1, "Send palette data");
	r = TransferPrint(0x000063d1, "Send palette data, receive 0x73hh****");

	uint32_t m = ((r & 0x00ff0000) >>  8) + 0xffff00d1;
	uint32_t h = ((r & 0x00ff0000) >> 16) + 0xf;

	TransferPrint((((r >> 16) + 0xf) & 0xff) | 0x00006400, "Send handshake data");
	r = TransferPrint((fsize - 0x190) / 4, "Send length info, receive seed 0x**cc****");

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
		Transfer(w2 ^ ((~(0x02000000 + fcnt)) + 1) ^m ^0x43202f2f);

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

	TransferWaitPrint(0x00000065, 0x00750065, "Wait for GBA to respond with CRC");

	TransferPrint(0x00000066, "GBA ready with CRC");
	r = TransferPrint(c,          "Let's exchange CRC!");

	printf("MultiBoot done\n\n");

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
			CmdFseek(r & 0xff);
			break;

		// FWRITE_CMD
		case 0x46575200:
			CmdFwrite(r & 0xff);
			break;

		// FCLOSE_CMD
		case 0x434C5300:
			CmdFclose(r & 0xff);
			break;

		// FREAD_CMD
		case 0x46524400:
			CmdFread(r & 0xff);
			break;

		// FTELL_CMD
		case 0x46544C00:
			CmdFtell(r & 0xff);
			break;

		default:
            // if (r) printf("%08x", r);
			break;
		}

        while (PPGBRawOutputRead());
		r = Transfer(0);
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
			r = Transfer(0);
		}

		c = r & 0xff;

		if((c >= 0x20 && c <= 0x7E) || c == 0x0D || c == 0x0A)
		{
			printf("%c", c);
		}

		r >>= 8;
	}
}
//---------------------------------------------------------------------------
void CmdPut(uint32_t chr)
{
	printf("%c", chr);
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
			r = Transfer(0);
		}

		fname[i] = r & 0xff;
		r >>= 8;
	}

	r = Transfer(0);
	ftype[0] = (r     ) & 0xff;
	ftype[1] = (r >> 8) & 0xff;

    if (nextHandle > 255) {
        printf("Too many file handles opened\n");
        nextHandle = 0;
    }
	fpSave[nextHandle] = fopen(fname, ftype);
    printf("[Opened %s as file %02x, mode %s]\n", fname, nextHandle, ftype);

	Transfer(nextHandle);
    nextHandle++;
}
//---------------------------------------------------------------------------
void CmdFseek(uint32_t handle)
{
	uint32_t offset = Transfer(0);
	uint32_t origin = Transfer(0);

	fseek(fpSave[handle], offset, origin);
}
//---------------------------------------------------------------------------
void CmdFwrite(uint32_t handle)
{
	uint32_t i, r;

	uint32_t size  = Transfer(0);
	uint32_t count = Transfer(0);

	for(i=0; i<size*count; i++)
	{
		if(i % 4 == 0)
		{
			r = Transfer(0);
		}

		fputc(r & 0xff, fpSave[handle]);
		r >>= 8;
	}

	printf("[Wrote %d bytes to file %02x]\n", size*count, handle);
}
//---------------------------------------------------------------------------
void CmdFclose(uint32_t handle)
{
	fclose(fpSave[handle]);
    printf("[Closed file %02x]\n", handle);
}
//---------------------------------------------------------------------------
void CmdFread(uint32_t handle)
{
	uint32_t i, r;
	uint32_t d1, d2, d3, d4;

	uint32_t size  = Transfer(0);
	uint32_t count = Transfer(0);

	for(i=0; i<size*count/4; i++)
	{
		d1 = fgetc(fpSave[handle]);
		d2 = fgetc(fpSave[handle]);
		d3 = fgetc(fpSave[handle]);
		d4 = fgetc(fpSave[handle]);

		r = 0;

		r += d4;
		r <<= 8;
		r += d3;
		r <<= 8;
		r += d2;
		r <<= 8;
		r += d1;

		Transfer(r);
        while (PPGBRawOutputRead());
	}
}
//---------------------------------------------------------------------------
void CmdFtell(uint32_t handle)
{
	fseek(fpSave[handle], 0, SEEK_END);
	Transfer(ftell(fpSave[handle]));
}
//---------------------------------------------------------------------------
uint32_t Transfer(uint32_t w)
{
	char send[4];
	char recv[4];

	send[3] = (w & 0x000000ff);
	send[2] = (w & 0x0000ff00) >>  8;
	send[1] = (w & 0x00ff0000) >> 16;
	send[0] = (w & 0xff000000) >> 24;

    recv[0] = PPGBTransfer(send[0]);
    recv[1] = PPGBTransfer(send[1]);
    recv[2] = PPGBTransfer(send[2]);
    recv[3] = PPGBTransfer(send[3]);

	uint32_t ret = 0;

	ret += ((unsigned char)recv[0]) << 24;
	ret += ((unsigned char)recv[1]) << 16;
	ret += ((unsigned char)recv[2]) <<  8;
	ret += ((unsigned char)recv[3]);

	return ret;
}
//---------------------------------------------------------------------------
uint32_t TransferPrint(uint32_t w, char* msg)
{
	uint32_t r = Transfer(w);

	printf("0x%08x 0x%08x  ; %s\n", r, w, msg);
	return  r;
}
//---------------------------------------------------------------------------
void TransferWaitPrint(uint32_t w, uint32_t comp, char* msg)
{
	printf("%s 0x%08x\n", msg, comp);
	uint32_t r;

	do
	{
		r = Transfer(w);
	} while(r != comp);
}

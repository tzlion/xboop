/**
 * Parallel port communication with Game Boy consoles
 * by taizou, based on code from gblinkdl by Brian Provinciano
 */

#include "ppgb.h"

#include <stdio.h>
#include <sys/time.h>

#ifdef __linux__
#include <sys/io.h>
#include <unistd.h>
#elif defined(__FreeBSD__)
#include <sys/types.h>
#include <machine/cpufunc.h>
#include <machine/sysarch.h>
#elif defined(_WIN32)
#include "windows.h"
#else
#error Unsupported platform
#endif

#ifdef _WIN32
typedef void(__stdcall *lpOut32)(short, short);
typedef short(__stdcall *lpInp32)(short);
lpOut32 gfpOut32;
lpInp32 gfpInp32;
HINSTANCE hInpOutDll;
#endif

U8 xbooCompat;
unsigned short dataPort;
unsigned short statusPort;
unsigned short controlPort;
int delay;

#define STATUS_BUSY 0x80
#define STATUS_ACK 0x40
#define CTL_MODE_DATAIN 0x20
#define D_CLOCK_HIGH 0x02

/******************************************************************************/

unsigned char inportb(unsigned short port)
{
#ifdef _WIN32
    return gfpInp32(port);
#else
    return inb(port);
#endif
}

void outportb(unsigned short port, unsigned char value)
{
#ifdef _WIN32
    gfpOut32(port,value);
#elif defined(__FreeBSD__)
    outb(port, value);
#elif defined(__linux__)
   outb(value, port);
#endif
}

/******************************************************************************/

void writeToGb(U8 value, U8 clock)
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

U8 readFromGb()
{
    if (xbooCompat) {
        U8 stat = inportb(statusPort);
        return stat&STATUS_ACK;
    } else {
        U8 stat = inportb(statusPort);
        return !(stat&STATUS_BUSY);
    }
}

void delayRead()
{
    inportb(dataPort);
}

void initPort()
{
    // don't know if this is needed
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
    // really don't know if this is needed
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

/******************************************************************************/

void lptdelay(int amt)
{
    int i;
    for(i=0;i<amt;i++)
        delayRead();
}

U8 transferByte(U8 value)
{
    U8 read = 0;
    int i;
    for(i=7;i>=0;i--) {
        U8 v = (value>>i)&1;

        writeToGb(v, 1);
        writeToGb(v, 0);

        if(readFromGb())
            read |= (1<<i);

        writeToGb(v, 1);
    }
    lptdelay(delay);
    return read;
}

double get1000xTiming(int transfer)
{
    struct timeval stop, start;
    gettimeofday(&start, NULL);
    int x;
    for(x = 0; x < 1000; x++) {
        if (transfer)
            transferByte(0);
        else
            delayRead();
    }
    gettimeofday(&stop, NULL);
    return (double)(stop.tv_usec - start.tv_usec) / 1000000 + (double)(stop.tv_sec - start.tv_sec);
}

double getMin1000xTiming(int transfer)
{
    double minsecs = -1;
    int x;
    for (x = 0; x < 4; x++) {
        double secs = get1000xTiming(transfer);
        if (minsecs < 0 || secs < minsecs) {
            minsecs = secs;
        }
    }
    return minsecs;
}

void determineDelay(int minDelay, int maxDelay)
{
    printf("Testing timing... ");
    delay = 0;

    double transferTiming = getMin1000xTiming(1);
    double delayTiming = getMin1000xTiming(0);

    double minimumTiming = 0.21;
    if (transferTiming < minimumTiming) {
        double desiredDelayMicrosecs = (minimumTiming - transferTiming) * 1000000;
        double delayTimeMicrosecs = delayTiming * 1000000;
        delay = (int)(desiredDelayMicrosecs / delayTimeMicrosecs);
    }
    if (delay < minDelay) delay = minDelay;
    if (delay > maxDelay && maxDelay >= 0) delay = maxDelay;
    printf("Using delay %i\n", delay);
}

/******************************************************************************/

int init(unsigned short basePort, int xbooCable, int minDelay, int maxDelay)
{
    xbooCompat = xbooCable;
    dataPort = basePort;
    statusPort = basePort + 1;
    controlPort = basePort + 2;
    delay = minDelay;

#ifdef _WIN32
    hInpOutDll = LoadLibrary("inpout32.dll");
    if (hInpOutDll != NULL) {
        gfpOut32 = (lpOut32)GetProcAddress(hInpOutDll, "Out32");
        gfpInp32 = (lpInp32)GetProcAddress(hInpOutDll, "Inp32");
    } else {
        printf("Unable to load inpout32.dll\n");
        return -1;
    }
#endif
#ifdef __linux__
    ioperm(dataPort, 3 , 1);
#elif defined(__FreeBSD__)
    i386_set_ioperm(dataPort, 3, 1);
#endif

    initPort();

    if (minDelay != maxDelay) {
        determineDelay(minDelay, maxDelay);
    }

    return 1;
}

void deinit()
{
    deinitPort();
#ifdef _WIN32
    FreeLibrary(hInpOutDll);
#endif
}

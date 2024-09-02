/**
 * Parallel port communication with Game Boy consoles
 * by taizou, based on code from gblinkdl by Brian Provinciano
 */

#ifndef PPGB_H
#define PPGB_H

typedef unsigned char U8;
typedef void(*PrintFunction)(const char*);

/**
 * Initialise the port. Must be called before transferring any data
 * @param basePort Base port address, standard onboard parallel port is 0x378
 * @param xbooCable Set to 1 if using an Xboo cable or 0 if using a GBlink cable
 * @param minDelay Minimum delay per byte (in number of port accesses) - 2 seems to be good for GBA and 8 for GB
 * @param maxDelay Maximum delay per byte, set to -1 for no max
 * Actual delay will be determined automatically between min and max. Set them both the same for a specific delay
 * @param printFunction function to log port initialisation info, set to null for no logging
 * @return 1 if success, -1 if failed
 */
int init(unsigned short basePort, int xbooCable, int minDelay, int maxDelay, PrintFunction printFunction);

/**
 * Deinitialise the port. Probably not super necessary unless you're going to initialise it again afterwards
 */
void deinit();

/**
 * Transfer a single byte to/from the GB
 * @param value Value to send. If you only want to read, send 00
 * @return Value received from the GB
 */
U8 transferByte(U8 value);

#endif //PPGB_H

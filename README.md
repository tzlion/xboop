Xboop
=====

CLI based simplified reimplementation of Xboo Communicator functionality for loading a GBA multiboot program and 
communicating with a PC via parallel port.

Mostly designed as a PC-side client for [VFDump](https://github.com/tzlion/vfdump), its suitability for other use cases
is not guaranteed.

Requirements
------------

* A PC with the following:
  * A parallel port, either onboard or on an expansion card (PCI/PCIe). USB to parallel adapters will not work
  * Windows or Linux OS
* A Game Boy Advance or Game Boy Advance SP console
* One of the following cables to connect the GBA to the PC's parallel port (you'll probably have to build it yourself):
  * An Xboo cable, this is the one supported by the original Xboo Communicator.  
    [Schematic available here](https://github.com/devkitPro/xcomms/blob/main/docs/xboo_schematic.png)
  * A GBlink cable, this is the one originally supported by GBlinkdl/GBlinkDX.  
    [Schematic available in the GBlinkdl archive](http://web.archive.org/web/20070203014624/http://www.bripro.com/low/hardware/gblinkdl/files/gblinkdl.zip)

Usage
-----

For Windows, just download the executable from the latest release and extract it.  
For Linux, download the source code archive from the latest release, extract it and run `make` in its directory.

Connect your GBA to your PC, power on the GBA and (if a cartridge is inserted) press SELECT+START when the Nintendo logo
appears to prevent it from booting. Then run this program from the command line as follows:
```
xboop YourMultibootRom_mb.gba [options]
```

For Windows you may need to run it as administrator the first time to allow inpout32 to install its driver.  
For Linux you will always need to run it as superuser to allow port access. 

Options may be:
* `-l` for GBlink cable mode, otherwise will use xboo cable mode
* `-pXXXX` to use a specified port address for your parallel port, default is `0378`

If everything has worked it should handshake with the GBA, the GBA will make a chime sound and the Nintendo logo will
appear. Then it will upload your multiboot program to the GBA and after a few seconds it should boot.

Once your program has booted it should then be able to communicate with the PC for file transfer and text output.

Limitations
-----------

* Keyboard input from the PC is not supported. Your program must be controlled entirely from the GBA.
* In general this program was developed and tested only for use with VFDump, compatibility with other multiboot programs
  is not guaranteed.

Credits
-------

Based on SendSave.c from [gba_01_multiboot / Raspberry Pi GBA Loader](https://github.com/akkera102/gba_01_multiboot) by akkera102  
Originally based on work by Ken Kaarvik  
Modified for parallel port usage by taizou  
Parallel port communication code based on gblinkdl by Brian Provinciano  
Thanks to WinterMute for the original Xboo Communicator & for releasing the [source code](https://github.com/devkitPro/xcomms)
(used for reference for some things)  
& thanks to Martin Korth for the [original Xboo implementation](https://problemkaputt.de/gba-xboo.htm)  
Uses [InpOut32](https://www.highrez.co.uk/Downloads/InpOut32/) for port access on Windows
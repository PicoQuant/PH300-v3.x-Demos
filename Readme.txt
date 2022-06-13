PHLib Programming Library for PicoHarp 300 
Version 3.0.0.3
PicoQuant GmbH - October 2015



Introduction

The PicoHarp 300 is a TCSPC system with USB 2.0 interface. 
It requires a 686 class PC with USB 2.0 host controller,
1 GB of memory and at least 1 GHz CPU clock. The PicoHarp software 
is suitable for Windows 7, 8, and 10, including the x64 versions. 


What's new in this version (3.0.0.3)

- a firmware fix to deal with occasional system errors


What's new in version 3.0.0.2

Version 3.0.0.2 of the library package was primarily a bugfix release. 
It corrects two firmware issues: a data corruption in T3 mode with 
maximum binning and a small loss of counts at very short measurement 
times in all modes. There are no functional changes with respect to the 
last major release 3.0. The C# demos have been updated to fix a bug 
relating to calling convention.


What's new in version 3.0.0.1

Version 3.0.0.1 was a bugfix release to deal with firmware errors in some
hardware devices. Only the DLL is updated. The demos remain unchanged.
See below for the last major release changes.


What's new in version 3.0.0.0

- a programmable time offset in the sync input to replace adjustable 
  cable delays (4 ps resolution, ±100ns)
- a programmable time offset in all router channels to tune 
  for relative delay (4 ps resolution, ±8ns)
- a programmable marker holdoff time to suppress marker glitches
- acquisition offset now also applicable in T3 mode
- a routine to obtain info on hardware features
- a routine to obtain hardware debug information
- a routine to disable/enable multistop
- consistent return of success/error instead of data


Installation 

Do not connect the PicoHarp device before installing the software.

Before installation, make sure to backup any work you kept in previous
installation directories and uninstall any older versions of PHLib.
The PHLib package can be distributed on CD/DVD or via download.
The setup distribution file is setup.exe.
If you received the package via download or email, it may be packed in a 
zip-file. Unzip that file and place the distribution setup file in a 
temporary disk folder. Start the installation by running setup.exe before
connecting the PicoHarp device.
On some versions of Windows you may need to log on as administrator.

The setup program will install the programming library including manual, 
and programming demos and the device driver for the PicoHarp. 
Dependent on the version of Windows you may be prompted to confirm the 
driver installation. 


To uninstall the PHLib package, you may need to be logged on as administrator. 
Backup your measurement data before uninstalling.
From the start menu select:  PicoQuant - PH300-PHLib vx.x  >  uninstall.
Alternatively you can use the Control Panel Wizard 'Add/Remove Programs'
(in some Windows versions this Wizard is called 'Software')


Disclaimer

PicoQuant GmbH disclaims all warranties with regard to this software 
and associated documentation including all implied warranties of 
merchantability and fitness. In no case shall PicoQuant GmbH be 
liable for any direct, indirect or consequential damages or any material 
or immaterial damages whatsoever resulting from loss of data, time 
or profits arising from use or performance of this software.


License and Copyright Notice

If you have purchased a license to use the PicoHarp 300 programming library 
software you may use the library to operate the device through custom programs. 
You have not purchased any other rights to the software itself.
The software is protected by copyright and intellectual property laws. 
You may not distribute the software to third parties or reverse engineer, 
decompile or disassemble the software or any part thereof. Copyright 
of this software including manuals belongs to PicoQuant GmbH. No parts 
of it may be reproduced or translated into other languages without written 
consent of PicoQuant GmbH.


Trademark Disclaimer

PicoHarp, HydraHarp TimeHarp and NanoHarp are registered trademarks of 
PicoQuant GmbH. Other products and corporate names appearing in the product 
manuals or in the online documentation may or may not be registered trademarks 
or copyrights of their respective owners. They are used only for identification 
or explanation and to the owner’s benefit, without intent to infringe.


Contact and Support

PicoQuant GmbH
Rudower Chaussee 29
12489 Berlin, Germany
Phone +49 30 6392 6929
Fax   +49 30 6392 6561
email info@picoquant.com
www   http://www.picoquant.com

# Demo for access to PicoHarp 300 Hardware via PHLIB.DLL v 3.0.
# The program performs a measurement based on hard coded settings.
# The resulting data is stored in a binary output file.
#
# Keno Goertz, PicoQuant GmbH, February 2018

import time
import ctypes
from ctypes import byref, POINTER
import sys
import struct

# From phdefin.h
LIB_VERSION = "3.0"
MAXDEVNUM = 8
MODE_T2 = 2
MODE_T3 = 3
TTREADMAX = 131072
FLAG_OVERFLOW = 0x0040
FLAG_FIFOFULL = 0x0003

# Measurement parameters, these are hardcoded since this is just a demo
mode = MODE_T2 # set T2 or T3 here, observe suitable Syncdivider and Range!
binning = 0 # you can change this, meaningful only in T3 mode
offset = 0 # you can change this, meaningful only in T3 mode
tacq = 10000 # Measurement time in millisec, you can change this
syncDivider = 1 # you can change this, observe mode! READ MANUAL!
CFDZeroCross0 = 10 # you can change this (in mV)
CFDLevel0 = 50 # you can change this (in mV)
CFDZeroCross1 = 10 # you can change this (in mV)
CFDLevel1 = 150 # you can change this (in mV)

# Variables to store information read from DLLs
buffer = (ctypes.c_uint * TTREADMAX)()
dev = []
libVersion = ctypes.create_string_buffer(b"", 8)
hwSerial = ctypes.create_string_buffer(b"", 8)
hwPartno = ctypes.create_string_buffer(b"", 8)
hwVersion = ctypes.create_string_buffer(b"", 8)
hwModel = ctypes.create_string_buffer(b"", 16)
errorString = ctypes.create_string_buffer(b"", 40)
resolution = ctypes.c_double()
countRate0 = ctypes.c_int()
countRate1 = ctypes.c_int()
flags = ctypes.c_int()
nactual = ctypes.c_int()
ctcDone = ctypes.c_int()
warnings = ctypes.c_int()
warningstext = ctypes.create_string_buffer(b"", 16384)

phlib = ctypes.CDLL("phlib.dll")

def closeDevices():
    for i in range(0, MAXDEVNUM):
        phlib.PH_CloseDevice(ctypes.c_int(i))
    exit(0)

def stoptttr():
    phlib.PH_StopMeas(ctypes.c_int(dev[0]))
    closeDevices()

def tryfunc(retcode, funcName, measRunning=False):
    if retcode < 0:
        phlib.PH_GetErrorString(errorString, ct.c_int(retcode))
        print("PH_%s error %d (%s). Aborted." % (funcName, retcode,\
              errorString.value.decode("utf-8")))
        if measRunning:
            stoptttr()
        else:
            closeDevices()

phlib.PH_GetLibraryVersion(libVersion)
print("PHLIB.DLL version is %s" % libVersion.value.decode("utf-8"))
if libVersion.value.decode("utf-8") != LIB_VERSION:
    print("Warning: The application was built for version %s" % LIB_VERSION)

outputfile = open("tttrmode.out", "wb+")

print("\nMode              : %d" % mode)
print("Binning           : %d" % binning)
print("Offset            : %d" % offset)
print("AcquisitionTime   : %d" % tacq)
print("SyncDivider       : %d" % syncDivider)
print("SyncCFDZeroCross  : %d" % CFDZeroCross0)
print("SyncCFDLevel      : %d" % CFDLevel0)
print("InputCFDZeroCross : %d" % CFDZeroCross1)
print("InputCFDLevel     : %d" % CFDLevel1)

print("\nSearching for PicoHarp devices...")
print("Devidx     Status")

for i in range(0, MAXDEVNUM):
    retcode = phlib.PH_OpenDevice(ctypes.c_int(i), hwSerial)
    if retcode == 0:
        print("  %1d        S/N %s" % (i, hwSerial.value.decode("utf-8")))
        dev.append(i)
    else:
        if retcode == -1: # ERROR_DEVICE_OPEN_FAIL
            print("  %1d        no device" % i)
        else:
            phlib.PH_GetErrorString(errorString, ctypes.c_int(retcode))
            print("  %1d        %s" % (i, errorString.value.decode("utf8")))

# in this demo we will use the first PicoHarp device we found, i.e. dev[0]
# you could also check for a specific serial number, so that you always know 
# which physical device you are talking to.

if len(dev) < 1:
    print("No device available.")
    closeDevices()
print("Using device #%1d" % dev[0])
print("\nInitializing the device...")

tryfunc(phlib.PH_Initialize(ctypes.c_int(dev[0]), ctypes.c_int(mode)), "Initialize")

# Only for information
tryfunc(phlib.PH_GetHardwareInfo(dev[0], hwModel, hwPartno, hwVersion),\
        "GetHardwareInfo")
print("Found Model %s Part no %s Version %s" % (hwModel.value.decode("utf-8"),\
      hwPartno.value.decode("utf-8"), hwVersion.value.decode("utf-8")))

print("\nCalibrating...")
tryfunc(phlib.PH_Calibrate(ctypes.c_int(dev[0])), "Calibrate")

tryfunc(phlib.PH_SetSyncDiv(ctypes.c_int(dev[0]), ctypes.c_int(syncDivider)),\
        "SetSyncDiv")

tryfunc(
    phlib.PH_SetInputCFD(ctypes.c_int(dev[0]), ctypes.c_int(0),\
                         ctypes.c_int(CFDLevel0), ctypes.c_int(CFDZeroCross0)),\
    "SetInputCFD"
)

tryfunc(
    phlib.PH_SetInputCFD(ctypes.c_int(dev[0]), ctypes.c_int(1),\
                         ctypes.c_int(CFDLevel1), ctypes.c_int(CFDZeroCross1)),\
    "SetInputCFD"
)

tryfunc(phlib.PH_SetBinning(ctypes.c_int(dev[0]), ctypes.c_int(binning)), "SetBinning")
tryfunc(phlib.PH_SetOffset(ctypes.c_int(dev[0]), ctypes.c_int(offset)), "SetOffset")
tryfunc(phlib.PH_GetResolution(ctypes.c_int(dev[0]), byref(resolution)),\
        "GetResolution")

# Note: after Init or SetSyncDiv you must allow >100 ms for valid  count rate readings
time.sleep(0.2)

tryfunc(phlib.PH_GetCountRate(ctypes.c_int(dev[0]), ctypes.c_int(0), byref(countRate0)),\
        "GetCountRate")

tryfunc(phlib.PH_GetCountRate(ctypes.c_int(dev[0]), ctypes.c_int(1), byref(countRate1)),\
        "GetCountRate")

print("Resolution=%1lf Countrate0=%1d/s Countrate1=%1d/s" % (resolution.value,\
            countRate0.value, countRate1.value))

progress = 0
sys.stdout.write("\nProgress:%9u" % progress)
sys.stdout.flush()

tryfunc(phlib.PH_StartMeas(ctypes.c_int(dev[0]), ctypes.c_int(tacq)), "StartMeas")

while True:
    tryfunc(phlib.PH_GetFlags(ctypes.c_int(dev[0]), byref(flags)), "GetFlags")
    
    if flags.value & FLAG_FIFOFULL > 0:
        print("\nFiFo Overrun!")
        stoptttr()
    
    tryfunc(
        phlib.PH_ReadFiFo(ctypes.c_int(dev[0]), byref(buffer), TTREADMAX,
                          byref(nactual)),\
        "ReadFiFo", measRunning=True
    )

    if nactual.value > 0:
        # We could just iterate through our buffer with a for loop, however,
        # this is slow and might cause a FIFO overrun. So instead, we shrinken
        # the buffer to its appropriate length with array slicing, which gives
        # us a python list. This list then needs to be converted back into
        # a ctype array which can be written at once to the output file
        outputfile.write((ctypes.c_uint*nactual.value)(*buffer[0:nactual.value]))
        progress += nactual.value
        sys.stdout.write("\rProgress:%9u" % progress)
        sys.stdout.flush()
    else:
        tryfunc(phlib.PH_CTCStatus(ctypes.c_int(dev[0]), byref(ctcDone)),\
                "CTCStatus")
        if ctcDone.value > 0: 
            print("\nDone")
            stoptttr()
    # within this loop you can also read the count rates if needed.

closeDevices()
outputfile.close()

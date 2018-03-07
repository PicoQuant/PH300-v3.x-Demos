# Demo for access to PicoHarp 300 Hardware via PHLIB.DLL v 3.0.
# The program performs a measurement based on hard coded settings.
# The resulting histogram (65536 channels) is stored in an ASCII output file.
#
# Keno Goertz, PicoQuant GmbH, February 2018

import time
import ctypes as ct
from ctypes import byref

# From phdefin.h
LIB_VERSION = "3.0"
HISTCHAN = 65536
MAXDEVNUM = 8
MODE_HIST = 0
FLAG_OVERFLOW = 0x0040

# Measurement parameters, these are hardcoded since this is just a demo
binning = 0 # you can change this
offset = 0
tacq = 1000 # Measurement time in millisec, you can change this
syncDivider = 1 # you can change this 
CFDZeroCross0 = 10 # you can change this (in mV)
CFDLevel0 = 100 # you can change this (in mV)
CFDZeroCross1 = 10 # you can change this (in mV)
CFDLevel1 = 50 # you can change this (in mV)
cmd = 0

# Variables to store information read from DLLs
counts = (ct.c_uint * HISTCHAN)()
dev = []
libVersion = ct.create_string_buffer(b"", 8)
hwSerial = ct.create_string_buffer(b"", 8)
hwPartno = ct.create_string_buffer(b"", 8)
hwVersion = ct.create_string_buffer(b"", 8)
hwModel = ct.create_string_buffer(b"", 16)
errorString = ct.create_string_buffer(b"", 40)
resolution = ct.c_double()
countRate0 = ct.c_int()
countRate1 = ct.c_int()
flags = ct.c_int()

phlib = ct.CDLL("phlib.dll")

def closeDevices():
    for i in range(0, MAXDEVNUM):
        phlib.PH_CloseDevice(ct.c_int(i))
    exit(0)

def tryfunc(retcode, funcName):
    if retcode < 0:
        phlib.PH_GetErrorString(errorString, ct.c_int(retcode))
        print("PH_%s error %d (%s). Aborted." % (funcName, retcode,\
              errorString.value.decode("utf-8")))
        closeDevices()

phlib.PH_GetLibraryVersion(libVersion)
print("Library version is %s" % libVersion.value.decode("utf-8"))
if libVersion.value.decode("utf-8") != LIB_VERSION:
    print("Warning: The application was built for version %s" % LIB_VERSION)

outputfile = open("dlldemo.out", "w+")

outputfile.write("Binning           : %d\n" % binning)
outputfile.write("Offset            : %d\n" % offset)
outputfile.write("AcquisitionTime   : %d\n" % tacq)
outputfile.write("SyncDivider       : %d\n" % syncDivider)
outputfile.write("CFDZeroCross0     : %d\n" % CFDZeroCross0)
outputfile.write("CFDLevel0         : %d\n" % CFDLevel0)
outputfile.write("CFDZeroCross1     : %d\n" % CFDZeroCross1)
outputfile.write("CFDLevel1         : %d\n" % CFDLevel1)

print("\nSearching for PicoHarp devices...")
print("Devidx     Status")

for i in range(0, MAXDEVNUM):
    retcode = phlib.PH_OpenDevice(ct.c_int(i), hwSerial)
    if retcode == 0:
        print("  %1d        S/N %s" % (i, hwSerial.value.decode("utf-8")))
        dev.append(i)
    else:
        if retcode == -1: # ERROR_DEVICE_OPEN_FAIL
            print("  %1d        no device" % i)
        else:
            phlib.PH_GetErrorString(errorString, ct.c_int(retcode))
            print("  %1d        %s" % (i, errorString.value.decode("utf8")))

# In this demo we will use the first PicoHarp device we find, i.e. dev[0].
# You can also use multiple devices in parallel.
# You can also check for specific serial numbers, so that you always know 
# which physical device you are talking to.

if len(dev) < 1:
    print("No device available.")
    closeDevices()
print("Using device #%1d" % dev[0])
print("\nInitializing the device...")

tryfunc(phlib.PH_Initialize(ct.c_int(dev[0]), ct.c_int(MODE_HIST)), "Initialize")

# Only for information
tryfunc(phlib.PH_GetHardwareInfo(dev[0], hwModel, hwPartno, hwVersion),\
        "GetHardwareInfo")
print("Found Model %s Part no %s Version %s" % (hwModel.value.decode("utf-8"),\
    hwPartno.value.decode("utf-8"), hwVersion.value.decode("utf-8")))

print("\nCalibrating...")
tryfunc(phlib.PH_Calibrate(ct.c_int(dev[0])), "Calibrate")

tryfunc(phlib.PH_SetSyncDiv(ct.c_int(dev[0]), ct.c_int(syncDivider)), "SetSyncDiv")

tryfunc(
    phlib.PH_SetInputCFD(ct.c_int(dev[0]), ct.c_int(0), ct.c_int(CFDLevel0),\
                         ct.c_int(CFDZeroCross0)),\
    "SetInputCFD"
)

tryfunc(
    phlib.PH_SetInputCFD(ct.c_int(dev[0]), ct.c_int(1), ct.c_int(CFDLevel1),\
                         ct.c_int(CFDZeroCross1)),\
    "SetInputCFD"
)

tryfunc(phlib.PH_SetBinning(ct.c_int(dev[0]), ct.c_int(binning)), "SetBinning")
tryfunc(phlib.PH_SetOffset(ct.c_int(dev[0]), ct.c_int(offset)), "SetOffset")
tryfunc(phlib.PH_GetResolution(ct.c_int(dev[0]), byref(resolution)), "GetResolution")

# Note: after Init or SetSyncDiv you must allow 100 ms for valid count rate readings
time.sleep(0.2)

tryfunc(phlib.PH_GetCountRate(ct.c_int(dev[0]), ct.c_int(0), byref(countRate0)),\
        "GetCountRate")
tryfunc(phlib.PH_GetCountRate(ct.c_int(dev[0]), ct.c_int(1), byref(countRate1)),\
        "GetCountRate")

print("Resolution=%lf Countrate0=%d/s Countrate1=%d/s" % (resolution.value,\
      countRate0.value, countRate1.value))

tryfunc(phlib.PH_SetStopOverflow(ct.c_int(dev[0]), ct.c_int(1), ct.c_int(65535)),\
        "SetStopOverflow")

while cmd != "q":
    # Always use block 0 if not routing
    tryfunc(phlib.PH_ClearHistMem(ct.c_int(dev[0]), ct.c_int(0)), "ClearHistMeM")

    print("press RETURN to start measurement")
    input()

    tryfunc(phlib.PH_GetCountRate(ct.c_int(dev[0]), ct.c_int(0), byref(countRate0)),\
            "GetCountRate")
    tryfunc(phlib.PH_GetCountRate(ct.c_int(dev[0]), ct.c_int(1), byref(countRate1)),\
            "GetCountRate")
    
    print("Countrate0=%d/s Countrate1=%d/s" % (countRate0.value, countRate1.value))
    
    tryfunc(phlib.PH_StartMeas(ct.c_int(dev[0]), ct.c_int(tacq)), "StartMeas")
        
    print("\nMeasuring for %d milliseconds..." % tacq)
    
    waitloop = 0
    ctcstatus = ct.c_int(0)
    while ctcstatus.value == 0:
        tryfunc(phlib.PH_CTCStatus(ct.c_int(dev[0]), byref(ctcstatus)), "CTCStatus")
        waitloop+=1
        
    tryfunc(phlib.PH_StopMeas(ct.c_int(dev[0])), "StopMeas")
    tryfunc(phlib.PH_GetHistogram(ct.c_int(dev[0]), byref(counts), ct.c_int(0)),\
            "GetHistogram")
    tryfunc(phlib.PH_GetFlags(ct.c_int(dev[0]), byref(flags)), "GetFlags")
    
    integralCount = 0
    for i in range(0, HISTCHAN):
        integralCount += counts[i]
    
    print("\nWaitloop=%1d  TotalCount=%1.0lf" % (waitloop, integralCount))
    
    if flags.value & FLAG_OVERFLOW > 0:
        print("  Overflow.")

    print("Enter c to continue or q to quit and save the count data.")
    cmd = input()

for i in range(0, HISTCHAN):
    outputfile.write("\n%5d " % counts[i])

closeDevices()
outputfile.close()

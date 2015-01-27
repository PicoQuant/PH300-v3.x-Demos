{
  PicoHarp 300  PHLIB.DLL v3.0  Usage Demo with Delphi or Lazarus/Freepascal.
  Tested with Delphi XE5 and Lazarus 1.1/Freepascal 2.7.1 on Windows XP, 7 and 8.

  The program performs a TTTR measurement based on hardcoded settings.
  The resulting event data is stored in a binary output file.

  Michael Wahl, PicoQuant GmbH, December 2013

  Note: This is a console application (i.e. run in Windows cmd box)
}

program tttrmode;
{$apptype console}

uses
  {$ifdef fpc}
  SysUtils;
  {$else}
  System.SysUtils,
  System.Ansistrings;
  {$endif}

const
  {$IFDEF WIN64}
    PHLIB_NAME = 'PHLib64.DLL';
  {$ELSE}
    PHLIB_NAME = 'PHLib.DLL';
  {$ENDIF}

  {constants taken from PHDEFIN.H and ERRCODES.H}
  LIBVERSION='3.0';

  MAXDEVNUM=8;
  HISTCHAN=65536;         // number of histogram channels
  TTREADMAX=131072;       // 128K event records
  MAXBINSTEPS=8;

  FLAG_OVERFLOW=$0040;
  FLAG_FIFOFULL=$0003;
  FLAG_SYSERROR=$0100;    // hardware problem

  ZCMIN=0;                //mV
  ZCMAX=20;               //mV
  DISCRMIN=0;             //mV
  DISCRMAX=800;           //mV

  OFFSETMIN=0;            //ps
  OFFSETMAX=1000000000;   //ps
  ACQTMIN=1;              //ms
  ACQTMAX=360000000;      //ms  (100*60*60*1000ms = 100h)

  ERROR_DEVICE_OPEN_FAIL=-1;

type
  Pshort = ^word;
  Plong = ^longword;

label
  stoptttr,ex,cncl;

var
  outf:file;
  i:integer;
  retcode:integer;
  found:integer=0;
  dev:array[0..MAXDEVNUM-1] of integer;
  LIB_Version:array[0..7] of Ansichar;
  HW_Serial:array[0..7] of Ansichar;
  HW_Model:array[0..15] of Ansichar;
  HW_Partnum:array[0..7] of Ansichar;
  HW_Version:array[0..7] of Ansichar;
  Errorstring:array[0..40] of Ansichar;

  Binning:integer=0;        //you can change this (meaningless in T2 mode)
  Offset:integer=0;         //normally no need to change this
  Tacq:integer=10000;       //you can change this, unit is millisec
  SyncDivider:integer=1;    //you can change this
  SyncOffset:integer=0;     //you can change this
  CFDZeroCross0:integer=10; //you can change this
  CFDLevel0:integer=50;     //you can change this
  CFDZeroCross1:integer=10; //you can change this
  CFDLevel1:integer=50;     //you can change this
  blocksz:integer=32768;    //up to TTREADMAX in steps of 512

  Resolution:double;
  Countrate0,
  Countrate1,
  flags,
  nactual,
  nwritten,
  FiFoWasFull,
  CTCDone,
  Progress:integer;

  buffer: array[0..TTREADMAX-1] of cardinal;

  {the following are the functions exported by PHLIB.DLL}

  function PH_GetLibraryVersion(LIB_Version:pAnsichar):integer;
    stdcall; external PHLIB_NAME;
  function PH_GetErrorString(errstring:pAnsichar; errcode:integer):integer;
    stdcall; external PHLIB_NAME;

  function PH_OpenDevice(devidx:integer; serial:pAnsichar):integer;
    stdcall; external PHLIB_NAME;
  function PH_CloseDevice(devidx:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_Initialize(devidx:integer; mode:integer):integer;
    stdcall; external PHLIB_NAME;

  function PH_GetHardwareInfo(devidx:integer; model:pAnsichar; partno:pAnsichar; vers:pAnsichar):integer;
    stdcall; external PHLIB_NAME;
  function PH_GetSerialNumber(devidx:integer; serial:pAnsichar):integer;
    stdcall; external PHLIB_NAME;
  function PH_GetBaseResolution(devidx:integer; var resolution:double; var binsteps:integer):integer;
    stdcall; external PHLIB_NAME;

  function PH_Calibrate(devidx:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_SetSyncDiv(devidx:integer; divd:integer):integer;
    stdcall; external PHLIB_NAME;
    function PH_SetSyncOffset(devidx:integer; offset:integer):integer;
      stdcall; external PHLIB_NAME;
  function PH_SetInputCFD(devidx:integer; channel, level, zerocross:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_SetStopOverflow(devidx:integer; stop_ovfl, stopcount:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_SetBinning(devidx:integer; binning:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_SetOffset(devidx:integer; offset:integer):integer;
    stdcall; external PHLIB_NAME;

  function PH_ClearHistMem(devidx:integer; block:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_StartMeas(devidx:integer; tacq:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_StopMeas(devidx:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_CTCStatus(devidx:integer; var ctcstatus:integer):integer;
    stdcall; external PHLIB_NAME;

  function PH_GetHistogram(devidx:integer; chcount:Plong; block:longint):integer;
    stdcall; external PHLIB_NAME;
  function PH_GetResolution(devidx:integer; var resolution:double):integer;
    stdcall; external PHLIB_NAME;
  function PH_GetCountRate(devidx:integer; channel:integer; var rate:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_GetFlags(devidx:integer; var flags:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_GetElapsedMeasTime(devidx:integer; var elapsed:double):integer;
    stdcall; external PHLIB_NAME;

  //for routing:
  function PH_GetRouterVersion(devidx:integer; model:pAnsichar; version:pAnsichar):integer;
    stdcall; external PHLIB_NAME;
  function PH_GetRoutingChannels(devidx:integer; var rtchannels:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_EnableRouting(devidx:integer; enable:integer):integer;
    stdcall; external PHLIB_NAME;
    function PH_SetRoutingChannelOffset(devidx:integer; channel:integer; offset:integer):integer;
      stdcall; external PHLIB_NAME;
  function PH_SetPHR800Input(devidx:integer; channel:integer; level:integer; edge:integer):integer;
    stdcall; external PHLIB_NAME;
  function PH_SetPHR800CFD(devidx:integer; channel:integer; dscrlevel:integer; zerocross:integer):integer;
    stdcall; external PHLIB_NAME;

  //for TT modes
  function PH_ReadFiFo(devidx:integer; buffer:Plong; count:integer; var nactual:integer):integer;
    stdcall; external PHLIB_NAME;



begin
  writeln;
  writeln('PicoHarp 300 PHLib.DLL  TTTR Mode Demo              PicoQuant GmbH, 2013');
  writeln('~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~');
  PH_GetLibraryVersion(LIB_Version);
  writeln('PHLIB version is '+LIB_Version);
  if trim(LIB_Version)<>trim(LIBVERSION)
  then
    writeln('Warning: The application was built for version '+LIBVERSION);

  assign(outf,'tttrmode.out');
  {$I-}
  rewrite(outf,4);
  {$I+}
  if IOResult <> 0 then
  begin
    writeln('cannot open output file');
    goto cncl;
  end;

  writeln;
  writeln('Binning          : ',Binning);
  writeln('Offset           : ',Offset);
  writeln('AcquisitionTime  : ',Tacq);
  writeln('SyncDivider      : ',SyncDivider);
  writeln('SyncOffset       : ',SyncOffset);
  writeln('CFDZeroCross0    : ',CFDZeroCross0);
  writeln('CFDLevel0        : ',CFDLevel0);
  writeln('CFDZeroCross1    : ',CFDZeroCross1);
  writeln('CFDLevel1        : ',CFDLevel1);

  writeln;
  writeln('Searching for PicoHarp devices...');
  writeln('Devidx     Status');

  for i:=0 to MAXDEVNUM-1 do
  begin
    retcode := PH_OpenDevice(i, HW_Serial);
    if retcode=0 then //Grab any PicoHarp we can open
      begin
        writeln('  ',i,'       S/N ',HW_Serial);
        dev[found] := i; //keep index to devices we want to use
        inc(found);
      end
    else
      begin
        if retcode=ERROR_DEVICE_OPEN_FAIL then
          writeln('  ',i,'        no device')
        else
          begin
            PH_GetErrorString(Errorstring, retcode);
            writeln('  ',i,'        ', Errorstring);
          end
      end
  end;

  //in this demo we will use the first PicoHarp device we found, i.e. dev[0]
  //you could also check for a specific serial number, so that you always know
  //which physical device you are talking to.

  if found<1 then
  begin
    writeln('No device available.');
    goto ex;
  end;

  writeln('Using device ',dev[0]);
  writeln('Initializing the device...');

  retcode:=PH_Initialize(dev[0],2); //2=T2Mode, 3=T3Mode
  if retcode<0
  then
    begin
      writeln('PH init error ',retcode,'. Aborted.');
      goto ex;
    end;

  retcode:=PH_GetHardwareInfo(dev[0],HW_Model,HW_Partnum,HW_Version); (*this is only for information*)
  if retcode<0
  then
    begin
      writeln('PH_GetHardwareInfo error ',retcode,'. Aborted.');
      goto ex;
    end
  else
    writeln('Found Model ',HW_Model,' Part number ',HW_Partnum,' Version ',HW_Version);

  writeln('Calibrating...');
  retcode:=PH_Calibrate(dev[0]);
  if retcode<0
  then
    begin
      writeln('Calibration Error ',retcode,'. Aborted.');
      goto ex;
    end;

  retcode:=PH_SetSyncDiv(dev[0],SyncDivider);
  if retcode<0
  then
    begin
      writeln('PH_SetSyncDiv error ',retcode,'. Aborted.');
      goto ex;
    end;

  retcode:=PH_SetSyncOffset(dev[0],SyncOffset);
   if retcode<0
   then
     begin
       writeln('PH_SetSyncOffset error ',retcode,'. Aborted.');
       goto ex;
     end;

   retcode:=PH_SetInputCFD(dev[0],0,CFDLevel0,CFDZeroCross0);
   if retcode<0
   then
     begin
       writeln('PH_SetInputCFD error ',retcode,'. Aborted.');
       goto ex;
     end;

   retcode:=PH_SetInputCFD(dev[0],1,CFDLevel1,CFDZeroCross1);
   if retcode<0
   then
     begin
       writeln('PH_SetInputCFD error ',retcode,'. Aborted.');
       goto ex;
     end;

   retcode:=PH_SetBinning(dev[0],Binning);
   if retcode<0
   then
     begin
       writeln('PH_SetBinning error ',retcode,'. Aborted.');
       goto ex;
     end;

   retcode:=PH_SetOffset(dev[0],Offset);
   if retcode<0
   then
     begin
       writeln('PH_SetOffset error ',retcode,'. Aborted.');
       goto ex;
     end;

   retcode := PH_GetResolution(dev[0],Resolution);
   if retcode<0
   then
     begin
       writeln('PH_GetResolution error ',retcode,'. Aborted.');
       goto ex;
     end;
   writeln ('Resolution is ', Resolution:1:0, 'ps');

  //Note: after Init or SetSyncDiv you must allow 100 ms for valid new count rate readings
  sleep(200);

  retcode:=PH_GetCountRate(dev[0],0,Countrate0);
  if retcode<0
  then
    begin
      writeln('PH_GetCountRate error ',retcode,'. Aborted.');
      goto ex;
    end;

  retcode:=PH_GetCountRate(dev[0],1,Countrate1);
  if retcode<0
  then
    begin
      writeln('PH_GetCountRate error ',retcode,'. Aborted.');
      goto ex;
    end;

  writeln('Countrate0=',Countrate0,' Countrate1=',Countrate1);

  Progress:=0;
  write('Progress:',Progress:9);

  retcode:=PH_StartMeas(dev[0],Tacq);
  if retcode<0
  then
    begin
      writeln;
      writeln('Error in StartMeas. Aborted.');
      goto ex;
    end;

  while true do
    begin
      retcode:=PH_GetFlags(dev[0],flags);
        if retcode<0 then
        begin
          writeln('Error ',retcode,' in PH_GetFlags. Aborted.');
          goto ex;
        end;

      FiFoWasFull:=flags and FLAG_FIFOFULL;

      if FiFoWasFull<>0
      then
        begin
          writeln;
          writeln('FiFo Overrun!');
          goto stoptttr;
        end;

      retcode:=PH_ReadFiFo(dev[0],@buffer[0],blocksz,nactual);       //may return less!
      if retcode<0
      then
        begin
          writeln;
          writeln('PH_ReadFiFo error ',retcode);
          goto ex;
        end;

     if nactual>0
     then
       begin
         blockwrite(outf,buffer[0],nactual,nwritten);
         if nactual<>nwritten
         then
           begin
             writeln;
             writeln('file write error');
             goto stoptttr;
           end;
         Progress:=Progress+nactual;
         write(#8#8#8#8#8#8#8#8#8,Progress:9);
       end
     else
       begin
        retcode:=PH_CTCStatus(dev[0],CTCDone);
        if retcode<0
        then
          begin
            writeln('Error ',retcode,' in PH_CTCStatus. Aborted.');
            goto ex;
          end;
        if CTCDone<>0
        then
           begin
             writeln;
             writeln('Done');
             goto stoptttr;
           end;
       end;

      //PH_GetCountRate can be called here if needed
      //PH_GetCountRate can be called here if needed
    end;

stoptttr:

retcode := PH_StopMeas(dev[0]);
if retcode<0
  then
    begin
      writeln('PH_StopMeas error ',retcode,'. Aborted.');
      goto ex;
    end;

ex:

  for i:=0 to MAXDEVNUM-1 do //no harm closing all
    PH_CloseDevice(i);

  closefile(outf);
cncl:
  writeln;
  writeln('press RETURN to exit');
  readln;
end.

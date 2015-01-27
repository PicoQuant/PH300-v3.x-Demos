/************************************************************************

  PicoHarp 300    PHLIB  Interactive Mode Routing Demo in C

  Demo access to PicoHarp 300 Hardware via PHLIB v 3.0.
  The program performs a routed measurement based on hardcoded settings.
  (see 'you can change this' below). The resulting histograms (4 x 
  65536 channels) are stored as a 4-column table in an ASCII output file.
  This requires a PHR 40x or PHR 800 router for PicoHarp 300. When using
  a PHR 800 you must also set its inputs suitably (PH_SetPHR800Input).

  Michael Wahl, PicoQuant GmbH, December 2013

  Note: This is a console application
  
  Tested with gcc 4.6.2 and 4.7.2

************************************************************************/

#ifndef _WIN32
#include <unistd.h>
#define Sleep(msec) usleep(msec*1000)
#define __int64 long long
#else
#include <windows.h>
#include <dos.h>
#include <conio.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "phdefin.h"
#include "phlib.h"
#include "errorcodes.h"

//keep large histogram buffer outside main to prevent stack overflow
unsigned int counts[4][HISTCHAN]; //histograms of 4 channels


int main(int argc, char* argv[])
{
 int dev[MAXDEVNUM]; 
 int found=0;
 FILE *fpout;   
 int retcode;
 char LIB_Version[8];
 char HW_Model[16];
 char HW_PartNo[8];
 char HW_Version[8];
 char HW_Serial[8];
 char Errorstring[40];
 char Routermodel[8];
 char Routerversion[8];
 int Binning=0; //you can change this
 int Offset=0; 
 int Tacq=500; //Measurement time in millisec, you can change this
 int SyncDivider = 8; //you can change this, read manual!
 int CFDZeroCross0=10; //you can change this
 int CFDLevel0=100; //you can change this
 int CFDZeroCross1=10; //you can change this
 int CFDLevel1=100; //you can change this
 int PHR800Level = -200; //you can change this but watch for deadlock
 int PHR800Edge = 0;     //you can change this but watch for deadlock
 int PHR800CFDLevel = 100; //you can change this
 int PHR800CFDZeroCross = 10; //you can change this
 int rtchannels;
 double Resolution; 
 int Countrate0;
 int Countrate1;
 __int64 Integralcount;
 int i,j;
 int flags;
 int ctcstatus;
 int waitloop;
 char cmd=0;



 printf("\nPicoHarp 300 PHLib  Routing Demo           M. Wahl, PicoQuant GmbH, 2013");
 printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");

 PH_GetLibraryVersion(LIB_Version);
 printf("\nPHLIB version is %s",LIB_Version);
 if(strncmp(LIB_Version,LIB_VERSION,sizeof(LIB_VERSION))!=0)
         printf("\nWarning: The application was built for version %s.",LIB_VERSION);

 if((fpout=fopen("routing.out","w"))==NULL)
  {printf("\ncannot open output file\n"); goto ex;}

 fprintf(fpout,"Binning          : %ld\n",Binning);
 fprintf(fpout,"Offset           : %ld\n",Offset);
 fprintf(fpout,"AcquisitionTime  : %ld\n",Tacq);
 fprintf(fpout,"SyncDivider      : %ld\n",SyncDivider);
 fprintf(fpout,"CFDZeroCross0    : %ld\n",CFDZeroCross0);
 fprintf(fpout,"CFDLevel0        : %ld\n",CFDLevel0);
 fprintf(fpout,"CFDZeroCross1    : %ld\n",CFDZeroCross1);
 fprintf(fpout,"CFDLevel1        : %ld\n",CFDLevel1);

 printf("\nSearching for PicoHarp devices...");
 printf("\nDevidx     Status");

 for(i=0;i<MAXDEVNUM;i++)
 {
	retcode = PH_OpenDevice(i, HW_Serial); 
	if(retcode==0) //Grab any PicoHarp we can open
	{
		printf("\n  %1d        S/N %s", i, HW_Serial);
		dev[found]=i; //keep index to devices we want to use
		found++;
	}
	else
	{
		if(retcode==ERROR_DEVICE_OPEN_FAIL)
			printf("\n  %1d        no device", i);
		else 
		{
			PH_GetErrorString(Errorstring, retcode);
			printf("\n  %1d        %s", i,Errorstring);
		}
	}
 }

 //in this demo we will use the first PicoHarp device we found, i.e. dev[0]
 //you could also check for a specific serial number, so that you always know 
 //which physical device you are talking to.

 if(found<1)
 {
	printf("\nNo device available.");
	goto ex; 
 }
 printf("\nUsing device #%1d",dev[0]);
 printf("\nInitializing the device...");

 retcode = PH_Initialize(dev[0],MODE_HIST); //Standard Histogramming 
 if(retcode<0)
 {
    printf("\nPH init error %d. Aborted.\n",retcode);
    goto ex;
 }
 
 retcode = PH_GetHardwareInfo(dev[0],HW_Model,HW_PartNo,HW_Version); /*this is only for information*/
 if(retcode<0)
 {
    printf("\nPH_GetHardwareInfo error %d. Aborted.\n",retcode);
    goto ex;
 }
 else
    printf("\nFound Model %s Partnum %s Version %s",HW_Model,HW_PartNo,HW_Version);

 printf("\nCalibrating...");
 retcode=PH_Calibrate(dev[0]);
 if(retcode<0)
 {
    printf("\nCalibration Error %d. Aborted.\n",retcode);
    goto ex;
 }

 retcode = PH_SetSyncDiv(dev[0],SyncDivider);
 if(retcode<0)
 {
    printf("\nPH_SetSyncDiv error %ld. Aborted.\n",retcode);
    goto ex;
 }

 retcode = PH_SetInputCFD(dev[0],0,CFDLevel0,CFDZeroCross0); 
 if(retcode<0)
 {
        printf("\nPH_SetInputCFD error %ld. Aborted.\n",retcode);
        goto ex;
 }

 retcode = PH_SetInputCFD(dev[0],1,CFDLevel1,CFDZeroCross1); 
 if(retcode<0)
 {
        printf("\nPH_SetInputCFD error %ld. Aborted.\n",retcode);
        goto ex;
 }

 retcode = PH_SetBinning(dev[0],Binning);
 if(retcode<0)
 {
        printf("\nPH_SetBinning error %d. Aborted.\n",retcode);
        goto ex;
 }

 retcode = PH_SetOffset(dev[0],Offset);
 if(retcode<0)
 {
    printf("\nPH_SetOffset error %d. Aborted.\n",retcode);
    goto ex;
 }

 retcode = PH_EnableRouting(dev[0],1); //NEED THIS FOR ROUTING
 if(retcode<0)
 {
    printf("\nNo router connected. Aborted.\n");
	goto ex;
 }

 retcode = PH_GetRoutingChannels(dev[0],&rtchannels);
 if(retcode<0)
 {
    printf("\nPH_GetRoutingChannels failed. Aborted.\n");
    goto ex;
 }

 if(rtchannels!=4)
 {
    printf("\nInappropriate number of routing channels. Aborted.\n");
	goto ex;
 }

 retcode = PH_GetRouterVersion(dev[0], Routermodel, Routerversion);
 if(retcode<0)
 {
    printf("\nPH_GetRouterVersion failed. Aborted.\n");
    goto ex;
 }
 else
	printf("\nFound Router Model %s Version %s",Routermodel,Routerversion);

 if(strcmp(Routermodel,"PHR 800")==0)
 {
	for(i=0; i<4; i++) 
	{		
		retcode = PH_SetPHR800Input(dev[0], i, PHR800Level, PHR800Edge);
		if(retcode<0) //All channels may not be installed, so be liberal here 
		{
			printf("\nPH_SetPHR800Input (ch%1d) failed. Maybe not installed.\n",i);
		}
	}

	for(i=0; i<4; i++) 
	{	
		retcode = PH_SetPHR800CFD(dev[0], i, PHR800CFDLevel, PHR800CFDZeroCross);
		if(retcode<0) //CFDs may not be installed, so be liberal here 
		{
			printf("\nPH_SetPHR800CFD (ch%1d) failed. Maybe not installed.\n",i);
		}
	}
 }

 retcode = PH_GetResolution(dev[0],&Resolution);
 if(retcode<0)
 {
        printf("\nPH_GetResolution error %d. Aborted.\n",retcode);
        goto ex;
 }

 //Note: after Init or SetSyncDiv you must allow 100 ms for valid new count rate readings
 Sleep(200);

 retcode = PH_GetCountRate(dev[0],0,&Countrate0);
 if(retcode<0)
 {
        printf("\nPH_GetCountRate error %d. Aborted.\n",retcode);
        goto ex;
 }

 retcode = PH_GetCountRate(dev[0],1,&Countrate1);
 if(retcode<0)
 {
        printf("\nPH_GetCountRate error %d. Aborted.\n",retcode);
        goto ex;
 }

 printf("\nResolution=%lf Countrate0=%1d/s Countrate1=%1d/s", Resolution, Countrate0, Countrate1);

 PH_SetStopOverflow(dev[0],1,65535);

 while(cmd!='q')
 { 

        for(i=0; i<4; i++) PH_ClearHistMem(dev[0],i);  // must clear 4 Blocks for 4-channel Routing
         
        retcode = PH_GetFlags(dev[0],&flags); // to clear the flags
        if(retcode<0)
        {
                printf("\nError %1d in GetFlags. Aborted.\n",retcode);
                goto ex;
        }

        printf("\npress RETURN to start measurement");
        getchar();

        retcode = PH_GetCountRate(dev[0],0,&Countrate0);
        if(retcode<0)
        {
                printf("\nPH_GetCountRate error %d. Aborted.\n",retcode);
                goto ex;
        }

        retcode = PH_GetCountRate(dev[0],1,&Countrate1);
        if(retcode<0)
        {
                printf("\nPH_GetCountRate error %d. Aborted.\n",retcode);
                goto ex;
        }

        printf("\nCountrate0=%1d/s Countrate1=%1d/s",Countrate0,Countrate1);
        
        retcode = PH_StartMeas(dev[0],Tacq); 
        if(retcode<0)
        {
                printf("\nError %1d in StartMeas. Aborted.\n",retcode);
                goto ex;
        }
         
        printf("\nMeasuring for %1d milliseconds...",Tacq);
        
        waitloop=0;
        ctcstatus=0;
        while(ctcstatus==0) 
        {
                retcode = PH_CTCStatus(dev[0],&ctcstatus);
                if(retcode<0)
                {
                        printf("\nError %1d in StartMeas. Aborted.\n",retcode);
                        goto ex;
                }
                waitloop++; 
        }
         
        retcode = PH_StopMeas(dev[0]);
        if(retcode<0)
        {
                printf("\nError %1d in StopMeas. Aborted.\n",retcode);
                goto ex;
        }
      
        for(i=0; i<4; i++) //loop through the routing channels to fetch the data
        {
                retcode = PH_GetHistogram(dev[0],counts[i],i);
                if(retcode<0)
                {
                        printf("\nError %1d in GetHistogram. Aborted.\n",retcode);
                        goto ex;
                }
                Integralcount = 0;
                for(j=0;j<HISTCHAN;j++)
                        Integralcount+=counts[i][j];
                printf("\nTotal count in channel %1d = %lu",i+1,Integralcount);
        }

        retcode = PH_GetFlags(dev[0],&flags);
        if(retcode<0)
        {
                printf("\nError %1d in GetFlags. Aborted.\n",retcode);
                goto ex;
        }

        if(flags&FLAG_OVERFLOW) printf("\nOverflow.");

        printf("\nEnter c to continue or q to quit and save the count data.");
        cmd=getchar();
 }
 
 //output histograms of the 4 channels as a 4 column table
 for(i=0;i<HISTCHAN;i++)
        fprintf(fpout,"\n%9d %9d %9d %9d",counts[0][i],counts[1][i],counts[2][i],counts[3][i]);

ex:
 if(fpout) fclose(fpout);

 for(i=0;i<MAXDEVNUM;i++) //no harm to close all
 {
        PH_CloseDevice(i);
 }

 printf("\npress RETURN to exit");
 getchar();
 return 0;
}


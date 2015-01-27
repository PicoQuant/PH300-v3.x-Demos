/************************************************************************

  PicoHarp 300    PHLIB.DLL  TTTR Mode Demo in C

  Demo access to PicoHarp 300 Hardware via PHLIB v 3.0.
  The program performs a TTTR measurement based on hardcoded settings.
  The resulting event data is stored in a binary output file.

  Michael Wahl, PicoQuant GmbH, December 2013

  Note: This is a console application
  
  Tested with gcc 4.6.2 and 4.8.1

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
#include <time.h>


#include "phdefin.h"
#include "phlib.h"
#include "errorcodes.h"

unsigned int buffer[TTREADMAX];


int main(int argc, char* argv[])
{
 int i;
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
 int Mode=MODE_T2; //set T2 or T3 here, observe suitable Syncdivider and Range!
 int Binning=0;   //you can change this (meaningless in T2 mode, important in T3 mode!)
 int Offset=0;  //normally no need to change this
 int Tacq=10000;        //you can change this, unit is millisec
 int SyncDivider = 1;  //you can change this, observe Mode! READ MANUAL!
 int CFDZeroCross0=10; //you can change this
 int CFDLevel0=50; //you can change this
 int CFDZeroCross1=10; //you can change this
 int CFDLevel1=150; //you can change this
 int blocksz = TTREADMAX; // in steps of 512
 double Resolution; 
 int Countrate0;
 int Countrate1;
 int flags;
 int nactual;
 int FiFoWasFull,CTCDone,Progress;


 printf("\nPicoHarp 300 PHLib   TTTR Mode Demo        M. Wahl, PicoQuant GmbH, 2013");
 printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
 PH_GetLibraryVersion(LIB_Version);
 printf("\nPHLIB version is %s",LIB_Version);
 if(strncmp(LIB_Version,LIB_VERSION,sizeof(LIB_VERSION))!=0)
         printf("\nWarning: The application was built for version %s.",LIB_VERSION);

 if((fpout=fopen("tttrmode.out","wb"))==NULL)
 {
         printf("\ncannot open output file\n"); 
         goto ex;
 }

 printf("\n\n");
 printf("Mode             : %ld\n",Mode);
 printf("Binning          : %ld\n",Binning);
 printf("Offset           : %ld\n",Offset);
 printf("AcquisitionTime  : %ld\n",Tacq);
 printf("SyncDivider      : %ld\n",SyncDivider);
 printf("CFDZeroCross0    : %ld\n",CFDZeroCross0);
 printf("CFDLevel0        : %ld\n",CFDLevel0);
 printf("CFDZeroCross1    : %ld\n",CFDZeroCross1);
 printf("CFDLevel1        : %ld\n",CFDLevel1);


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

 retcode = PH_Initialize(dev[0],Mode); 
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
 retcode = PH_Calibrate(dev[0]);
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

 printf("\nResolution=%1lf Countrate0=%1d/s Countrate1=%1d/s", Resolution, Countrate0, Countrate1);

 Progress = 0;
 printf("\nProgress:%9d",Progress);

 retcode = PH_StartMeas(dev[0],Tacq);
 if(retcode<0)
 {
        printf("\nError in StartMeas. Aborted.\n");
        goto ex;
 }

 while(1)  
 {
        retcode = PH_GetFlags(dev[0],&flags);
        if(retcode<0)
        {
                printf("\nError %1d in GetFlags. Aborted.\n",retcode);
                goto ex;
        }

		FiFoWasFull=flags&FLAG_FIFOFULL;
   
		if (FiFoWasFull) 
		{
			printf("\nFiFo Overrun!\n"); 
			goto stoptttr;
		}
		
		retcode = PH_ReadFiFo(dev[0],buffer,blocksz,&nactual);	//may return less!  
		if(retcode<0) 
		{ 
			printf("\nReadData error %d\n",retcode); 
			goto stoptttr; 
		}  

		if(nactual) 
		{
			if(fwrite(buffer,4,nactual,fpout)!=(unsigned)nactual)
			{
				printf("\nfile write error\n");
				goto stoptttr;
			}               
				Progress += nactual;
				printf("\b\b\b\b\b\b\b\b\b%9d",Progress);
				fflush(stdout);
		}
		else
		{
            retcode = PH_CTCStatus(dev[0],&CTCDone);
            if(retcode<0)
            {
                printf("\nError %1d in StartMeas. Aborted.\n",retcode);
                goto ex;
            }

			if (CTCDone) 
			{ 
				printf("\nDone\n"); 
				goto stoptttr; 
			}  
		}
	
		//You can query the count rates here, but do it only if you need them
/*
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
*/
 }

stoptttr:

 PH_StopMeas(dev[0]);

ex:

 for(i=0;i<MAXDEVNUM;i++) //no harm to close all
 {
	PH_CloseDevice(i);
 }

 if(fpout) fclose(fpout);
 printf("\npress RETURN to exit");
 getchar();
 return 0;
}

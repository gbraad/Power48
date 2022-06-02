/*************************************************************************\
*
*  SATURN.C
*
*  This is the core of the emulator
*
*  (c)1995 Sebastien Carlier
*  (c)2002 Robert Hildinger
*
\*************************************************************************/

#include "Power48.h"
#include "Power48_core.h"
#include <SonyCLIE.h>
#include "IO.H"
#include <PceNativeCall.h>
#include "TRACE.H"


emulState* Q;

MemPtr corePointer;


void SetCPUSpeed(void)
{
	UInt32 cycles;
	
	if (optionTrueSpeed)
	{
		if (optionTarget==HP48SX)
		{
			Q->optionTrueSpeed = true;
			cycles = 10500;
		}
		else
		{
			Q->optionTrueSpeed = true;
			cycles = 21000;
		}
	}
	else
	{
		Q->optionTrueSpeed = false;
		cycles = 0;
	}
	Q->maxCyclesPerTick = ByteSwap32(cycles);
}



static void EmulateBeep(char *checkAddr)
{
	long			frequency, duration;
	SndCommandType	sndCommand;
		
	frequency = ((long)Q->D[4]<<16) | ((long)Q->D[3]<<12) | (Q->D[2]<<8) | (Q->D[1]<<4) | Q->D[0];
	duration = ((long)Q->C[4]<<16) | ((long)Q->C[3]<<12) | (Q->C[2]<<8) | (Q->C[1]<<4) | Q->C[0];

	Q->CARRY=1;

	if (!(*checkAddr &0x8) && (frequency))
	{
		sndCommand.cmd = sndCmdFreqDurationAmp;
		sndCommand.param1=frequency;
		sndCommand.param2=(UInt16)(duration&0xFFFF);
		sndCommand.param3=PrefGetPreference (prefSysSoundVolume);
		SndDoCmd(NULL,&sndCommand,0);

		Q->P=0;
		Q->CARRY=0;
	}
} 



UInt32 emulate(void) 
{
	MemHandle armH;
	MemPtr armP;
	UInt16 pColor,bColor;
	UInt32 rVal;
	
	Q->optionTarget = optionTarget;
	
	lastInstCount=0;
	lastCycleCount=0;

	pColor=(((UInt16)statusPixelColor->r<<8)&0xf800)|(((UInt16)statusPixelColor->g<<3)&0x07e0)|(statusPixelColor->b>>3);
	bColor=(((UInt16)statusBackColor->r<<8)&0xf800)|(((UInt16)statusBackColor->g<<3)&0x07e0)|(statusBackColor->b>>3);

	Q->colorPixelVal=ByteSwap32(((UInt32)pColor<<16)|pColor);
	Q->colorBackVal=ByteSwap32(((UInt32)bColor<<16)|bColor);
	
	Q->ErrorPrint = &ErrorPrint;
	Q->EmulateBeep = &EmulateBeep;
	Q->display_clear = &display_clear;
	Q->toggle_lcd = &toggle_lcd;
	Q->ProcessPalmEvent = &ProcessPalmEvent;
	Q->kbd_handler = &kbd_handler;
	
#ifdef TRACE_ENABLED
	Q->traceInProgress=0;
	Q->TracePrint = &TracePrint;
#endif

	Q->rowInt16s = screenRowInt16s;
		
	SetCPUSpeed();	
	
	ResetTime();
		
	armH = DmGetResource('ARMC', 0x3002);
	armP = MemHandleLock(armH);
		
	corePointer=armP;
	statusCoreLoaded=true;
		
	rVal = PceNativeCall(armP, Q);
		
	MemHandleUnlock(armH);
	DmReleaseResource(armH);

	statusCoreLoaded=false;

	return rVal;
}
/*************************************************************************\
*
*  Power48_core.h
*
*  Supplementary header file for Saturn core
*
*  (c)2003 by Robert Hildinger
*
\*************************************************************************/

#include <PalmOS.h>
#include <PceNativeCall.h>
#include "TRACE.H"


/***********************************************************************
 *
 *	Internal Constants
 *
 ***********************************************************************/


#define XM 1
#define SB 2
#define SR 4
#define MP 8



#define noKey				99
#define byteswapped_noKey	0x6300
#define virtKeyVideoSwap	0xFF   
#define virtKeyVideoSwapLE	0xFF00   




#ifndef __CALCTARGET__
#define __CALCTARGET__

typedef enum 		// Make sure any changes to here
{					// are reflected in Power48.h
	HP48SX,
	HP48GX,
	HP49G
} calcTarget;

typedef enum
{
	KEY_RELEASE_ALL,
	KEY_PRESS_SINGLE
} KHType;

#endif


#pragma warn_padding on

typedef struct {
	// versioning, ID and checksum section
	UInt32			structureID;
	UInt32			structureVersion;
	
	// normal variables from this point
	UInt8 			A[16];
	UInt8			B[16];
	UInt8 			C[16];
	UInt8 			D[16];
	UInt8 			R0[16];
	UInt8 			R1[16];
	UInt8 			R2[16];
	UInt8 			R3[16];
	UInt8 			R4[16];
	UInt8 			ST[4];
	
	UInt8			ioram[64];
	
	UInt8			lcdRedrawBuffer[64];
	
	Boolean			status49GActive;
	Boolean			statusQuit;
	Boolean			statusPerformReset;
	Boolean			statusScreenRedraw;
	
	Boolean			statusDisplayMode2x2;
	Boolean			statusBackdrop;
	Boolean			statusColorCycle;
	Boolean			statusPortraitViaLandscape;
	
	UInt8			optionTarget;
	Boolean			statusChangeAnn;
	Boolean 		statusRecalc;
	UInt8 			t1;
	
	Boolean 		mcIOAddrConfigured;
	Boolean 		mcRAMSizeConfigured;
	Boolean 		mcRAMAddrConfigured;
	Boolean 		mcCE1SizeConfigured;
	
	Boolean 		mcCE1AddrConfigured;
	Boolean 		mcCE2SizeConfigured;
	Boolean 		mcCE2AddrConfigured;
	Boolean 		mcNCE3SizeConfigured;
	
	UInt8 			F_s[16];
	UInt8 			F_l[16];
	
	UInt8			kbd_row[9];
	Boolean 		optionTrueSpeed;
	UInt8 			HST;
	UInt8 			P;
	
	UInt8			CARRY;
	Int8			CARDSTATUS;
	Boolean 		mcNCE3AddrConfigured;
	Boolean 		MODE;
	
	// start of members that must be ByteSwap16'd

	Int16 			INT16_variable_start;
	UInt16 			crc;
	
	UInt16 			IO_TIMER_RUN;
	UInt16			delayEvent;
	
	UInt16 			IN;
	UInt16			OUT;
	
	UInt16			rstkp;
	UInt16 			dummy_16;
	
	UInt16  		lcdCurrentLine;
	Int16  			lcdDisplayAreaWidth;
	
	UInt16  		lcdLineCount;
	Int16  			lcdByteOffset;
	
	UInt16  		lcdContrastLevel;
	UInt16  		lcdPixelOffset;
	
	UInt16  		lcdAnnunciatorState;
	UInt16  		lcdPreviousAnnunciatorState;
	
	UInt16  		lcdOn;
	UInt16  		lcdTouched;
	
	UInt16			delayKey;
	UInt16			delayEvent2;
	
	UInt16 			flashBankLo;
	UInt16 			flashBankHi;

	UInt16			delayKey2;
	Int16 			INT16_variable_end;
	
	// start of members that must be ByteSwap32'd

	Int32 			INT32_variable_start;

	UInt32			SHUTDN;
	UInt32			WAKEUP;
	UInt32			INTERRUPT_ENABLED;
	UInt32			INTERRUPT_PENDING;
	UInt32			INTERRUPT_PROCESSING;
	UInt32			IR15X;
	
	UInt32			pc;
	UInt32			rstk[8];
	UInt32			d0;
	UInt32			d1;
	
	UInt32 			lcdDisplayAddressStart;
	UInt32 			lcdDisplayAddressStart2;
	UInt32 			lcdDisplayAddressEnd;
	UInt32 			lcdMenuAddressStart;
	UInt32 			lcdMenuAddressEnd;
	
	UInt8*			lcdLineAddress[64];
	
	UInt32 			mcIOBase;
	UInt32 			mcIOCeil;
	UInt32 			mcRAMBase;
	UInt32			mcRAMCeil;
	UInt32			mcRAMSize;
	UInt32			mcRAMDelta;
	UInt32 			mcCE1Base;
	UInt32			mcCE1Ceil;
	UInt32			mcCE1Size;
	UInt32			mcCE1Delta;
	UInt32 			mcCE2Base;
	UInt32			mcCE2Ceil;
	UInt32			mcCE2Size;
	UInt32			mcCE2Delta;
	UInt32 			mcNCE3Base;
	UInt32			mcNCE3Ceil;
	UInt32			mcNCE3Size;
	UInt32			mcNCE3Delta;
	
	UInt8*			rom;
	UInt8*			ram;
	UInt8*			port1;
	UInt8*			port2;
	UInt8*			ce1addr;
	UInt8*			ce2addr;
	UInt8*			nce3addr;
	UInt8* 			flashAddr[16];
	
	UInt8*			flashRomLo;
	UInt8*			flashRomHi;
	
	UInt32			t2;
	UInt32 			prevTickStamp;
	
	UInt32 			instCount;
	UInt32 			cycleCount;
	
	UInt32			rowInt16s;
	
	void*			ErrorPrint;
	void*			EmulateBeep;
	void*			display_clear;
	void*			toggle_lcd;
	void*			ProcessPalmEvent;
	void*			kbd_handler;
#ifdef TRACE_ENABLED
	void*			TracePrint;
#endif
	Int32 			INT32_variable_end;
	
	// non-swap section

	UInt32*			lcd2x2MemPtrBase;
	UInt32*			lcd2x2MemPtrBasePVL;
	UInt16*			lcd1x1MemPtrBase;
	UInt16*			softKeyMemPtrBase;
	UInt16*			labelMemPtrBase;
	UInt32*			lcd2x2AnnAreaPtrBase;
	UInt32*			lcd2x2AnnAreaPtrBasePVL;
	UInt32*			lcd1x1AnnAreaPtrBase;
	UInt16*			annunciatorMemPtrBase;
	UInt32*			backgroundImagePtr2x2;
	UInt16*			backgroundImagePtr1x1;


	const void*		emulStateP;
	Call68KFuncType* call68KFuncP;
	
	UInt32 			maxCyclesPerTick;
	
	UInt32			labelOffset[6];
	UInt32			colorPixelVal;
	UInt32			colorBackVal;
	UInt32			contrastPixelVal;
	UInt32			contrastBackVal;
	
	UInt8			colorCyclePhase;
	UInt8			colorCycleRed;
	UInt8			colorCycleGreen;
	UInt8			colorCycleBlue;
		
#ifdef TRACE_ENABLED
	UInt32 			A_HI;
	UInt32 			A_LO;
	UInt32 			B_HI;
	UInt32 			B_LO;
	UInt32 			C_HI;
	UInt32 			C_LO;
	UInt32 			D_HI;
	UInt32 			D_LO;
	
	UInt32			nibcode;
	
	UInt8			OLD_P;
	UInt8			OLD_CARRY;
	UInt8			OLD_T1;
	UInt8			poporpush;
		
	UInt32			OLD_T2;
	UInt32			OLD_D0;
	UInt32 			OLD_D1;
	UInt32 			OLD_PC;

	UInt16 			OLD_ST;
	UInt16 			dummyi16;
		
	UInt32			lcdMainAddressLower;
	UInt32			lcdMainAddressUpper;
	UInt32 			lcdMenuAddressLower;
	UInt32 			lcdMenuAddressUpper;

	UInt32 			traceNumber;
	UInt32 			traceVal;
	UInt32 			traceVal2;
	UInt32 			traceVal3;
	UInt32			traceInProgress;
#endif		
} emulState;

#pragma warn_padding off

extern emulState* Q;


#define annunciator2x2OriginX		46
#define annunciator1x1OriginX		154
#define annunciator2x2OriginY		2
#define annunciator1x1OriginY		3
#define annunciatorBitmapWidth		60
#define annShiftLeftOffset			0
#define ann2x2ShiftLeftPositionX	14
#define ann1x1ShiftLeftPositionY	0
#define annShiftRightOffset			10
#define ann2x2ShiftRightPositionX	54
#define ann1x1ShiftRightPositionY	11
#define annAlphaOffset				20
#define ann2x2AlphaPositionX		94
#define ann1x1AlphaPositionY		22
#define annAlarmOffset				30
#define ann2x2AlarmPositionX		134
#define ann1x1AlarmPositionY		33
#define annBusyOffset				40
#define ann2x2BusyPositionX			190
#define ann1x1BusyPositionY			44
#define annIOOffset					50
#define ann2x2IOPositionX			230
#define ann1x1IOPositionY			55
#define annHeight					10


#define		softLabelStart	57
#define 	skipToKeyB		26
#define 	skipToKeyC		26
#define 	skipToKeyD		115   // Actual value is 27*pixel width - this value
#define 	skipToKeyE		26
#define 	skipToKeyF		26


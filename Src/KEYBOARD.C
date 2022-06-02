/*************************************************************************\
*
*  KEYBOARD.C
*
*  This module is responsible for processing all keyboard input for
*  the emulator.
*
*  (c)2002 by Robert Hildinger
*
\*************************************************************************/

#include "Power48.h"
#include "Power48_core.h"
#include <PalmOS.h>
#include <SonyCLIE.h>
#include "Power48Rsc.h"
#include <GarminChars.h>
#include "TRACE.H"
#include <PalmSG.h>
#include <TapWave.h>

Boolean keyChordingOn=false;
Boolean specialKeyPressed=false;
UInt16 scanCode=noKey,keyReleasePending=noKey;
UInt16 chordingBuf[5]={noKey,noKey,noKey,noKey,noKey};
UInt16 numCodesInBuf=0;
UInt32 updateMark=0;
Int32 delayCycles=0;
UInt32 keyHoldTimeMark=0;
Boolean keyHoldTimerStarted=false;
Boolean keyShifterOn=false;
UInt16 shiftedKey=noKey;
UInt16 oldDirKeyValue = noKey;


const squareDef virtLayout2x2[] =
{
	{ 44, 123,  88, 144},		// virtKeySoftKeyA
	{ 90, 123, 132, 144},		// virtKeySoftKeyB
	{134, 123, 176, 144},		// virtKeySoftKeyC
	{178, 123, 220, 144},		// virtKeySoftKeyD
	{222, 123, 264, 144},		// virtKeySoftKeyE
	{266, 123, 308, 144},		// virtKeySoftKeyF
	{266,   0, 308, 122},		// virtKeyLabelNxt
	{ 44,   0,  88, 122}		// virtKeyLabelPrev
};


const squareDef virtLayout1x1[] =
{
	{168,  58, 188,  68},		// virtKeySoftKeyA
	{190,  58, 210,  68},		// virtKeySoftKeyB
	{212,  58, 232,  68},		// virtKeySoftKeyC
	{234,  58, 254,  68},		// virtKeySoftKeyD
	{256,  58, 276,  68},		// virtKeySoftKeyE
	{278,  58, 298,  68},		// virtKeySoftKeyF
	{278,   4, 298,  56},		// virtKeyLabelNxt
	{168,   4, 188,  56}		// virtKeyLabelPrev
};


const keyDef keyLayout450[] =      // coordinates relative to upper left of display
{
	{ 44, 147,  77, 163, 1, 0x10},		// softKeyA
	{ 90, 147, 123, 163, 8, 0x10},		// softKeyB
	{136, 147, 169, 163, 8, 0x08},		// softKeyC
	{182, 147, 215, 163, 8, 0x04},		// softKeyD
	{228, 147, 261, 163, 8, 0x02},		// softKeyE
	{274, 147, 307, 163, 8, 0x01},		// softKeyF

	{ 44, 178,  77, 199, 2, 0x10},		// keyMTH
	{ 90, 178, 123, 199, 7, 0x10},		// keyPRG
	{136, 178, 169, 199, 7, 0x08},		// keyCST
	{182, 178, 215, 199, 7, 0x04},		// keyVAR
	{228, 178, 261, 199, 7, 0x02},		// keyUp
	{274, 178, 307, 199, 7, 0x01},		// keyNXT

	{ 44, 213,  77, 234, 0, 0x10},		// keyQuote
	{ 90, 213, 123, 234, 6, 0x10},		// keySTO
	{136, 213, 169, 234, 6, 0x08},		// keyEVAL
	{182, 213, 215, 234, 6, 0x04},		// keyLEFT
	{228, 213, 261, 234, 6, 0x02},		// keyDOWN
	{274, 213, 307, 234, 6, 0x01},		// keyRIGHT

	{ 44, 248,  77, 269, 3, 0x10},		// keySIN
	{ 90, 248, 123, 269, 5, 0x10},		// keyCOS
	{136, 248, 169, 269, 5, 0x08},		// keyTAN
	{182, 248, 215, 269, 5, 0x04},		// keySquareRoot
	{228, 248, 261, 269, 5, 0x02},		// keyYtotheX
	{274, 248, 307, 269, 5, 0x01},		// key1OverX

	{ 44, 282, 122, 304, 4, 0x10},		// keyENTER
	{136, 282, 169, 304, 4, 0x08},		// keyPlusMinus
	{182, 282, 215, 304, 4, 0x04},		// keyEEX
	{228, 282, 261, 304, 4, 0x02},		// keyDEL
	{274, 282, 307, 304, 4, 0x01},		// keyBackspace

	{ 44, 318,  77, 340, 3, 0x20},		// keyAlpha
	{ 97, 318, 139, 340, 3, 0x08},		// key7
	{153, 318, 195, 340, 3, 0x04},		// key8
	{209, 318, 251, 340, 3, 0x02},		// key9
	{265, 318, 307, 340, 3, 0x01},		// keyDivide

	{ 44, 354,  77, 376, 2, 0x20},		// keyLeftShift
	{ 97, 354, 139, 376, 2, 0x08},		// key4
	{153, 354, 195, 376, 2, 0x04},		// key5
	{209, 354, 251, 376, 2, 0x02},		// key6
	{265, 354, 307, 376, 2, 0x01},		// keyMultiply

	{ 44, 390,  77, 412, 1, 0x20},		// keyRightShift
	{ 97, 390, 139, 412, 1, 0x08},		// key1
	{153, 390, 195, 412, 1, 0x04},		// key2
	{209, 390, 251, 412, 1, 0x02},		// key3
	{265, 390, 307, 412, 1, 0x01},		// keySubtract

	{ 44, 424,  77, 446, 4, 0x20},		// keyON
	{ 97, 424, 139, 446, 0, 0x08},		// key0
	{153, 424, 195, 446, 0, 0x04},		// keyPeriod
	{209, 424, 251, 446, 0, 0x02},		// keySPC
	{265, 424, 307, 446, 0, 0x01},		// keyAdd

	{900, 900, 910, 910, 0, 0x00},		// keyAPPS
	{900, 900, 910, 910, 0, 0x00},		// keyMODE
	{900, 900, 910, 910, 0, 0x00},		// keyTOOL
	{900, 900, 910, 910, 0, 0x00},		// keyHIST
	{900, 900, 910, 910, 0, 0x00},		// keyCAT
	{900, 900, 910, 910, 0, 0x00},		// keyEQW
	{900, 900, 910, 910, 0, 0x00},		// keySYMB
	{900, 900, 910, 910, 0, 0x00},		// keyX

	{  4,  61,  39,  72, 0, 0x00},		// keyAbout48GX
	{ 81, 429,  91, 440, 0, 0x00},		// keyOnPlus
	{ 23, 111,  42, 121, 0, 0x00},		// keyObject
	{  4,   4,  38,  42, 0, 0x00}		// keySwitch
};


const keyDef keyLayout450_hp49[] =      // coordinates relative to upper left of display
{
	{ 42, 155,  78, 173, 5, 0x01},		// softKeyA
	{ 89, 155, 125, 173, 5, 0x02},		// softKeyB
	{136, 155, 172, 173, 5, 0x04},		// softKeyC
	{183, 155, 219, 173, 5, 0x08},		// softKeyD
	{230, 155, 266, 173, 5, 0x10},		// softKeyE
	{277, 155, 313, 173, 5, 0x20},		// softKeyF

	{900, 900, 910, 910, 0, 0x00},		// keyMTH
	{900, 900, 910, 910, 0, 0x00},		// keyPRG
	{900, 900, 910, 910, 0, 0x00},		// keyCST
	{181, 189, 221, 209, 2, 0x80},		// keyVAR
	{ 13, 207,  33, 237, 6, 0x08},		// keyUp
	{273, 189, 313, 209, 0, 0x80},		// keyNXT

	{900, 900, 910, 910, 0, 0x00},		// keyQuote
	{227, 189, 267, 209, 1, 0x80},		// keySTO
	{900, 900, 910, 910, 0, 0x00},		// keyEVAL
	{  4, 249,  21, 280, 6, 0x04},		// keyLEFT
	{ 13, 292,  33, 322, 6, 0x02},		// keyDOWN
	{ 24, 249,  42, 280, 6, 0x01},		// keyRIGHT

	{159, 254, 202, 274, 2, 0x20},		// keySIN
	{215, 254, 258, 274, 1, 0x20},		// keyCOS
	{270, 254, 313, 274, 0, 0x20},		// keyTAN
	{104, 254, 147, 274, 3, 0x20},		// keySquareRoot
	{ 48, 254,  91, 274, 4, 0x20},		// keyYtotheX
	{215, 285, 258, 308, 1, 0x10},		// key1OverX

	{270, 422, 313, 446, 0, 0x01},		// keyENTER
	{104, 285, 147, 308, 3, 0x10},		// keyPlusMinus
	{ 48, 285,  91, 308, 4, 0x10},		// keyEEX
	{900, 900, 910, 910, 0, 0x00},		// keyDEL
	{270, 222, 313, 242, 0, 0x40},		// keyBackspace

	{ 48, 319,  91, 342, 7, 0x08},		// keyAlpha
	{104, 319, 147, 342, 3, 0x08},		// key7
	{159, 319, 202, 342, 2, 0x08},		// key8
	{215, 319, 258, 342, 1, 0x08},		// key9
	{270, 285, 313, 308, 0, 0x10},		// keyDivide

	{ 48, 354,  91, 377, 7, 0x04},		// keyLeftShift
	{104, 354, 147, 377, 3, 0x04},		// key4
	{159, 354, 202, 377, 2, 0x04},		// key5
	{215, 354, 258, 377, 1, 0x04},		// key6
	{270, 319, 313, 342, 0, 0x08},		// keyMultiply

	{ 48, 388,  91, 411, 7, 0x02},		// keyRightShift
	{104, 388, 147, 411, 3, 0x02},		// key1
	{159, 388, 202, 411, 2, 0x02},		// key2
	{215, 388, 258, 411, 1, 0x02},		// key3
	{270, 354, 313, 377, 0, 0x04},		// keySubtract

	{ 48, 422,  91, 446, 0, 0x00},		// keyON
	{104, 422, 147, 446, 3, 0x01},		// key0
	{159, 422, 202, 446, 2, 0x01},		// keyPeriod
	{215, 422, 258, 446, 1, 0x01},		// keySPC
	{270, 388, 313, 411, 0, 0x02},		// keyAdd

	{ 43, 189,  83, 209, 5, 0x80},		// keyAPPS
	{ 89, 189, 130, 209, 4, 0x80},		// keyMODE
	{135, 189, 175, 209, 3, 0x80},		// keyTOOL
	{ 48, 222,  91, 242, 4, 0x40},		// keyHIST
	{104, 222, 147, 242, 3, 0x40},		// keyCAT
	{159, 222, 202, 242, 2, 0x40},		// keyEQW
	{215, 222, 258, 242, 1, 0x40},		// keySYMB
	{159, 285, 202, 308, 2, 0x10},		// keyX

	{  4,  61,  46,  72, 0, 0x00},		// keyAbout48GX
	{ 27, 433,  45, 443, 0, 0x00},		// keyOnPlus
	{ 23, 111,  42, 121, 0, 0x00},		// keyObject
	{  4,   4,  38,  44, 0, 0x00}			// keySwitch
};


const keyDef keyLayout320[] =      // coordinates relative to upper left of display
{
	{  4,  20,  40,  40, 1, 0x10},		// softKeyA
	{ 51,  20,  87,  40, 8, 0x10},		// softKeyB
	{ 98,  20, 134,  40, 8, 0x08},		// softKeyC
	{  4,  47,  40,  67, 8, 0x04},		// softKeyD
	{ 51,  47,  87,  67, 8, 0x02},		// softKeyE
	{ 98,  47, 134,  67, 8, 0x01},		// softKeyF

	{  6,  84,  37, 107, 2, 0x10},		// keyMTH
	{ 52,  84,  83, 107, 7, 0x10},		// keyPRG
	{ 98,  84, 129, 107, 7, 0x08},		// keyCST
	{144,  84, 175, 107, 7, 0x04},		// keyVAR
	{190,  84, 221, 107, 7, 0x02},		// keyUp
	{236,  84, 267, 107, 7, 0x01},		// keyNXT

	{  6, 119,  37, 142, 0, 0x10},		// keyQuote
	{ 52, 119,  83, 142, 6, 0x10},		// keySTO
	{ 98, 119, 129, 142, 6, 0x08},		// keyEVAL
	{144, 119, 175, 142, 6, 0x04},		// keyLEFT
	{190, 119, 221, 142, 6, 0x02},		// keyDOWN
	{236, 119, 267, 142, 6, 0x01},		// keyRIGHT

	{282, 119, 313, 142, 3, 0x10},		// keySIN
	{282, 154, 313, 177, 5, 0x10},		// keyCOS
	{282, 189, 313, 212, 5, 0x08},		// keyTAN
	{282, 224, 313, 247, 5, 0x04},		// keySquareRoot
	{282, 259, 313, 282, 5, 0x02},		// keyYtotheX
	{282, 294, 313, 317, 5, 0x01},		// key1OverX

	{  6, 154,  82, 177, 4, 0x10},		// keyENTER
	{ 98, 154, 129, 177, 4, 0x08},		// keyPlusMinus
	{144, 154, 175, 177, 4, 0x04},		// keyEEX
	{190, 154, 221, 177, 4, 0x02},		// keyDEL
	{236, 154, 267, 177, 4, 0x01},		// keyBackspace

	{  6, 189,  37, 212, 3, 0x20},		// keyAlpha
	{ 58, 189,  99, 212, 3, 0x08},		// key7
	{114, 189, 155, 212, 3, 0x04},		// key8
	{170, 189, 211, 212, 3, 0x02},		// key9
	{226, 189, 267, 212, 3, 0x01},		// keyDivide

	{  6, 224,  37, 247, 2, 0x20},		// keyLeftShift
	{ 58, 224,  99, 247, 2, 0x08},		// key4
	{114, 224, 155, 247, 2, 0x04},		// key5
	{170, 224, 211, 247, 2, 0x02},		// key6
	{226, 224, 267, 247, 2, 0x01},		// keyMultiply

	{  6, 259,  37, 282, 1, 0x20},		// keyRightShift
	{ 58, 259,  99, 282, 1, 0x08},		// key1
	{114, 259, 155, 282, 1, 0x04},		// key2
	{170, 259, 211, 282, 1, 0x02},		// key3
	{226, 259, 267, 282, 1, 0x01},		// keySubtract

	{  6, 294,  37, 317, 4, 0x20},		// keyON
	{ 58, 294,  99, 317, 0, 0x08},		// key0
	{114, 294, 155, 317, 0, 0x04},		// keyPeriod
	{170, 294, 211, 317, 0, 0x02},		// keySPC
	{226, 294, 267, 317, 0, 0x01},		// keyAdd

	{900, 900, 910, 910, 0, 0x00},		// keyAPPS
	{900, 900, 910, 910, 0, 0x00},		// keyMODE
	{900, 900, 910, 910, 0, 0x00},		// keyTOOL
	{900, 900, 910, 910, 0, 0x00},		// keyHIST
	{900, 900, 910, 910, 0, 0x00},		// keyCAT
	{900, 900, 910, 910, 0, 0x00},		// keyEQW
	{900, 900, 910, 910, 0, 0x00},		// keySYMB
	{900, 900, 910, 910, 0, 0x00},		// keyX

	{  4,   4,  51,  16, 0, 0x00},		// keyAbout48GX
	{ 42, 299,  52, 309, 0, 0x00},		// keyOnPlus
	{304,  51, 319,  59, 0, 0x00},		// keyObject
	{288,  74, 316,  93, 0, 0x00}			// keySwitch
};


const keyDef keyLayout320_hp49[] =      // coordinates relative to upper left of display
{
	{  5,  21,  39,  39, 5, 0x01},		// softKeyA
	{ 52,  21,  86,  39, 5, 0x02},		// softKeyB
	{ 99,  21, 133,  39, 5, 0x04},		// softKeyC
	{  5,  48,  39,  66, 5, 0x08},		// softKeyD
	{ 52,  48,  86,  66, 5, 0x10},		// softKeyE
	{ 99,  48, 133,  66, 5, 0x20},		// softKeyF

	{900, 900, 910, 910, 0, 0x00},		// keyMTH
	{900, 900, 910, 910, 0, 0x00},		// keyPRG
	{900, 900, 910, 910, 0, 0x00},		// keyCST
	{139,  82, 179, 103, 2, 0x80},		// keyVAR
	{285, 101, 298, 120, 6, 0x08},		// keyUp
	{231,  82, 272, 103, 0, 0x80},		// keyNXT

	{900, 900, 910, 910, 0, 0x00},		// keyQuote
	{185,  82, 225, 103, 1, 0x80},		// keySTO
	{900, 900, 910, 910, 0, 0x00},		// keyEVAL
	{271, 110, 283, 132, 6, 0x04},		// keyLEFT
	{285, 123, 298, 142, 6, 0x02},		// keyDOWN
	{301, 110, 314, 132, 6, 0x01},		// keyRIGHT

	{274, 150, 317, 171, 2, 0x20},		// keySIN
	{274, 186, 317, 207, 1, 0x20},		// keyCOS
	{274, 222, 317, 243, 0, 0x20},		// keyTAN
	{274, 258, 317, 279, 3, 0x20},		// keySquareRoot
	{274, 294, 317, 315, 4, 0x20},		// keyYtotheX
	{163, 147, 206, 172, 1, 0x10},		// key1OverX

	{216, 291, 259, 317, 0, 0x01},		// keyENTER
	{ 56, 147,  99, 172, 3, 0x10},		// keyPlusMinus
	{  4, 147,  45, 172, 4, 0x10},		// keyEEX
	{900, 900, 910, 910, 0, 0x00},		// keyDEL
	{216, 115, 259, 136, 0, 0x40},		// keyBackspace

	{  4, 183,  45, 208, 7, 0x08},		// keyAlpha
	{ 56, 183,  99, 208, 3, 0x08},		// key7
	{109, 183, 152, 208, 2, 0x08},		// key8
	{163, 183, 206, 208, 1, 0x08},		// key9
	{216, 147, 259, 172, 0, 0x10},		// keyDivide

	{  4, 219,  45, 244, 7, 0x04},		// keyLeftShift
	{ 56, 219,  99, 244, 3, 0x04},		// key4
	{109, 219, 152, 244, 2, 0x04},		// key5
	{163, 219, 206, 244, 1, 0x04},		// key6
	{216, 183, 259, 208, 0, 0x08},		// keyMultiply

	{  4, 255,  45, 280, 7, 0x02},		// keyRightShift
	{ 56, 255,  99, 280, 3, 0x02},		// key1
	{109, 255, 152, 280, 2, 0x02},		// key2
	{163, 255, 206, 280, 1, 0x02},		// key3
	{216, 219, 259, 244, 0, 0x04},		// keySubtract

	{  4, 291,  38, 317, 0, 0x00},		// keyON
	{ 56, 291,  99, 317, 3, 0x01},		// key0
	{109, 291, 152, 317, 2, 0x01},		// keyPeriod
	{163, 291, 206, 317, 1, 0x01},		// keySPC
	{216, 255, 259, 280, 0, 0x02},		// keyAdd

	{  4,  82,  41, 103, 5, 0x80},		// keyAPPS
	{ 47,  82,  87, 103, 4, 0x80},		// keyMODE
	{ 93,  82, 133, 103, 3, 0x80},		// keyTOOL
	{  4, 115,  45, 136, 4, 0x40},		// keyHIST
	{ 56, 115,  99, 136, 3, 0x40},		// keyCAT
	{109, 115, 152, 136, 2, 0x40},		// keyEQW
	{163, 115, 206, 136, 1, 0x40},		// keySYMB
	{109, 147, 152, 172, 2, 0x10},		// keyX

	{  7,   4, 130,  10, 0, 0x00},		// keyAbout48GX
	{ 42, 304,  52, 314, 0, 0x00},		// keyOnPlus
	{304,  51, 319,  59, 0, 0x00},		// keyObject
	{288,  76, 316,  95, 0, 0x00}			// keySwitch
};




void ResetTime(void)
{
	unsigned long long 	ticks,timeoff;
	UInt32				seconds;
	int 				i;
	UInt8				*n;
	char 				val;
	UInt16				crc=0;
	
	seconds = TimGetSeconds();
	ticks = (unsigned long long)seconds*8192;
	ticks += TIME_OFFSET;
	timeoff = ticks + 0x4B0000;
	
	if (optionTarget==HP48SX)
		n=Q->ram+0x52;
	else
		n=Q->ram+0x58;
			
	for (i=0;i<13;i++)
	{
		val = ticks&0xf;
		crc = (crc>>4)^(((crc^val)&0xf)*0x1081);
		*n++ = val;
		ticks>>=4;
	}
	for (i=0;i<4;i++)
	{
		*n++ = (char)(crc&0xf);
		crc>>=4;
	}
	for (i=0;i<13;i++)
	{
		*n++ = timeoff&0xf;
		timeoff>>=4;
	}
	*n=0xf;
}






/*************************************************************************\
*
*  FUNCTION:  UInt16 swapAlphasFor320Layout(UInt16 swapKey)
*
\*************************************************************************/
static UInt16 swapAlphasFor320Layout(UInt16 swapKey)
{
	if (Q->status49GActive)
	{
		switch (swapKey)
		{
			case keyEEX: 		return keyYtotheX;		// Q
			case keyPlusMinus: 	return keySquareRoot;	// R
			case keyX:			return keySIN;			// S
			case key1OverX:		return keyCOS;			// T
			case keyDivide:		return keyTAN;			// U
			case keySIN:		return keyEEX;			// V
			case keyMultiply:	return keyPlusMinus;	// W
			case keyCOS:		return keyX;			// X
			case keySubtract:	return key1OverX;		// Y
			case keyTAN:		return keyDivide;		// Z

			default: return swapKey;
		}
	}
	else
	{
		switch (swapKey)
		{
			case keyPlusMinus: 	return keyCOS;			// T
			case keyEEX: 		return keyTAN;			// U
			case keyDEL: 		return keySquareRoot;	// V
			case keyBackspace: 	return keyYtotheX;		// W
			case keyCOS: 		return key1OverX;		// X
			case keyDivide: 	return keyPlusMinus;	// Y
			case keyTAN: 		return keyEEX;			// Z

			default: return swapKey;
		}
	}
}




/***********************************************************************
 *
 * FUNCTION:    TranslateEventToKey(EventPtr event)
 *
 * DESCRIPTION: This routine translates a penDown/penUp event into
 *              the press/release of a calculator key
 *
 *
 ***********************************************************************/
UInt16 TranslateEventToKey(EventPtr event)
{
	UInt16 i;
	squareDef *virtKey;
	keyDef *key;
	Int16 x,y,tmp;

	x = event->screenX<<1;
	y = event->screenY<<1;
	
	if (statusPortraitViaLandscape)
	{
		tmp=x;
		x=(screenHeight-1)-y;
		y=tmp;
	}
	
	if ((x >= optionPanelX) &&
		(x < displayExtX) &&
		(y >= optionPanelY) &&
		(y <= (optionPanelY+panelHeight)) &&
		(!statusPanelHidden))
	{
		return virtKeyIPSMonitor;
	}

	if ((event->eType != virtualPenDownEvent)&&(event->eType != virtualPenUpEvent))
	{
		virtKey=virtLayout;
		for (i=0;i<maxVirtKeys;i++)
		{
			if ((x >= virtKey->upperLeftX) &&
				(x <= virtKey->lowerRightX) &&
				(y >= virtKey->upperLeftY) &&
				(y <= virtKey->lowerRightY))
			{
				return i+virtKeyBase;
			}
			virtKey++;
		}
	
		if (optionThreeTwentySquared)
		{
			if (statusDisplayMode2x2)
			{
				if ((x >= fullScreenOriginX) &&
					(x <= (fullScreenOriginX+fullScreenExtentX)) &&
					(y >= fullScreenOriginY) &&
					(y <= (fullScreenOriginY+fullScreenExtentY)))
					return virtKeyVideoSwap;
			}
			else
			{
				if ((x >= lcd1x1AnnOriginX) &&
					(x <= (lcd1x1AnnOriginX+lcd1x1AnnExtentX+lcd1x1FSExtentX+2)) &&
					(y >= lcd1x1AnnOriginY) &&
					(y <= (lcd1x1AnnOriginY+lcd1x1AnnExtentY)))
					return virtKeyVideoSwap;
			}
		}
	}
	
	key=keyLayout;
	for (i=0;i<maxKeys;i++)
	{
		if ((x >= key->upperLeftX) &&
			(x <= key->lowerRightX) &&
			(y >= key->upperLeftY) &&
			(y <= key->lowerRightY))
		{
			return i;
		}
		key++;
	}
	return noKey;
}


/*************************************************************************\
*
*  FUNCTION:  keyboard_handler(UInt16 keyIndex, KHType action)
*
\*************************************************************************/
void kbd_handler(UInt16 keyIndex, KHType action) 
{	
	if (action==KEY_PRESS_SINGLE)
	{
		if (keyIndex == keyON)
		{
			Q->IR15X = 0x00800000;  // byteswapped 0x8000
			return;
		}
			
		// are we in 320x320 mode and is the alpha annunciator active? (byteswapped 0x04)
		if ((optionThreeTwentySquared) && (Q->lcdAnnunciatorState&0x0400))
			keyIndex=swapAlphasFor320Layout(keyIndex);	// swap keys to correct mapping

		Q->kbd_row[keyLayout[keyIndex].row]|=keyLayout[keyIndex].mask;
	}
	else
	{
		Q->IR15X = 0x00000000;  
		Q->kbd_row[0]=0;
		Q->kbd_row[1]=0;
		Q->kbd_row[2]=0;
		Q->kbd_row[3]=0;
		Q->kbd_row[4]=0;
		Q->kbd_row[5]=0;
		Q->kbd_row[6]=0;
		Q->kbd_row[7]=0;
		Q->kbd_row[8]=0;
	}
	
	delayCycles=3;
}




/*************************************************************************\
*
*  FUNCTION:  simulate_keypress(UInt16 keyIndex)
*
\*************************************************************************/
void simulate_keypress(UInt16 keyIndex) 
{
	kbd_handler(keyIndex,KEY_PRESS_SINGLE);
	Q->delayEvent=0x0300;   // wait 5 key scan cycles (approx .05 sec) before releasing key
	Q->delayKey=ByteSwap16(keyIndex);  // specify key to release
	Q->delayEvent2=0;
	Q->delayKey2=byteswapped_noKey;
}



/*************************************************************************\
*
*  FUNCTION:  simulate_2L_keypress(UInt16 keyIndex)
*
\*************************************************************************/
inline void simulate_2L_keypress(UInt16 keyIndex1,UInt16 keyIndex2) 
{
	if (((keyIndex1==keyAlpha)&&(Q->lcdAnnunciatorState&0x0400)) ||
	    ((keyIndex1==keyLeftShift)&&(Q->lcdAnnunciatorState&0x0100)) ||
	    ((keyIndex1==keyRightShift)&&(Q->lcdAnnunciatorState&0x0200)))
	{
		// no need to do two stage simulation, we're already in shift mode
		kbd_handler(keyIndex2,KEY_PRESS_SINGLE);
		Q->delayEvent=0x0300;   // wait 5 key scan cycles (approx .05 sec) before releasing key
		Q->delayKey=ByteSwap16(keyIndex2);  // specify key to release
		Q->delayEvent2=0;
		Q->delayKey2=byteswapped_noKey;
	}
	else
	{
		// two stage: send shift modifier, then send key
		kbd_handler(keyIndex1,KEY_PRESS_SINGLE);
		Q->delayEvent=0x0300;   
		Q->delayKey=ByteSwap16(keyIndex1);  // specify key to release
		Q->delayEvent2=0x0300;   
		Q->delayKey2=ByteSwap16(keyIndex2);  // specify key to release
	}
}






/*************************************************************************\
*
*  FUNCTION:  void AdjustTime(void)
*
*  The sole purpose of this function is to adjust the internal clock onz
*  the emulator target to account for the fact that all emulator operations
*  are suspended during user dialog interaction, and there is no other way
*  to catch up the clock.
*
\*************************************************************************/
void AdjustTime(void)
{
	if (!statusRUN) return; 
	MemSemaphoreReserve(true);
	Q->ram = (UInt8*)ByteSwap32(Q->ram);
	ResetTime();  // not a1 safe
	Q->ram = (UInt8*)ByteSwap32(Q->ram);
	//prevT2TickStamp = TimGetTicks();
	MemSemaphoreRelease(true);
}
	




#ifdef COMPILE_BATTERY_TEST
UInt32 cbtCount=0;
Int16 prevMinute=0;
Int16 reportCount=0;
FileHand blrFileHandle;
Boolean stopReport=false;

static void DisplayBatteryInfo(void)
{
	char 			tmp[80],tmp2[80];
	UInt8 			percent;
	DateTimeType 	currentDateTime;
	char 			timeString[40];
	UInt16 			attributes;
	LocalID			id;
	Err				error;
	
	if (stopReport) return;
	
	TimSecondsToDateTime(TimGetSeconds(),&currentDateTime);

	if (reportCount==0)
	{
		// first time in procedure, get filehandle data
		blrFileHandle = FileOpen(0, "Power48 battery level report", 'batL', appFileCreator, fileModeReadWrite, &error);

		TimeToAscii(currentDateTime.hour,currentDateTime.minute,tfColonAMPM,timeString);

		StrCopy(tmp,"START REPORT at ");
		StrCat(tmp,timeString);
		StrCat(tmp," \n");
		FileWrite(blrFileHandle,tmp,1,StrLen(tmp),NULL);
		StrCopy(tmp," \n");
		FileWrite(blrFileHandle,tmp,1,StrLen(tmp),NULL);
	}
		
	SysBatteryInfo(false,NULL,NULL,NULL,NULL,NULL,&percent);
	StrPrintF(tmp," Batt:%u%%  ",percent);
	ErrorPrint(tmp,false);
	
	EvtResetAutoOffTimer();
	if ((cbtCount++ & 0x1f)==0)
	{
		simulate_keypress(keyON);
	}
	
	if (currentDateTime.minute != prevMinute)
	{
		reportCount++;
		prevMinute = currentDateTime.minute;
		TimeToAscii(currentDateTime.hour,currentDateTime.minute,tfColonAMPM,timeString);
		StrPrintF(tmp2,"Report #%d -- ",reportCount);
		StrCopy(tmp,tmp2);
		StrCat(tmp,timeString);
		StrPrintF(tmp2," -- Battery level %u%%  \n",percent);
		StrCat(tmp,tmp2);

		FileWrite(blrFileHandle,tmp,1,StrLen(tmp),NULL);

		if (percent<=25)
		{
			// we've reached 25% of battery capacity, stop reporting
			stopReport=true;
			StrCopy(tmp," \n");
			FileWrite(blrFileHandle,tmp,1,StrLen(tmp),NULL);
			StrCopy(tmp,"END REPORT \n");
			FileWrite(blrFileHandle,tmp,1,StrLen(tmp),NULL);
			FileClose(blrFileHandle);
			
			// set the backup bit on the file
			id = DmFindDatabase(0, "Power48 battery level report");
			DmDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
						   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			attributes |= dmHdrAttrBackup;
			DmSetDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
							  NULL, NULL, NULL, NULL, NULL, NULL, NULL);
							  
			Q->statusQuit=1;
		}		
	}
}
#endif




/*************************************************************************\
*
*  FUNCTION:  Boolean ProcessPalmEvent(void)
*
\*************************************************************************/
Boolean ProcessPalmEvent(UInt32 timeout)
{
	Boolean genKPInterrupt=false;
	Boolean swallowEvent=false;
	Boolean duplicateChordKey=false;
	EventType event;
	Int16 i;
	UInt16 keyOne=noKey,keyTwo=noKey;
	Boolean is49;
	UInt32 mark;
	UInt32 keyState;
	UInt16 newDirKeyValue;
		
	event.eType=nilEvent;
	
	is49 = Q->status49GActive;
	
	if (statusRUN) MemSemaphoreRelease(true);
		
	mark = TimGetTicks();
	if (mark>updateMark)
	{
		#ifdef COMPILE_BATTERY_TEST
			DisplayBatteryInfo();
		#endif
		if ((optionPanelOpen) && (!statusPanelHidden)) UpdatePanelDisplay();
		updateMark = mark+TICKS_PER_UPDATE;
	}
	
	if (delayCycles>0)
	{
		delayCycles--;
		if (statusRUN) MemSemaphoreReserve(true);
		return false;
	}
	
	if (status5WayNavPresent)
	{

		keyState=KeyCurrentState();
		if (status5WayNavType==PALMONE)
		{
			if (keyState&keyBitPageUp) newDirKeyValue=keyUp;
			else if (keyState&keyBitPageDown) newDirKeyValue=keyDOWN;
			else if (keyState&keyBitNavLeft) newDirKeyValue=keyLEFT;
			else if (keyState&keyBitNavRight) newDirKeyValue=keyRIGHT;
			else newDirKeyValue=noKey;
		}
		else
		{
			if (keyState&keyBitRockerUp) newDirKeyValue=keyUp;
			else if (keyState&keyBitRockerDown) newDirKeyValue=keyDOWN;
			else if (keyState&keyBitRockerLeft) newDirKeyValue=keyLEFT;
			else if (keyState&keyBitRockerRight) newDirKeyValue=keyRIGHT;
			else if (keyState&keyBitRockerCenter) newDirKeyValue=keyENTER;
			else newDirKeyValue=noKey;
		}
		
		if (oldDirKeyValue!=newDirKeyValue)
		{
			//something has changed
			if (oldDirKeyValue==noKey)
			{
				// User has just pushed a direction on the 5way nav
				event.eType=virtualPenDownEvent;
				event.screenX=(keyLayout[newDirKeyValue].upperLeftX>>1)+1;
				event.screenY=(keyLayout[newDirKeyValue].upperLeftY>>1)+1;
				EvtAddEventToQueue(&event);
				oldDirKeyValue=newDirKeyValue;
			}
			else if (newDirKeyValue==noKey)
			{
				// User has just released a direction on the 5way nav
				event.eType=virtualPenUpEvent;
				event.screenX=(keyLayout[oldDirKeyValue].upperLeftX>>1)+1;
				event.screenY=(keyLayout[oldDirKeyValue].upperLeftY>>1)+1;
				EvtAddEventToQueue(&event);
				oldDirKeyValue=noKey;
			}
			else
			{
				// User has just shifted directions on the 5way nav without releasing
				event.eType=virtualPenUpEvent;
				event.screenX=(keyLayout[oldDirKeyValue].upperLeftX>>1)+1;
				event.screenY=(keyLayout[oldDirKeyValue].upperLeftY>>1)+1;
				EvtAddEventToQueue(&event);
				
				event.eType=virtualPenDownEvent;
				event.screenX=(keyLayout[newDirKeyValue].upperLeftX>>1)+1;
				event.screenY=(keyLayout[newDirKeyValue].upperLeftY>>1)+1;
				EvtAddEventToQueue(&event);
				oldDirKeyValue=newDirKeyValue;
			}
		}
	}
	
	WinSetCoordinateSystem(kCoordinatesStandard);
	EvtGetEvent(&event,timeout);
	WinSetCoordinateSystem(kCoordinatesNative);
	
	if ((TimGetTicks()>(keyHoldTimeMark+70)) && (!keyShifterOn) && (keyHoldTimerStarted))
	{
		keyShifterOn=true;
		shiftedKey=keyReleasePending;
		LockVirtualKey(keyReleasePending);
	}

	if (event.eType==nilEvent) 
	{
		if (statusRUN) MemSemaphoreReserve(true);
		return false;   //Only early exit point
	}
		
   	if ((event.eType==penDownEvent) || (event.eType==virtualPenDownEvent))
    {
    	if (!keyChordingOn)
    		keyReleasePending = noKey;
		scanCode = TranslateEventToKey(&event);
		if (scanCode != noKey)
		{
    		if (optionKeyClick) SndPlaySystemSound(sndClick);
			if ((scanCode==keyAbout48GX) || (scanCode==keyObject))
			{
				if (!keyChordingOn)  // Don't allow special key press during chording
				{
					DepressVirtualKey(scanCode);
					keyReleasePending=scanCode;
				}
				else 
				{
					keyReleasePending = noKey;
				}
			}
			else if ((scanCode==keyLeftShift) || (scanCode==keyRightShift) || (scanCode==keyAlpha))
			{
				if (!keyChordingOn)  // Don't allow special key press during chording
				{
					if ((keyShifterOn) && (shiftedKey==scanCode))   // user pressed a shifter that is already locked, so release
					{
						keyReleasePending = scanCode;
					}
					else
					{
						if (keyShifterOn)  // user pressed a different shifter then the one already locked
						{
							kbd_handler(noKey,KEY_RELEASE_ALL);
							ReleaseVirtualKey(shiftedKey);
							keyShifterOn=false;
							shiftedKey=noKey;
						}
						DepressVirtualKey(scanCode);
						keyReleasePending = scanCode;
						keyHoldTimeMark=mark;
						keyHoldTimerStarted=true;
						kbd_handler(scanCode,KEY_PRESS_SINGLE);
						genKPInterrupt=true;
					}
				}
				else 
				{
					keyReleasePending = noKey;
				}
			}
			else if (scanCode==virtKeyIPSMonitor)
			{
				if (!keyChordingOn)  // Don't allow special key press during chording
				{
					HandlePanelPress(event.screenX<<1,event.screenY<<1);
				}
				else 
				{
					keyReleasePending = noKey;
				}
			}
			else if (scanCode&virtKeyBase)
			{
				if (!keyChordingOn)  // Don't allow special key press during chording
				{
					keyReleasePending=scanCode;
				}
				else 
				{
					keyReleasePending = noKey;
				}
			}
			else if (scanCode==keySwitch)
			{
				if (!keyChordingOn)  // Don't allow special key press during chording
				{
					DisplaySwitchGraphic();
				}
				else 
				{
					keyReleasePending = noKey;
				}
			}
			else if ((scanCode==keyON) && (keyChordingOn))
			{
				// If already chording, release chord
				keyChordingOn=false;
				for (i=numCodesInBuf-1;i>=0;i--)
				{
					ReleaseVirtualKey(chordingBuf[i]);
					chordingBuf[i]=noKey;
				}
				kbd_handler(noKey, KEY_RELEASE_ALL);
				numCodesInBuf=0;
				ReleaseVirtualKey(keyOnPlus);
			}
			else if (scanCode==keyOnPlus)
			{
				if (keyChordingOn)  // If already chording, release chord
				{
					keyChordingOn=false;
					for (i=numCodesInBuf-1;i>=0;i--)
					{
						ReleaseVirtualKey(chordingBuf[i]);
						chordingBuf[i]=noKey;
					}
					kbd_handler(noKey, KEY_RELEASE_ALL);
					numCodesInBuf=0;
					ReleaseVirtualKey(keyOnPlus);
				}
				else  // start the chord with the ON key
				{
					DepressVirtualKey(keyOnPlus);
					kbd_handler(keyON,KEY_PRESS_SINGLE);
					DepressVirtualKey(keyON);
					chordingBuf[numCodesInBuf++] = keyON;
					genKPInterrupt=true;
					keyChordingOn=true;
				}
			}
			else
			{
				//pass keypress to HP
				if (keyChordingOn)  // queue up key press in chord (key buffer)
				{
					for (i=0;i<numCodesInBuf;i++)      // check to make sure the user hasn't pressed a key
						if (scanCode==chordingBuf[i])  // that's already in the chording buffer
							duplicateChordKey=true;
					if (!duplicateChordKey)
					{
						if (numCodesInBuf<5)
						{
							kbd_handler(scanCode,KEY_PRESS_SINGLE);
							DepressVirtualKey(scanCode);
							chordingBuf[numCodesInBuf++] = scanCode;
							genKPInterrupt=true;
						}
					}
				}
				else  // just a regular single key press
				{
					kbd_handler(scanCode,KEY_PRESS_SINGLE);
					if ((!optionThreeTwentySquared) || (!statusDisplayMode2x2) || (event.eType!=virtualPenDownEvent))
						DepressVirtualKey(scanCode);
					keyReleasePending = scanCode;
					genKPInterrupt=true;
				}
			}
			swallowEvent=true;
		}
	}
	if ((event.eType==penUpEvent) || (event.eType==virtualPenUpEvent))
	{
		// inform HP of key release
		scanCode = TranslateEventToKey(&event);
		if (keyReleasePending != noKey)
		{
			if (keyReleasePending&virtKeyBase)
			{
				if (scanCode==keyReleasePending)   // Make sure user didn't wander off the key before releasing.
				{
					switch (scanCode)
					{
						case virtKeyVideoSwap:
							// toggle LCD screen
							toggle_lcd();
							AdjustTime();
							break;
						case virtKeySoftKeyA:simulate_keypress(softKeyA);genKPInterrupt=true;break;
						case virtKeySoftKeyB:simulate_keypress(softKeyB);genKPInterrupt=true;break;
						case virtKeySoftKeyC:simulate_keypress(softKeyC);genKPInterrupt=true;break;
						case virtKeySoftKeyD:simulate_keypress(softKeyD);genKPInterrupt=true;break;
						case virtKeySoftKeyE:simulate_keypress(softKeyE);genKPInterrupt=true;break;
						case virtKeySoftKeyF:simulate_keypress(softKeyF);genKPInterrupt=true;break;
						case virtKeyLabelNxt:simulate_keypress(keyNXT);genKPInterrupt=true;break;
						case virtKeyLabelPrev:simulate_2L_keypress(keyLeftShift,keyNXT);genKPInterrupt=true;break;
						default: break;
					}
				}
			}
			else if (keyReleasePending == keyAbout48GX)
			{
				ReleaseVirtualKey(keyAbout48GX);
				if (scanCode==keyAbout48GX)   // Make sure user didn't wander off the key before releasing.
				{
					// Start displaying the About Power48 screens
				#ifdef TRACE_ENABLED
					if (Q->traceInProgress)
						TraceClose();
					else
						TraceOpen();
				#else
					DisplayAboutScreen();
				#endif
					
					AdjustTime();
				}
			}
			else if (keyReleasePending == keyObject)
			{
				ReleaseVirtualKey(keyObject);
				if ((scanCode==keyObject) && (statusRUN))
				{
					// Bring up Object load /save dialog
					OpenObjectLoadSaveDialog();
					AdjustTime();
				}
			}
			else if ((keyReleasePending==keyLeftShift) || (keyReleasePending==keyRightShift) || (keyReleasePending==keyAlpha))
			{
				if ((TimGetTicks()>(keyHoldTimeMark+70)) &&  (keyHoldTimerStarted))
				{
					// do nothing
				}
				else
				{
					shiftedKey=noKey;
					keyShifterOn=false;
					kbd_handler(noKey,KEY_RELEASE_ALL);
					ReleaseVirtualKey(keyReleasePending);
					keyReleasePending = noKey;
				}
			}
			else if (!keyChordingOn)  // If we're not chording, then go ahead and release key
			{
				kbd_handler(noKey,KEY_RELEASE_ALL);
				if ((!optionThreeTwentySquared) || (!statusDisplayMode2x2) || (event.eType!=virtualPenUpEvent))
					ReleaseVirtualKey(keyReleasePending);
				if (keyShifterOn)
				{
					Q->delayEvent=0x0300;   // wait 3 key scan cycles (approx .03 sec) before releasing key
					Q->delayKey=ByteSwap16(shiftedKey);  // specify key to release
					Q->delayEvent2=0;
					Q->delayKey2=byteswapped_noKey;
					ReleaseVirtualKey(shiftedKey);
					shiftedKey=noKey;
					keyShifterOn=false;
				}
				keyReleasePending = noKey;
			}
			swallowEvent=true;
		}
		keyHoldTimerStarted=false;
	}
	if (event.eType==keyDownEvent)
	{
		switch (event.data.keyDown.chr)
		{
			case vchrMenu:
				// Bring up the preference screen
				DisplayPreferenceScreen();
				AdjustTime();
				swallowEvent=true;
				break;
				
			// Virtual or hard keystrokes from graffiti or keyboard
			case chrDigitZero:			keyOne=key0;break;
			case chrDigitOne:			keyOne=key1;break;
			case chrDigitTwo:			keyOne=key2;break;
			case chrDigitThree:			keyOne=key3;break;
			case chrDigitFour:			keyOne=key4;break;
			case chrDigitFive:			keyOne=key5;break;
			case chrDigitSix:			keyOne=key6;break;
			case chrDigitSeven:			keyOne=key7;break;
			case chrDigitEight:			keyOne=key8;break;
			case chrDigitNine:			keyOne=key9;break;
			case chrLineFeed:			keyOne=keyENTER;break;
			case chrSpace:				keyOne=keySPC;break;
			case chrBackspace:			keyOne=keyBackspace;break;



			case chrQuotationMark:		keyOne=keyRightShift;keyTwo=(is49)?keyMultiply:keySubtract;break;
			case chrNumberSign:			keyOne=(is49)?keyLeftShift:keyRightShift;keyTwo=(is49)?key3:keyDivide;break;
			case chrCarriageReturn:		keyOne=keyRightShift;keyTwo=keyPeriod;break;
			case chrApostrophe:			keyOne=(is49)?keyRightShift:keyQuote;keyTwo=(is49)?keyEQW:noKey;break;
			case chrLeftParenthesis:	keyOne=keyLeftShift;keyTwo=(is49)?keySubtract:keyDivide;break;
			case chrRightParenthesis:	keyOne=keyLeftShift;keyTwo=(is49)?keySubtract:keyDivide;break;
			case chrAsterisk:			keyOne=keyMultiply;break;
			case chrPlusSign:			keyOne=keyAdd;break;
			case chrComma:				keyOne=(is49)?keyRightShift:keyLeftShift;keyTwo=(is49)?keySPC:keyPeriod;break;
			case chrHyphenMinus:		keyOne=keySubtract;break;
			case chrColon:				keyOne=(is49)?keyLeftShift:keyRightShift;keyTwo=(is49)?keyPeriod:keyAdd;break;
			case chrLeftSquareBracket:	keyOne=keyLeftShift;keyTwo=keyMultiply;break;
			case chrRightSquareBracket:	keyOne=keyLeftShift;keyTwo=keyMultiply;break;
			case chrLeftCurlyBracket:	keyOne=keyLeftShift;keyTwo=keyAdd;break;
			case chrRightCurlyBracket:	keyOne=keyLeftShift;keyTwo=keyAdd;break;
			case chrFullStop:			keyOne=keyPeriod;break;
			case chrSolidus:			keyOne=keyDivide;break;
			case chrLessThanSign:		keyOne=(is49)?keyRightShift:keyLeftShift;keyTwo=(is49)?keyX:keySubtract;break;
			case chrGreaterThanSign:	keyOne=(is49)?keyRightShift:keyLeftShift;keyTwo=(is49)?key1OverX:keySubtract;break;
			case chrEqualsSign:			keyOne=(is49)?keyRightShift:keyLeftShift;keyTwo=(is49)?keyPlusMinus:key0;break;


			case vchrPageUp:			keyOne=keyUp;break;
			case vchrPageDown:			keyOne=keyDOWN;break;
			case chrLeftArrow:			keyOne=keyLEFT;break;
			case chrRightArrow:			keyOne=keyRIGHT;break;
			case chrUpArrow:			keyOne=keyUp;break;
			case chrDownArrow:			keyOne=keyDOWN;break;
			
			case chrSmall_A:
			case chrCapital_A:keyOne=keyAlpha;keyTwo=softKeyA;break;
			case chrSmall_B:
			case chrCapital_B:keyOne=keyAlpha;keyTwo=softKeyB;break;
			case chrSmall_C:
			case chrCapital_C:keyOne=keyAlpha;keyTwo=softKeyC;break;
			case chrSmall_D:
			case chrCapital_D:keyOne=keyAlpha;keyTwo=softKeyD;break;
			case chrSmall_E:
			case chrCapital_E:keyOne=keyAlpha;keyTwo=softKeyE;break;
			case chrSmall_F:
			case chrCapital_F:keyOne=keyAlpha;keyTwo=softKeyF;break;
			case chrSmall_G:
			case chrCapital_G:keyOne=keyAlpha;keyTwo=(is49)?keyAPPS:keyMTH;break;
			case chrSmall_H:
			case chrCapital_H:keyOne=keyAlpha;keyTwo=(is49)?keyMODE:keyPRG;break;
			case chrSmall_I:
			case chrCapital_I:keyOne=keyAlpha;keyTwo=(is49)?keyTOOL:keyCST;break;
			case chrSmall_J:
			case chrCapital_J:keyOne=keyAlpha;keyTwo=keyVAR;break;
			case chrSmall_K:
			case chrCapital_K:keyOne=keyAlpha;keyTwo=(is49)?keySTO:keyUp;break;
			case chrSmall_L:
			case chrCapital_L:keyOne=keyAlpha;keyTwo=keyNXT;break;
			case chrSmall_M:
			case chrCapital_M:keyOne=keyAlpha;keyTwo=(is49)?keyHIST:keyQuote;break;
			case chrSmall_N:
			case chrCapital_N:keyOne=keyAlpha;keyTwo=(is49)?keyCAT:keySTO;break;
			case chrSmall_O:
			case chrCapital_O:keyOne=keyAlpha;keyTwo=(is49)?keyEQW:keyEVAL;break;
			case chrSmall_P:
			case chrCapital_P:keyOne=keyAlpha;keyTwo=(is49)?keySYMB:keyLEFT;break;
			case chrSmall_Q:
			case chrCapital_Q:keyOne=keyAlpha;keyTwo=(is49)?keyYtotheX:keyDOWN;break;
			case chrSmall_R:
			case chrCapital_R:keyOne=keyAlpha;keyTwo=(is49)?keySquareRoot:keyRIGHT;break;
			case chrSmall_S:
			case chrCapital_S:keyOne=keyAlpha;keyTwo=keySIN;break;
			case chrSmall_T:
			case chrCapital_T:keyOne=keyAlpha;keyTwo=keyCOS;break;
			case chrSmall_U:
			case chrCapital_U:keyOne=keyAlpha;keyTwo=keyTAN;break;
			case chrSmall_V:
			case chrCapital_V:keyOne=keyAlpha;keyTwo=(is49)?keyEEX:keySquareRoot;break;
			case chrSmall_W:
			case chrCapital_W:keyOne=keyAlpha;keyTwo=(is49)?keyPlusMinus:keyYtotheX;break;
			case chrSmall_X:
			case chrCapital_X:keyOne=keyAlpha;keyTwo=(is49)?keyX:key1OverX;break;
			case chrSmall_Y:
			case chrCapital_Y:keyOne=keyAlpha;keyTwo=(is49)?key1OverX:keyPlusMinus;break;
			case chrSmall_Z:
			case chrCapital_Z:keyOne=keyAlpha;keyTwo=(is49)?keyDivide:keyEEX;break;
			
			// toggle support
			case vchrHard5:
			case vchrJogBack:
			case vchrGarminEscape:				
				if (optionThreeTwentySquared) 
				{
					toggle_lcd();
					AdjustTime();
					if (statusRUN) MemSemaphoreReserve(true);
					return false;
				}
				break;
				
			// JogDial input
			case vchrJogUp:		keyOne=keyUp;break;
			case vchrJogDown:	keyOne=keyDOWN;break;
			case vchrJogPush:	keyOne=keyENTER;break;
			
			// Garmin Thumbwheel input
			case vchrGarminThumbWheelUp:	keyOne=keyUp;break;
			case vchrGarminThumbWheelDown:	keyOne=keyDOWN;break;
			case vchrGarminThumbWheelIn:	keyOne=keyENTER;break;

			// T3 slider opening/closing
			case vchrSliderClose:
				//PINSetInputAreaState(pinInputAreaOpen);
				break;
			case vchrSliderOpen:
				PINSetInputAreaState(pinInputAreaClosed);
				break;
				
			default:break;
		}
		
		if (keyOne!=noKey)
		{
    		if (optionKeyClick) SndPlaySystemSound(sndClick);
			if (keyTwo!=noKey)
				simulate_2L_keypress(keyOne,keyTwo);
			else
				simulate_keypress(keyOne);
			genKPInterrupt=true;
			swallowEvent=true;
		}
		keyHoldTimerStarted=false;
	}
	if (event.eType==invokeColorSelectorEvent)
	{
		ColorSelectorScreen();
		AdjustTime();
		swallowEvent=true;
	}
	if ((event.eType==volumeUnmountedEvent) && (statusRUN))
	{
		if (!statusInternalFiles)
		{
			if (event.data.generic.datum[0]==msVolRefNum)
			{
				statusInUnmountDialog = true;
				if (OpenAlertConfirmDialog(alert,pleaseReinsertCardString,false))
					statusVolumeMounted=false;
				else
					statusVolumeMounted=true;
				statusInUnmountDialog = false;
				AdjustTime();
			}
			swallowEvent=true;
		}
	}
	if (event.eType==volumeMountedEvent)
	{
		if (!statusInternalFiles)
		{
			CheckForRemount();
			AdjustTime();
			swallowEvent=true;
		}
	}
	if (event.eType==appStopEvent)  // respond to app-switching events.
	{
		Q->statusQuit=true;
		swallowEvent=true;
	}
	if (event.eType==winDisplayChangedEvent)
	{
		if (CheckDisplayTypeForGUIRedraw()) GUIRedraw();
		swallowEvent=true;
	}

	if (!swallowEvent)
	{
		WinSetCoordinateSystem(kCoordinatesStandard);
		if (! SysHandleEvent(&event))  // respond to all system events, like power-down, etc...
		{
			FrmDispatchEvent(&event);
		}
		WinSetCoordinateSystem(kCoordinatesNative);
	}
	if (statusRUN) MemSemaphoreReserve(true);
	return genKPInterrupt;
}



/*************************************************************************\
*
*  FUNCTION:  DepressVirtualKey(UInt16 keyCode)
*
\*************************************************************************/
void DepressVirtualKey(UInt16 keyCode)
{
	RectangleType 	myRect,vacRect,frameRect;
	RGBColorType  	myRGB,prevRGB;
	
	WinSetDrawWindow(mainDisplayWindow);
	
	if (keyCode!=noKey)
	{
		if (statusPortraitViaLandscape)
		{
			myRect.extent.x = keyLayout[keyCode].lowerRightY-keyLayout[keyCode].upperLeftY+4;
			myRect.extent.y = keyLayout[keyCode].lowerRightX-keyLayout[keyCode].upperLeftX+3;
			myRect.topLeft.x = keyLayout[keyCode].upperLeftY;
			myRect.topLeft.y = screenHeight-1-keyLayout[keyCode].upperLeftX-myRect.extent.y+1;
		
			//if (myRect.topLeft.x<=4) { myRect.topLeft.x=0; myRect.extent.x+=4; }
			//if (myRect.topLeft.y<=4) { myRect.topLeft.y=0; myRect.extent.y+=4; }
		}
		else
		{
			myRect.topLeft.x = keyLayout[keyCode].upperLeftX;
			myRect.topLeft.y = keyLayout[keyCode].upperLeftY;
			myRect.extent.x = keyLayout[keyCode].lowerRightX-keyLayout[keyCode].upperLeftX+3;
			myRect.extent.y = keyLayout[keyCode].lowerRightY-keyLayout[keyCode].upperLeftY+4;
		
			if (myRect.topLeft.x<=4) { myRect.topLeft.x=0; myRect.extent.x+=4; }
			if (myRect.topLeft.y<=4) { myRect.topLeft.y=0; myRect.extent.y+=4; }
		}
		
		
		WinGetPixelRGB(myRect.topLeft.x,myRect.topLeft.y,&myRGB);
		WinSetForeColorRGB(&myRGB,&prevRGB);
		if (statusPortraitViaLandscape)
			WinScrollRectangle(&myRect, winRight, 3, &vacRect);
		else
			WinScrollRectangle(&myRect, winDown, 3, &vacRect);
		WinDrawRectangle(&vacRect,0);
		if (statusPortraitViaLandscape)
			WinScrollRectangle(&myRect, winUp, 2, &vacRect);
		else
			WinScrollRectangle(&myRect, winRight, 2, &vacRect);
		WinDrawRectangle(&vacRect,0);

		if (!Q->status49GActive)
		{							
			WinSetForeColorRGB(&redRGB,NULL);
			WinSetBackColorRGB(&whiteRGB,NULL);
			WinSetDrawMode(winSwap);
			WinPaintRectangle(&myRect,0);
		}
		WinSetDrawMode(winPaint);
		if (keyCode==keyOnPlus)
		{
			WinSetForeColorRGB(&redRGB,NULL);
			if (statusPortraitViaLandscape)
			{
				frameRect.extent.x = keyLayout[keyCode].lowerRightY-keyLayout[keyCode].upperLeftY-2;
				frameRect.extent.y = keyLayout[keyCode].lowerRightX-keyLayout[keyCode].upperLeftX-1;
				frameRect.topLeft.x = keyLayout[keyCode].upperLeftY+3;
				frameRect.topLeft.y = screenHeight-1-(keyLayout[keyCode].upperLeftX+2)-frameRect.extent.y+1;
			}
			else
			{
				frameRect.topLeft.x = keyLayout[keyCode].upperLeftX+2;
				frameRect.topLeft.y = keyLayout[keyCode].upperLeftY+3;
				frameRect.extent.x = keyLayout[keyCode].lowerRightX-keyLayout[keyCode].upperLeftX-1;
				frameRect.extent.y = keyLayout[keyCode].lowerRightY-keyLayout[keyCode].upperLeftY-2;
			}
			WinDrawRectangleFrame(simpleFrame,&frameRect);
		}
		WinSetForeColorRGB(&prevRGB,NULL);
	}
}



/*************************************************************************\
*
*  FUNCTION:  ReleaseVirtualKey(UInt16 keyCode)
*
\*************************************************************************/
void ReleaseVirtualKey(UInt16 keyCode)
{
	RectangleType myRect;
	Int16 i;
	
	WinSetDrawWindow(mainDisplayWindow);
	
	if (keyCode!=noKey)
	{
		if (statusPortraitViaLandscape)
		{
			myRect.extent.x = keyLayout[keyCode].lowerRightY-keyLayout[keyCode].upperLeftY+4;
			myRect.extent.y = keyLayout[keyCode].lowerRightX-keyLayout[keyCode].upperLeftX+3;
			myRect.topLeft.x = keyLayout[keyCode].upperLeftY;
			myRect.topLeft.y = screenHeight-1-keyLayout[keyCode].upperLeftX-myRect.extent.y+1;
		
			//if (myRect.topLeft.x<=4) { myRect.topLeft.x=0; myRect.extent.x+=4; }
			//if (myRect.topLeft.y<=4) { myRect.topLeft.y=0; myRect.extent.y+=4; }
		}
		else
		{
			myRect.topLeft.x = keyLayout[keyCode].upperLeftX;
			myRect.topLeft.y = keyLayout[keyCode].upperLeftY;
			myRect.extent.x = keyLayout[keyCode].lowerRightX-keyLayout[keyCode].upperLeftX+3;
			myRect.extent.y = keyLayout[keyCode].lowerRightY-keyLayout[keyCode].upperLeftY+4;
		
			if (myRect.topLeft.x<=4) { myRect.topLeft.x=0; myRect.extent.x+=4; }
			if (myRect.topLeft.y<=4) { myRect.topLeft.y=0; myRect.extent.y+=4; }
		}
		
		WinCopyRectangle(backStoreWindow, mainDisplayWindow, &myRect, 
							myRect.topLeft.x, myRect.topLeft.y,winPaint);
							
		if ((optionThreeTwentySquared)&&(keyCode>=softKeyA)&&(keyCode<=softKeyF))
		{
			for (i=softLabelStart;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal menu redraw
		}
	}
}




/*************************************************************************\
*
*  FUNCTION:  LockVirtualKey(UInt16 keyCode)
*
\*************************************************************************/
void LockVirtualKey(UInt16 keyCode)
{
	Coord x,y,h;
	RGBColorType prevRGB;
	
	WinSetDrawWindow(mainDisplayWindow);
	
	if (keyCode!=noKey)
	{
		if (statusPortraitViaLandscape)
		{
			h = keyLayout[keyCode].lowerRightX-keyLayout[keyCode].upperLeftX+3;
			x = keyLayout[keyCode].upperLeftY;
			y = screenHeight-1-keyLayout[keyCode].upperLeftX-h+1;
		}
		else
		{
			x = keyLayout[keyCode].upperLeftX;
			y = keyLayout[keyCode].upperLeftY;
		}
		
		x+=5;
		y+=5;
		
		FntSetFont(boldFont);
		WinSetTextColorRGB(&greyRGB,&prevRGB);

		DrawCharsExpanded("Lock", 4, x, y, 0, winOverlay, SINGLE_DENSITY);
		
		WinSetTextColorRGB(&prevRGB,NULL);
	}
}





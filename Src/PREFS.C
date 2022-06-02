/*************************************************************************\
*
*  PREFS.C
*
*  This module is responsible for implementing the preferences screens
*
*  (c)2002 by Robert Hildinger
*
\*************************************************************************/

#include <PalmOS.h>
#include "Power48Rsc.h"
#include <SonyCLIE.h>
#include "Power48.h"
#include "Power48_core.h"
#include <PceNativeCall.h>


Boolean tempTarget;
Boolean tempVerbose;
Boolean tempScrollFast;
Boolean tempThreeTwentySquared;

Boolean	temp48GXBackdrop;
Boolean	temp48SXBackdrop;
Boolean	temp49GBackdrop;
Boolean	temp48GXColorCycle;
Boolean	temp48SXColorCycle;
Boolean	temp49GColorCycle;
Boolean	temp48GX128KExpansion;
Boolean	temp48SX128KExpansion;
Boolean	tempGenTrueSpeed;
Boolean	tempGenDisplayMonitor;
Boolean	tempGenKeyClick;

WinHandle miniToWin;
WinHandle miniFromWin;
Boolean	displayMini=false;
Boolean	skipAnimation=false;


const prefButton prefLayout[] =   // coordinates relative to upper left of pref dialog
{
	{   3,  35,  84,  60, 0xE },		// prefGeneral
	{  89,  35, 150,  60, 0xD },		// pref48GX
	{ 155,  35, 216,  60, 0xB },		// pref48SX
	{ 221,  35, 278,  60, 0x7 },		// pref49G
	{  19, 251,  88, 285, 0xF },		// prefCancel
	{ 193, 251, 262, 285, 0xF },		// prefOK
	
	{   6,  88,  22, 104, 0x1 },		// prefGenDisplayMonitor
	{   6, 117,  22, 133, 0x1 },		// prefGenTrueSpeed
	{   6, 146,  22, 162, 0x1 },		// prefGenKeyClick
	{ 145,  88, 161, 104, 0x1 },		// prefGenVerbose
	{ 168, 126, 184, 142, 0x1 },		// prefGenVerboseSlow
	{ 219, 126, 235, 142, 0x1 },		// prefGenVerboseFast
	{ 147, 160, 208, 191, 0x1 },		// prefGenReset
	{ 147, 203, 208, 234, 0x1 },		// prefGenColorSelect
	
	{ 125,  75, 141,  91, 0x2 },		// pref48GXBackdrop
	{ 125, 104, 141, 120, 0x2 },		// pref48GXColorCycle
	{ 125, 138, 141, 154, 0x2 },		// pref48GX128KExpansion
	{  31, 218,  47, 234, 0x2 },		// pref48GXActive
	
	{ 125,  75, 141,  91, 0x4 },		// pref48SXBackdrop
	{ 125, 104, 141, 120, 0x4 },		// pref48SXColorCycle
	{ 125, 138, 141, 154, 0x4 },		// pref48SX128KExpansion
	{  31, 218,  47, 234, 0x4 },		// pref48SXActive
	
	{ 125,  75, 141,  91, 0x8 },		// pref49GBackdrop
	{ 125, 104, 141, 120, 0x8 },		// pref49GColorCycle
	{  31, 218,  47, 234, 0x8 }			// pref49GActive
}; 



/***********************************************************************
 *
 * FUNCTION:    TranslatePenToPref(Int16 x, Int16 y)
 *
 * DESCRIPTION: This routine translates a penDown/penUp event into
 *              the press release of a preference button
 *
 * PARAMETERS:  
 *
 * RETURNED:    
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

static UInt16 TranslatePenToPref(Int16 x, Int16 y)
{
	UInt16 i;
	Int16 tmp;
	
	if (statusPortraitViaLandscape)
	{
		tmp=x;
		x=(screenHeight-1)-y;
		y=tmp;
	}
	
	for (i=0;i<maxPrefButtons;i++)
	{
		if ((x >= (prefLayout[i].upperLeftX+prefDialogOriginX)) &&
		    (x <= (prefLayout[i].lowerRightX+prefDialogOriginX)) &&
		    (y >= (prefLayout[i].upperLeftY+prefDialogOriginY)) &&
		    (y <= (prefLayout[i].lowerRightY+prefDialogOriginY)) &&
		    (statusPrefTabActive&prefLayout[i].scope))
		{
			return i;
		}
	}
	return noPref;
}




static void UpdatePixelColorForARMCore(void)
{
	UInt16 			pColor,bColor;

	pColor=(((UInt16)statusPixelColor->r<<8)&0xf800)|(((UInt16)statusPixelColor->g<<3)&0x07e0)|(statusPixelColor->b>>3);
	bColor=(((UInt16)statusBackColor->r<<8)&0xf800)|(((UInt16)statusBackColor->g<<3)&0x07e0)|(statusBackColor->b>>3);

	Q->colorPixelVal=ByteSwap32(((UInt32)pColor<<16)|pColor);
	Q->colorBackVal=ByteSwap32(((UInt32)bColor<<16)|bColor);
}




static Boolean InsertOrRemove128KCard(void)
{
	void    *mem;
	UInt32  memSize;
	UInt32  ramSize;
	UInt16 	ftrNum;
	UInt32	fileSize;
	Err		error;
	UInt32	count;
	UInt8	*buf;
	UInt8	*a,*b;
	FileRef	myRef;
	UInt32	numRead;
	UInt32  portChecksum=0;
	
	
	Q->mcRAMBase	= ByteSwap32(Q->mcRAMBase);
	Q->mcRAMDelta	= ByteSwap32(Q->mcRAMDelta);
	Q->mcCE2Base	= ByteSwap32(Q->mcCE2Base);
	Q->mcCE2Delta	= ByteSwap32(Q->mcCE2Delta);
	Q->mcCE1Base	= ByteSwap32(Q->mcCE1Base);
	Q->mcCE1Delta	= ByteSwap32(Q->mcCE1Delta);
	Q->ce1addr		= (UInt8*)ByteSwap32(Q->ce1addr);
	Q->ce2addr		= (UInt8*)ByteSwap32(Q->ce2addr);
	Q->ram			= (UInt8*)ByteSwap32(Q->ram);
	Q->port1		= (UInt8*)ByteSwap32(Q->port1);
	
	Q->CARDSTATUS=0;
	
	if (optionTarget==HP48SX)
	{
		ftrNum = ramMemSXFtrNum;
		memSize = RAM_SIZE_SX+16; // The extra 16 bytes is just a precautionary cushion
		ramSize = RAM_SIZE_SX;
	}
	else if (optionTarget==HP48GX)
	{
		ftrNum = ramMemGXFtrNum;
		memSize = RAM_SIZE_GX+16; // The extra 16 bytes is just a precautionary cushion
		ramSize = RAM_SIZE_GX;
	}

	if (status128KExpansion)
	{
		// Insert 128K card in port 1
		memSize += 0x40000;
		if (statusUseSystemHeap!=ALL_IN_FTRMEM)
		{
			if (MemPtrNewLarge(memSize, &mem))
			{
				return false;
			}
			else
			{
				MemMove(mem,ramBase,ramSize);  // copy old RAM contents
				MemPtrFree(ramBase);  // free old ram allocation
			}	
		}
		else
		{
			if (FtrPtrResize(appFileCreator, ftrNum, memSize, &mem))
				return false;
		}
		ramBase=mem;
		Q->ram=mem;
		if (optionTarget==HP48SX)
			Q->port1=Q->ram+0x10000;
		else if (optionTarget==HP48GX)
			Q->port1=Q->ram+0x40000;
			
		// load port1 image file if available
		fileSize=0x20000;  // half the size in nibbles
		
		if (optionTarget==HP48SX)
			error = VFSFileOpen(msVolRefNum,sxVFSPORT1FileName,vfsModeRead,&myRef);
		else if (optionTarget==HP48GX)
			error = VFSFileOpen(msVolRefNum,gxVFSPORT1FileName,vfsModeRead,&myRef);
				
		if (error) 
		{
			// no image file available, clear mem
			OpenAlertConfirmDialog(alert,unableToFindPort1FileString,false);
			MemSemaphoreReserve(true);
			MemSet(Q->port1,0x40000,0);
			MemSemaphoreRelease(true);
		}
		else
		{
			// image file available, load it
			buf=(UInt8*)Q->port1;
			
			MemSemaphoreReserve(true);
			
			VFSFileRead(myRef,4,&magicNumberPORT,NULL);
			VFSFileRead(myRef,4,&portChecksum,NULL);
			
			if (statusUseSystemHeap==ALL_IN_FTRMEM)
				error=VFSFileReadData(myRef,fileSize,Q->ram,ramSize+fileSize,&numRead);
			else
				error=VFSFileRead(myRef,fileSize,(void*)((UInt32)Q->ram+ramSize+fileSize),&numRead);

			VFSFileClose(myRef);
			
			a=&buf[0];
			b=&buf[fileSize];
			count=fileSize;
			
			do
			{
				*a++=*b&0xf;
				*a++=*b++>>4;
			} while(--count);

			MemSemaphoreRelease(true);
		}
		if (optionTarget==HP48SX)
			Q->CARDSTATUS=0x5;
		else if (optionTarget==HP48GX)
			Q->CARDSTATUS=0xa;
	}
	else
	{
		newMagicNumber = magicNumberState;
		// Save port1 image
		SaveRAM(true);
		
		// Remove 128K card from port 1
		if (statusUseSystemHeap!=ALL_IN_FTRMEM)
		{
			if (MemPtrNewLarge(memSize, &mem))
			{
				return false;
			}
			else
			{
				MemMove(mem,ramBase,ramSize);  // copy old RAM contents
				MemPtrFree(ramBase);  // free old ram allocation
			}		
		}
		else
		{
			if (FtrPtrResize(appFileCreator, ftrNum, memSize, &mem))
				return false;
		}
		ramBase=mem;
		Q->ram=mem;
		Q->port1=NULL;
	}
	
	// adjust ram and port1 pointer references
	if (optionTarget==HP48SX)
		Q->ce1addr=Q->port1;
	else if (optionTarget==HP48GX)
		Q->ce2addr=Q->port1;
	
	if (Q->mcRAMAddrConfigured) Q->mcRAMDelta = (long)Q->ram-Q->mcRAMBase;
	if (Q->mcCE1AddrConfigured) Q->mcCE1Delta = (long)Q->ce1addr-Q->mcCE1Base;
	if (Q->mcCE2AddrConfigured) Q->mcCE2Delta = (long)Q->ce2addr-Q->mcCE2Base;
	
	Q->statusRecalc=true;

	Q->mcRAMBase	= ByteSwap32(Q->mcRAMBase);
	Q->mcRAMDelta	= ByteSwap32(Q->mcRAMDelta);
	Q->mcCE2Base	= ByteSwap32(Q->mcCE2Base);
	Q->mcCE2Delta	= ByteSwap32(Q->mcCE2Delta);
	Q->mcCE1Base	= ByteSwap32(Q->mcCE1Base);
	Q->mcCE1Delta	= ByteSwap32(Q->mcCE1Delta);
	Q->ce1addr		= (UInt8*)ByteSwap32(Q->ce1addr);
	Q->ce2addr		= (UInt8*)ByteSwap32(Q->ce2addr);
	Q->ram			= (UInt8*)ByteSwap32(Q->ram);
	Q->port1		= (UInt8*)ByteSwap32(Q->port1);
		
	return true;
}


static void UpdatePrefTab(void)
{
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;
	Err				error;
	DmResID			rID,mID;
	WinHandle		ptBackStore;
	Coord 			picWidth,picHeight;
	RectangleType	myRect;
	Int16			len;
	char			tmp[40];
	
	
	switch (statusPrefTabActive)
	{
		case 1: rID = PrefGeneralDialogBitmapFamily;displayMini=false;break;
		case 2: rID = Pref48GXDialogBitmapFamily;mID = mini48GXBitmapFamily;displayMini=true;break;
		case 4: rID = Pref48SXDialogBitmapFamily;mID = mini48SXBitmapFamily;displayMini=true;break;
		case 8: rID = Pref49GDialogBitmapFamily; mID = mini49GBitmapFamily; displayMini=true;break;
		default:
			statusPrefTabActive=1;	rID = PrefGeneralDialogBitmapFamily;break;
	}
	picHandle = DmGetResource(bitmapRsc, rID);
	picBitmap = MemHandleLock(picHandle);
	BmpGetDimensions(picBitmap,&picWidth,&picHeight,NULL);
	
	if (statusPortraitViaLandscape)
		ptBackStore = WinCreateOffscreenWindow(picHeight,picWidth,nativeFormat,&error);
	else
		ptBackStore = WinCreateOffscreenWindow(picWidth,picHeight,nativeFormat,&error);
		
	if (error) 
	{
		MemHandleUnlock(picHandle);
		DmReleaseResource(picHandle);
		return;
	}
	
	
	WinSetDrawWindow(ptBackStore);
	FastDrawBitmap(picBitmap,0,0);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
	
	
	picHandle = DmGetResource(bitmapRsc, zekeBitmapFamily);
	picBitmap = MemHandleLock(picHandle);
	FastDrawBitmap(picBitmap,(prefDialogExtentX>>1)-15,prefDialogExtentY-46);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);

	StrCopy(tmp,"Ver: ");
	StrCat(tmp,appVersionNum);
	FntSetFont(stdFont);
	WinSetTextColorRGB(&blackRGB,NULL);
	WinSetBackColorRGB(&greyRGB,NULL);
	len = FntCharsWidth(tmp, StrLen(tmp));
	DrawCharsExpanded(tmp, StrLen(tmp), (prefDialogExtentX>>1)-(len>>2), prefDialogExtentY-15, 77, winPaint, SINGLE_DENSITY);
	
	if (displayMini)
	{
		picHandle = DmGetResource(bitmapRsc, mID);
		picBitmap = MemHandleLock(picHandle);
		if (!skipAnimation)
		{
			WinSetDrawWindow(miniToWin);
			FastDrawBitmap(picBitmap,0,0);
		}
		else
		{
			WinSetDrawWindow(ptBackStore);
			FastDrawBitmap(picBitmap,miniOffsetX,miniOffsetY);
		}
		MemHandleUnlock(picHandle);
		DmReleaseResource(picHandle);
	}		
	
	
	if (((statusPrefTabActive==2) && (!status48GXROMLocation)) ||
		((statusPrefTabActive==4) && (!status48SXROMLocation)) ||
		((statusPrefTabActive==8) && (!status49GROMLocation)))
	{
		myRect.topLeft.x = 8;
		myRect.topLeft.y = 210; 
		myRect.extent.x = 100;
		myRect.extent.y = 25;
		GreyOutRectangle(myRect,ptBackStore);
		
		picHandle = DmGetResource(bitmapRsc, noImageBitmapFamily);
		picBitmap = MemHandleLock(picHandle);
		WinSetDrawWindow(ptBackStore);
		FastDrawBitmap(picBitmap,noImageOffsetX,noImageOffsetY);
		MemHandleUnlock(picHandle);
		DmReleaseResource(picHandle);
		displayMini=false;
	}
			
	if (statusPrefTabActive==1)
	{
		// grey out Color and Reset buttons if not running active target
		if (!statusRUN)
		{
			myRect.topLeft.x = 147;
			myRect.topLeft.y = 160;
			myRect.extent.x = 126;
			myRect.extent.y = 75;
			GreyOutRectangle(myRect,ptBackStore);		
		}
	}
	WinSetDrawWindow(mainDisplayWindow);	
		
	if (statusPortraitViaLandscape)
		CrossFade(ptBackStore,mainDisplayWindow,prefDialogOriginY,screenHeight-prefDialogOriginX-picWidth,true,128);
	else
		CrossFade(ptBackStore,mainDisplayWindow,prefDialogOriginX,prefDialogOriginY,true,128);
	
	WinDeleteWindow(ptBackStore,false); 
	
	if (!skipAnimation)
	{
		myRect.topLeft.x = miniOffsetX+prefDialogOriginX;
		myRect.topLeft.y = miniOffsetY+prefDialogOriginY;
		myRect.extent.y = miniHeight; 
		myRect.extent.x = miniWidth;
		P48WinCopyRectangle(mainDisplayWindow,miniFromWin,&myRect,0,0,winPaint);
	}
}



static void UpdateGreyArea(void)
{
	RectangleType	myRect;
	Err	error;
	
	if (statusPrefTabActive==1) 
	{
		if (!tempVerbose)
		{
			myRect.topLeft.x = 165+prefDialogOriginX;
			myRect.topLeft.y = 112+prefDialogOriginY;
			myRect.extent.x = 96;
			myRect.extent.y = 33;
			if (statusPortraitViaLandscape) swapRectPVL(&myRect,screenHeight);
			if (greyBackStore==NULL)
				greyBackStore = WinCreateOffscreenWindow(myRect.extent.x,myRect.extent.y,nativeFormat,&error);
			if (greyBackStore!=NULL)
				WinCopyRectangle(mainDisplayWindow,greyBackStore,&myRect,0,0,winPaint);
			myRect.topLeft.x = 165+prefDialogOriginX;
			myRect.topLeft.y = 112+prefDialogOriginY;
			myRect.extent.x = 96;
			myRect.extent.y = 33;
			GreyOutRectangle(myRect,mainDisplayWindow);
		}
		else
		{
			if (greyBackStore!=NULL)
			{
				myRect.topLeft.x = 0;
				myRect.topLeft.y = 0;
				myRect.extent.x = 96;
				myRect.extent.y = 33;
				P48WinCopyRectangle(greyBackStore,mainDisplayWindow,&myRect,165+prefDialogOriginX,112+prefDialogOriginY,winPaint);
			}
		}
	}		
}



static void UpdateHeapSpace(void)
{
	UInt16 numHeaps0;
	UInt32 heapFree,heapMax;
	Int16  length,position;
	char   tmp[20],tmp2[20];
	Int16  i,j,p;
	Int16  len;
	
	
	if (statusPrefTabActive!=1) return;
	
	numHeaps0 = MemNumHeaps(0);
	for (i=0;i<numHeaps0;i++)
	{
		if (MemHeapDynamic(i))
		{
			MemHeapFreeBytes(i, &heapFree, &heapMax);
			FntSetFont(stdFont);
			StrPrintF(tmp,"%lu",heapFree);
			len=StrLen(tmp);
			p=0;			
			for (j=0;j<len;j++)
			{
				tmp2[p++]=tmp[j];
				if ((((len-j-1)%3)==0) && (j<(len-1))) tmp2[p++]=',';
			}
			tmp2[p]=0;
			
			StrCat(tmp2," B");
			
			length = (FntCharsWidth(tmp2, StrLen(tmp2))>>1);
			position=((79-length)/2)+1;
			if ((position<0)||(position>79))
				position=0;
			WinSetTextColorRGB(&blackRGB,NULL);
			WinSetBackColorRGB(&greyRGB,NULL);
			DrawCharsExpanded(tmp2, StrLen(tmp2), position+28+prefDialogOriginX, prefDialogOriginY+229, 77, winPaint, SINGLE_DENSITY);
		}
	}
}



static void computeBoxCoordinates(UInt16 prefCode, Coord *x, Coord *y)
{
	if (statusPortraitViaLandscape)
	{
		*x=prefLayout[prefCode].upperLeftY+prefDialogOriginY;
		*y=screenHeight-(prefLayout[prefCode].upperLeftX+prefDialogOriginX)-17;
	}
	else
	{
		*x=prefLayout[prefCode].upperLeftX+prefDialogOriginX;
		*y=prefLayout[prefCode].upperLeftY+prefDialogOriginY;
	}
}


static void UpdateCheckBoxes(void)
{
	RectangleType trueRect;
	RectangleType falseRect;

	Coord x,y;
	
	trueRect.topLeft.x = 0;
	trueRect.topLeft.y = 0;
	trueRect.extent.x = 17;
	trueRect.extent.y = 17;
	falseRect.topLeft.x = 17;
	falseRect.topLeft.y = 0;
	falseRect.extent.x = 17;
	falseRect.extent.y = 17;
	
	switch (statusPrefTabActive)
	{
		case 1:
			computeBoxCoordinates(prefGenVerbose,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((tempVerbose)?&trueRect:&falseRect),x,y,winPaint);

			computeBoxCoordinates(prefGenTrueSpeed,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((tempGenTrueSpeed)?&trueRect:&falseRect),x,y,winPaint);

			computeBoxCoordinates(prefGenDisplayMonitor,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((tempGenDisplayMonitor)?&trueRect:&falseRect),x,y,winPaint);

			computeBoxCoordinates(prefGenKeyClick,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((tempGenKeyClick)?&trueRect:&falseRect),x,y,winPaint);

			if (tempVerbose)
			{
				computeBoxCoordinates(prefGenVerboseSlow,&x,&y);
				WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((!tempScrollFast)?&trueRect:&falseRect),x,y,winPaint);

				computeBoxCoordinates(prefGenVerboseFast,&x,&y);
				WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((tempScrollFast)?&trueRect:&falseRect),x,y,winPaint);
			}
			break;
		case 2: 
			computeBoxCoordinates(pref48GXBackdrop,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((temp48GXBackdrop)?&trueRect:&falseRect),x,y,winPaint);

			computeBoxCoordinates(pref48GXColorCycle,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((temp48GXColorCycle)?&trueRect:&falseRect),x,y,winPaint);

			computeBoxCoordinates(pref48GX128KExpansion,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((temp48GX128KExpansion)?&trueRect:&falseRect),x,y,winPaint);

			if (status48GXROMLocation)
			{
				computeBoxCoordinates(pref48GXActive,&x,&y);
				WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((tempTarget==HP48GX)?&trueRect:&falseRect),x,y,winPaint);
			}
			break;
		case 4: 
			computeBoxCoordinates(pref48SXBackdrop,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((temp48SXBackdrop)?&trueRect:&falseRect),x,y,winPaint);

			computeBoxCoordinates(pref48SXColorCycle,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((temp48SXColorCycle)?&trueRect:&falseRect),x,y,winPaint);

			computeBoxCoordinates(pref48SX128KExpansion,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((temp48SX128KExpansion)?&trueRect:&falseRect),x,y,winPaint);

			if (status48SXROMLocation)
			{
				computeBoxCoordinates(pref48SXActive,&x,&y);
				WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((tempTarget==HP48SX)?&trueRect:&falseRect),x,y,winPaint);
			}
			break;
		case 8:
			computeBoxCoordinates(pref49GBackdrop,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((temp49GBackdrop)?&trueRect:&falseRect),x,y,winPaint);

			computeBoxCoordinates(pref49GColorCycle,&x,&y);
			WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((temp49GColorCycle)?&trueRect:&falseRect),x,y,winPaint);

			if (status49GROMLocation)
			{
				computeBoxCoordinates(pref49GActive,&x,&y);
				WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((tempTarget==HP49G)?&trueRect:&falseRect),x,y,winPaint);
			}
			break;
		default:
			break;
	}
}




static void DepressPreferenceButton(UInt16 prefCode)
{
	RectangleType 	myRect;
	RGBColorType  	prevRGB;
	Int16			count=0;
	
	WinSetForeColorRGB(&redRGB,&prevRGB);

	switch (prefCode)
	{
		case prefGenVerbose:
		case prefGenVerboseSlow:
		case prefGenVerboseFast:
		case prefGenTrueSpeed:
		case prefGenDisplayMonitor:
		case prefGenKeyClick:
		case pref48GXBackdrop:
		case pref48GXColorCycle:
		case pref48GX128KExpansion:
		case pref48GXActive:
		case pref48SXBackdrop:
		case pref48SXColorCycle:
		case pref48SX128KExpansion:
		case pref48SXActive:
		case pref49GBackdrop:
		case pref49GColorCycle:
		case pref49GActive:
			WinSetBackColorRGB(&blackRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = prefLayout[prefCode].upperLeftX+prefDialogOriginX;
			myRect.topLeft.y = prefLayout[prefCode].upperLeftY+prefDialogOriginY;
			myRect.extent.x = prefLayout[prefCode].lowerRightX-prefLayout[prefCode].upperLeftX+1;
			myRect.extent.y = prefLayout[prefCode].lowerRightY-prefLayout[prefCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,0);
			WinSetDrawMode(winPaint);
			WinSetBackColorRGB(&backRGB,NULL);
			break;
		case prefGeneral:
		case pref48GX:
		case pref48SX:
		case pref49G:
		case prefGenReset:
		case prefGenColorSelect:
		case prefCancel:
		case prefOK:
			WinSetBackColorRGB(&greyRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = prefLayout[prefCode].upperLeftX+prefDialogOriginX;
			myRect.topLeft.y = prefLayout[prefCode].upperLeftY+prefDialogOriginY;
			myRect.extent.x = prefLayout[prefCode].lowerRightX-prefLayout[prefCode].upperLeftX+1;
			myRect.extent.y = prefLayout[prefCode].lowerRightY-prefLayout[prefCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,10);
			WinSetDrawMode(winPaint);
			WinSetBackColorRGB(&backRGB,NULL);
			break;
		default:break;
	}
	
	WinSetForeColorRGB(&prevRGB,NULL);
}


static void ReleasePreferenceButton(UInt16 prefCode)
{
	RectangleType myRect;
	RGBColorType  prevRGB;
	
	WinSetForeColorRGB(&redRGB,&prevRGB);

	switch (prefCode)
	{
		case prefGenVerbose:
		case prefGenVerboseSlow:
		case prefGenVerboseFast:
		case prefGenTrueSpeed:
		case prefGenDisplayMonitor:
		case prefGenKeyClick:
		case pref48GXBackdrop:
		case pref48GXColorCycle:
		case pref48GX128KExpansion:
		case pref48GXActive:
		case pref48SXBackdrop:
		case pref48SXColorCycle:
		case pref48SX128KExpansion:
		case pref48SXActive:
		case pref49GBackdrop:
		case pref49GColorCycle:
		case pref49GActive:
			WinSetBackColorRGB(&blackRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = prefLayout[prefCode].upperLeftX+prefDialogOriginX;
			myRect.topLeft.y = prefLayout[prefCode].upperLeftY+prefDialogOriginY;
			myRect.extent.x = prefLayout[prefCode].lowerRightX-prefLayout[prefCode].upperLeftX+1;
			myRect.extent.y = prefLayout[prefCode].lowerRightY-prefLayout[prefCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,0);
			WinSetDrawMode(winPaint);
			break;
		case prefGeneral:
		case pref48GX:
		case pref48SX:
		case pref49G:
		case prefGenReset:
		case prefGenColorSelect:
		case prefCancel:
		case prefOK:
			WinSetBackColorRGB(&greyRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = prefLayout[prefCode].upperLeftX+prefDialogOriginX;
			myRect.topLeft.y = prefLayout[prefCode].upperLeftY+prefDialogOriginY;
			myRect.extent.x = prefLayout[prefCode].lowerRightX-prefLayout[prefCode].upperLeftX+1;
			myRect.extent.y = prefLayout[prefCode].lowerRightY-prefLayout[prefCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,10);
			WinSetDrawMode(winPaint);
			break;
		default:break;
	}
	
	WinSetForeColorRGB(&prevRGB,NULL);
	WinSetBackColorRGB(&backRGB,NULL);
}



/***********************************************************************
 *
 * FUNCTION:    DisplayPreferenceScreen
 *
 * DESCRIPTION: 
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
void DisplayPreferenceScreen(void)
{
	RectangleType 	copyRect;
	int				i;
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;
	Coord 			picWidth,picHeight;
	Int16			penX,penY;
	Boolean			penDown=false;
	int				applyChangesOrCancel=0;
	EventType		event,newEvent;
	UInt16			prefCode;
	UInt16			prefReleasePending=noPref;
	Err				error;
	Boolean			revertScreen=false;
	Boolean			delayClear=false;
	drawInfo		*dInfo=NULL;
	UInt16			rowB;
	MemHandle 		armH;
	MemPtr    		armP;
	UInt32			position;
	UInt32			processorType;
	UInt32			timeout;
	
	tempTarget					= optionTarget;
	tempVerbose					= optionVerbose;
	tempScrollFast				= optionScrollFast;
	tempThreeTwentySquared 		= optionThreeTwentySquared;
	temp48GXBackdrop			= option48GXBackdrop;
	temp48SXBackdrop			= option48SXBackdrop;
	temp49GBackdrop				= option49GBackdrop;
	temp48GXColorCycle			= option48GXColorCycle;
	temp48SXColorCycle			= option48SXColorCycle;
	temp49GColorCycle			= option49GColorCycle;
	temp48GX128KExpansion		= option48GX128KExpansion;
	temp48SX128KExpansion		= option48SX128KExpansion;
	tempGenTrueSpeed			= optionTrueSpeed;
	tempGenDisplayMonitor		= optionDisplayMonitor;
	tempGenKeyClick				= optionKeyClick;
	
	if ((optionThreeTwentySquared)&&(statusDisplayMode2x2))
	{
		toggle_lcd(); // put calc screen in 1x1 mode
		revertScreen=true;
	}
	
	HidePanel();
	
	greyBackStore=NULL;
	
	copyRect.topLeft.x = 0;
	copyRect.topLeft.y = 0;
	copyRect.extent.x = 320;
	copyRect.extent.y = ((optionThreeTwentySquared)?320:450);
	P48WinCopyRectangle(mainDisplayWindow,backStoreWindow,&copyRect,0,0,winPaint);

	// Get dialog check box graphic
	picHandle = DmGetResource(bitmapRsc, PrefCheckboxGraphicBitmapFamily);
	picBitmap = MemHandleLock(picHandle);
	BmpGetDimensions(picBitmap,&picWidth,&picHeight,NULL);
	checkBoxWindow = WinCreateOffscreenWindow(picWidth,picHeight,nativeFormat,&error);
	if (!error) 
	{
		WinSetDrawWindow(checkBoxWindow);
		WinDrawBitmap(picBitmap,0,0);
		WinSetDrawWindow(mainDisplayWindow);
	}
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
	if (error)
	{
		OpenAlertConfirmDialog(alert,errorDisplayingPrefScreenString,false);
		goto dpsexit;
	}
	
	miniToWin=NULL;
	miniFromWin=NULL;
	skipAnimation=false;
	
	// set up background windows for mini display
	
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
	if (processorType == sysFtrNumProcessorx86) skipAnimation=true;

	if (!skipAnimation) 
	{
		dInfo = (drawInfo*) MemPtrNew(sizeof(drawInfo));
		if (statusPortraitViaLandscape)
		{
			miniToWin = WinCreateOffscreenWindow(miniHeight,miniWidth,nativeFormat,&error);
			if (error) skipAnimation=true;
			miniFromWin = WinCreateOffscreenWindow(miniHeight,miniWidth,nativeFormat,&error);
			if (error) skipAnimation=true;
			dInfo->toWidth = miniHeight;
			dInfo->toHeight = miniWidth;
			dInfo->offsetX = prefDialogOriginY + miniOffsetY;
			dInfo->offsetY = screenHeight-(prefDialogOriginX + miniOffsetX)-miniWidth;
		}
		else
		{
			miniToWin = WinCreateOffscreenWindow(miniWidth,miniHeight,nativeFormat,&error);
			if (error) skipAnimation=true;
			miniFromWin = WinCreateOffscreenWindow(miniWidth,miniHeight,nativeFormat,&error);
			if (error) skipAnimation=true;
			dInfo->toWidth = miniWidth;
			dInfo->toHeight = miniHeight;
			dInfo->offsetX = prefDialogOriginX + miniOffsetX;
			dInfo->offsetY = prefDialogOriginY + miniOffsetY;
		}
	}
	

	if (!skipAnimation)
	{
		dInfo->fromBasePtr=BmpGetBits(WinGetBitmap(miniFromWin));
		dInfo->toBasePtr=BmpGetBits(WinGetBitmap(miniToWin));
		dInfo->destBasePtr=BmpGetBits(WinGetBitmap(mainDisplayWindow));
		BmpGetDimensions(WinGetBitmap(mainDisplayWindow),NULL,NULL,&rowB);
		dInfo->destRowInt16s = rowB>>1;
		
		dInfo->effectType 	= TYPE_ONE;
		dInfo->effectType 	= ByteSwap32(dInfo->effectType);
		dInfo->fromBasePtr 	= (UInt16*)ByteSwap32(dInfo->fromBasePtr);
		dInfo->toBasePtr 	= (UInt16*)ByteSwap32(dInfo->toBasePtr);
		dInfo->destBasePtr 	= (UInt16*)ByteSwap32(dInfo->destBasePtr);
		dInfo->destRowInt16s = ByteSwap32(dInfo->destRowInt16s);
		dInfo->offsetX 		= ByteSwap32(dInfo->offsetX);
		dInfo->offsetY 		= ByteSwap32(dInfo->offsetY);
		dInfo->toWidth 		= ByteSwap32(dInfo->toWidth);
		dInfo->toHeight 	= ByteSwap32(dInfo->toHeight);

		position=0;

		armH = DmGetResource('ARMC', 0x3090);
		armP = MemHandleLock(armH);
	}

redrawPrefs:	
	UpdatePrefTab();	
	UpdateCheckBoxes();
	UpdateHeapSpace();
	UpdateGreyArea();
	
	EvtFlushPenQueue();
	do
	{
		timeout = evtWaitForever;
		
		if (!skipAnimation)
		{
			dInfo->position=position;
			position++;
			if (position>=90) position=0;
			if (displayMini)  PceNativeCall(armP, dInfo);
			timeout = 1;
		}
		
		WinSetCoordinateSystem(kCoordinatesStandard);
		EvtGetEvent(&event,timeout);
		WinSetCoordinateSystem(kCoordinatesNative);
		if (event.eType!=nilEvent)
		{
			//process event
			if (event.eType==penDownEvent)
			{
				EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
				prefCode = TranslatePenToPref(penX,penY);
				if (prefCode != noPref)
				{
					DepressPreferenceButton(prefCode);
					prefReleasePending = prefCode;
					continue;
				}
				else
				{
					prefReleasePending = noPref;
				}
			}
			if (event.eType==penUpEvent)
			{
				if (prefReleasePending != noPref)
				{
					EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
					prefCode = TranslatePenToPref(penX,penY);
					ReleasePreferenceButton(prefReleasePending);
					
					if (prefReleasePending == prefCode)
					{
						switch(prefReleasePending)
						{
							case prefGeneral:
								statusPrefTabActive=1;
								UpdatePrefTab();
								UpdateCheckBoxes();
								UpdateHeapSpace();
								UpdateGreyArea();
								break;
							case pref48GX:
								statusPrefTabActive=2;
								UpdatePrefTab();
								UpdateCheckBoxes();
								break;
							case pref48SX:
								statusPrefTabActive=4;
								UpdatePrefTab();
								UpdateCheckBoxes();
								break;
							case pref49G:
								statusPrefTabActive=8;
								UpdatePrefTab();
								UpdateCheckBoxes();
								break;
							case prefGenTrueSpeed:
								if (tempGenTrueSpeed)
									tempGenTrueSpeed=false;
								else
									tempGenTrueSpeed=true;
								UpdateCheckBoxes();
								break;
							case prefGenDisplayMonitor:
								if (tempGenDisplayMonitor)
									tempGenDisplayMonitor=false;
								else
									tempGenDisplayMonitor=true;
								UpdateCheckBoxes();
								break;
							case prefGenKeyClick:
								if (tempGenKeyClick)
									tempGenKeyClick=false;
								else
									tempGenKeyClick=true;
								UpdateCheckBoxes();
								break;
							case prefGenVerbose:
								if (tempVerbose)
									tempVerbose=false;
								else
									tempVerbose=true;
								UpdateGreyArea();
								UpdateCheckBoxes();
								break;
							case prefGenVerboseSlow:
								if (tempVerbose)
								{
									if (tempScrollFast)
										tempScrollFast=false;
									UpdateCheckBoxes();
								}
								break;								
							case prefGenVerboseFast:
								if (tempVerbose)
								{
									if (!tempScrollFast)
										tempScrollFast=true;
									UpdateCheckBoxes();
								}
								break;								
							case prefGenReset:
								if (statusRUN)
								{
									if (OpenAlertConfirmDialog(confirm,checkResetString,false))
									{
										PinResetEmulator();
										applyChangesOrCancel=1;  // Discard any changes
									}
								}
								break;
							case prefGenColorSelect:
								if (statusRUN)
								{
									// bring up the color selector after dialog closes
									newEvent.eType = invokeColorSelectorEvent;
									applyChangesOrCancel=1;  // Discard any changes
									EvtAddEventToQueue(&newEvent);
								}
								break;
								
							case pref48GXBackdrop:
								if (temp48GXBackdrop)
									temp48GXBackdrop=false;
								else
									temp48GXBackdrop=true;
								UpdateCheckBoxes();
								break;
							case pref48GXColorCycle:
								if (temp48GXColorCycle)
									temp48GXColorCycle=false;
								else
									temp48GXColorCycle=true;
								UpdateCheckBoxes();
								break;
							case pref48GX128KExpansion:
								if (temp48GX128KExpansion)
									temp48GX128KExpansion=false;
								else
									temp48GX128KExpansion=true;
								UpdateCheckBoxes();
								break;
							case pref48GXActive:
								if (status48GXROMLocation)
								{
									tempTarget=HP48GX;
									UpdateCheckBoxes();
								}
								break;
								
							case pref48SXBackdrop:
								if (temp48SXBackdrop)
									temp48SXBackdrop=false;
								else
									temp48SXBackdrop=true;
								UpdateCheckBoxes();
								break;
							case pref48SXColorCycle:
								if (temp48SXColorCycle)
									temp48SXColorCycle=false;
								else
									temp48SXColorCycle=true;
								UpdateCheckBoxes();
								break;
							case pref48SX128KExpansion:
								if (temp48SX128KExpansion)
									temp48SX128KExpansion=false;
								else
									temp48SX128KExpansion=true;
								UpdateCheckBoxes();
								break;
							case pref48SXActive:
								if (status48SXROMLocation)
								{
									tempTarget=HP48SX;
									UpdateCheckBoxes();
								}
								break;
								
							case pref49GBackdrop:
								if (temp49GBackdrop)
									temp49GBackdrop=false;
								else
									temp49GBackdrop=true;
								UpdateCheckBoxes();
								break;
							case pref49GColorCycle:
								if (temp49GColorCycle)
									temp49GColorCycle=false;
								else
									temp49GColorCycle=true;
								UpdateCheckBoxes();
								break;
							case pref49GActive:
								if (status49GROMLocation)
								{
									tempTarget=HP49G;
									UpdateCheckBoxes();
								}
								break;
								
							case prefCancel:
								applyChangesOrCancel=1;
								break;
							case prefOK:
								applyChangesOrCancel=2;
								break;
							default: break;
						}
						continue;
					}
				}
			}
			
			if (event.eType==appStopEvent)  // respond to app-switching events.
			{
				DepressPreferenceButton(prefCancel);
				SysTaskDelay(10);
				ReleasePreferenceButton(prefCancel);
				Q->statusQuit=true;
				applyChangesOrCancel=1;
				EvtAddEventToQueue(&event);  // put app stop back on queue
				continue;
			}

			if (event.eType==keyDownEvent)
			{
				if (event.data.keyDown.chr == chrLineFeed)
				{
					DepressPreferenceButton(prefOK);
					SysTaskDelay(10);
					ReleasePreferenceButton(prefOK);
					applyChangesOrCancel=2;
					continue;
				}
				if (event.data.keyDown.chr == vchrSliderOpen)
				{
					PINSetInputAreaState(pinInputAreaClosed);
					continue;
				}
				if (event.data.keyDown.chr == vchrSliderClose)
				{
					continue;
				}				
			}
			
			if (event.eType==volumeUnmountedEvent)
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
					}
					continue;
				}
			}
			
			if (event.eType==volumeMountedEvent)
			{
				if (!statusInternalFiles)
				{
					CheckForRemount();
					continue;
				}
			}
			
			if (event.eType==winDisplayChangedEvent)
			{
				if (CheckDisplayTypeForGUIRedraw()) GUIRedraw();
				goto redrawPrefs;
			}
			
    		WinSetCoordinateSystem(kCoordinatesStandard);
			if (! SysHandleEvent(&event))  // respond to all system events, like power-down, etc...
			{
				FrmDispatchEvent(&event);
			}
			WinSetCoordinateSystem(kCoordinatesNative);
		}
	} while (!applyChangesOrCancel);
	
	
	if (applyChangesOrCancel==2)
	{
		//apply all the changes
		optionVerbose             = tempVerbose;
		optionScrollFast		  = tempScrollFast;
		
		if (tempTarget==HP48SX)
		{
			if (option48SXBackdrop != temp48SXBackdrop)
			{
				if (statusBackdrop != temp48SXBackdrop)
				{
					if (temp48SXBackdrop)
					{
						// backdrop off -> on transition: load in backdrop
						statusBackdrop = true;
						LoadBackgroundImage(tempTarget);  // may set Q->statusBackdrop to false if error
					}
					else
					{
						// backdrop on -> off transition: clear backdrop window memory
						statusBackdrop = false;
						WinDeleteWindow(backdrop1x1,false);
						WinDeleteWindow(backdrop2x2,false);
						if (statusLandscapeAvailable) WinDeleteWindow(backdrop2x2PVL,false);
						backdrop1x1=NULL;
						backdrop2x2=NULL;
						backdrop2x2PVL=NULL;
					}
					delayClear=true;
					for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
				}
			}
			
			if (option48SXColorCycle != temp48SXColorCycle)
			{
				statusColorCycle = temp48SXColorCycle;
				UpdatePixelColorForARMCore();
				for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
			}
			
			if (option48SX128KExpansion != temp48SX128KExpansion)
			{
				if (Q->lcdOn) 
				{
					OpenAlertConfirmDialog(alert,turnOffCalcBeforeCardString,false);
					temp48SX128KExpansion = option48SX128KExpansion;
				}
				else
				{
					status128KExpansion = temp48SX128KExpansion;
					if (statusRUN) InsertOrRemove128KCard();
				}
			}
		}
		else if (tempTarget==HP48GX)
		{
			if (option48GXBackdrop != temp48GXBackdrop)
			{
				if (statusBackdrop != temp48GXBackdrop)
				{
					if (temp48GXBackdrop)
					{
						// backdrop off -> on transition: load in backdrop
						statusBackdrop = true;
						LoadBackgroundImage(tempTarget);  // may set Q->statusBackdrop to false if error
					}
					else
					{
						// backdrop on -> off transition: clear backdrop window memory
						statusBackdrop = false;
						WinDeleteWindow(backdrop1x1,false);
						WinDeleteWindow(backdrop2x2,false);
						if (statusLandscapeAvailable) WinDeleteWindow(backdrop2x2PVL,false);
						backdrop1x1=NULL;
						backdrop2x2=NULL;
						backdrop2x2PVL=NULL;
					}
					delayClear=true;
					for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
				}
			}
			
			if (option48GXColorCycle != temp48GXColorCycle)
			{
				statusColorCycle = temp48GXColorCycle;
				UpdatePixelColorForARMCore();
				for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
			}
			
			if (option48GX128KExpansion != temp48GX128KExpansion)
			{
				if (Q->lcdOn)
				{
					OpenAlertConfirmDialog(alert,turnOffCalcBeforeCardString,false);
					temp48GX128KExpansion = option48GX128KExpansion;
				}
				else
				{
					status128KExpansion = temp48GX128KExpansion;
					if (statusRUN) InsertOrRemove128KCard();
				}
			}
		}
		else
		{
			if (option49GBackdrop != temp49GBackdrop)
			{
				if (statusBackdrop != temp49GBackdrop)
				{
					if (temp49GBackdrop)
					{
						// backdrop off -> on transition: load in backdrop
						statusBackdrop = true;
						LoadBackgroundImage(tempTarget);  // may set Q->statusBackdrop to false if error
					}
					else
					{
						// backdrop on -> off transition: clear backdrop window memory
						statusBackdrop = false;
						WinDeleteWindow(backdrop1x1,false);
						WinDeleteWindow(backdrop2x2,false);
						if (statusLandscapeAvailable) WinDeleteWindow(backdrop2x2PVL,false);
						backdrop1x1=NULL;
						backdrop2x2=NULL;
						backdrop2x2PVL=NULL;
					}
					delayClear=true;
					for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
				}
			}
			
			if (option49GColorCycle != temp49GColorCycle)
			{
				statusColorCycle = temp49GColorCycle;
				UpdatePixelColorForARMCore();
				for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
			}
		}
		
		option48SXBackdrop = temp48SXBackdrop;
		option48SXColorCycle = temp48SXColorCycle;
		option48SX128KExpansion = temp48SX128KExpansion;
		option48GXBackdrop = temp48GXBackdrop;
		option48GXColorCycle = temp48GXColorCycle;
		option48GX128KExpansion = temp48GX128KExpansion;
		option49GBackdrop = temp49GBackdrop;
		option49GColorCycle = temp49GColorCycle;
		
		optionDisplayMonitor = tempGenDisplayMonitor;
		optionKeyClick		 = tempGenKeyClick;
		
		optionTrueSpeed = tempGenTrueSpeed;
		SetCPUSpeed();	
			
		Q->statusColorCycle = statusColorCycle;
		Q->statusBackdrop = statusBackdrop;

		if (optionTarget != tempTarget)
		{
			statusSwitchEmulation = 1;
			Q->statusQuit = true;           // stop the emulator so it can be restarted.
			statusNextTarget=tempTarget;
		}
	}

	if (!skipAnimation)
	{
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
	}

	if (miniFromWin!=NULL) WinDeleteWindow(miniFromWin,false);
	if (miniToWin!=NULL) WinDeleteWindow(miniToWin,false);
	
	if (dInfo!=NULL) MemPtrFree(dInfo);

	WinDeleteWindow(checkBoxWindow,false);

dpsexit:

	if (greyBackStore!=NULL) WinDeleteWindow(greyBackStore,false);
	
	EvtFlushPenQueue();
	
	CrossFade(backStoreWindow,mainDisplayWindow,0,0,false,128);

	if (delayClear) display_clear();
	
	ShowPanel();
	
	if (revertScreen)
		toggle_lcd();		
}




/*************************************************************************\
*
*  Power48.c
*
*  This is the module that provides all palm-related startup and shutdown
*  features, as well as global option and preference processing. Some
*  GUI elements are defined here as well.
*
*  (c)2002 Robert Hildinger
*
\*************************************************************************/

#include <PalmOS.h>
#include "Power48Rsc.h"
#include <SonyCLIE.h>
#include "Power48.h"
#include "Power48_core.h"
#include "PceNativeCall.h" 
#include <PalmResources.h>
#include <PalmSG.h>
#include <TapWave.h>

/***********************************************************************
 *
 *	Global variables
 *
 ***********************************************************************/

UInt16 silkLibRefNum;
Boolean collapsibleIA=false;
UInt32 PINVersion=0;
UInt32 SILKVersion=0;

UInt8 *romBase=NULL;  // pure, unswapped pointer to allocated ROM memory space
UInt8 *ramBase=NULL;  // pure, unswapped pointer to allocated RAM memory space
UInt8 *port1Base=NULL;  // pure, unswapped pointer to allocated PORT1 memory space
UInt8 *port2Base=NULL;  // pure, unswapped pointer to allocated PORT2 memory space

WinHandle mainDisplayWindow;
WinHandle annunciatorBitmapWindow;	
WinHandle backStoreWindow=NULL;	
WinHandle digitsWindow=NULL;	
WinHandle panelWindow=NULL;	
WinHandle backdrop1x1=NULL;
WinHandle backdrop2x2=NULL;
WinHandle backdrop2x2PVL=NULL;
WinHandle checkBoxWindow;	
WinHandle greyBackStore;
WinHandle textWindow;
BitmapPtr textBitmap;
UInt16 *textBitmapPtr;


RGBColorType backRGB = {0,255,255,255};  // default white
RGBColorType textRGB = {0,0,0,0};        // default black

Int16 displayExtX=0;
Int16 displayExtY=0;
UInt32 screenHeight=0;
UInt32 oldScreenDepth=0;
UInt32 screenRowInt16s=0;

static UInt16 screenY=lcdOriginY-13;
static UInt16 skipOnce=1;
static UInt16 errorScreenY=errorPrintOriginY+1;
static UInt16 errorSkipOnce=1;
UInt16 msVolRefNum=0;

keyDef* keyLayout;
squareDef* virtLayout;

UInt32 magicNumberState=0;
UInt32 magicNumberRAM=0;
UInt32 magicNumberPORT=0;
UInt32 newMagicNumber=0;

const UInt8 F_s_init[16] = {0/*P*/,0       ,2,0,15,3 ,0,0 ,0,0,0,0,0,0,0,0};
const UInt8 F_l_init[16] = {1     ,1/*P+1*/,1,3,1 ,12,2,16,0,0,0,0,0,0,0,5};

const RGBColorType redRGB={0,255,0,0};
const RGBColorType greenRGB={0,0,255,0};
const RGBColorType blueRGB={0,0,0,255};
const RGBColorType whiteRGB={0,255,255,255};
const RGBColorType blackRGB={0,0,0,0};
const RGBColorType greyRGB={0,199,199,199};
const RGBColorType panelRGB={0,136,136,136};
const RGBColorType lightBlueRGB={0,17,207,255};
const RGBColorType lightGreenRGB={0,0,255,136};

const char *aboutText = 
	"\0\1"
	"(c) 2002-04 by Robert Hildinger\0\2"
	"\0\1"
	"\0\1"
	"Version: " appVersionNum "\0\2"
	"\0\1"
	"Build Date: " __DATE__ "\0\2"
	"\0\1"
	"\0\1"
	"\0\1"
	"HP48/49 ROM images are the property of\0\1"
	"and copyrighted by Hewlett-Packard.\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"Power48 is based primarily on Emu48 written\0\1"
	"by Sebastien Carlier, though it also borrows\0\1"
	"a few tricks from the later incarnations of\0\1"
	"Emu48 as maintained and updated by Christoph\0\1"
	"Gießelink. The code has been almost completely\0\1"
	"rewritten and optimized for the Palm platform\0\1"
	"in an attempt to squeeze in every ounce of\0\1"
	"performance possible.\0\1"
	"\0\1"
	"\0\1"
	"48GX graphic representation based on work of\0\1"
	"Jeffery L. McMahan\0\1"
	"\0\1"
	"48SX graphic representation based on work of\0\1"
	"Jeff Breadner\0\1"
	"\0\1"
	"49G graphic representation based on work of\0\1"
	"Eric Rechlin\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"For all Power48 support issues...\0\2"
	"email:\0\1"
	"support@power48.mobilevoodoo.com\0\1"
	"website:\0\1"
	"http://power48.mobilevoodoo.com/\0\3"
	"\0\1"
	"\0\1"
	"\0\1"
	"Notes:\0\1"
	"\0\1"
	"I would like to thank everyone who has\0\1"
	"donated to the continued development of\0\1"
	"Power48. Your generosity is very much ap-\0\1"
	"preciated!\0\1"
	"\0\1"
	"Power48 is a free program distributed under\0\1"
	"the GPL, but unfortunately testing hardware\0\1"
	"and development environments are not. Working\0\1"
	"on this program also requires large amounts\0\1"
	"of my free time. If you're feeling generous\0\1"
	"and find this program useful and would like\0\1"
	"to support it's continued development, perhaps\0\1"
	"you'd consider making a donation to help offset\0\1"
	"the costs of developing it? If you feel so\0\1"
	"inclined, you can send your donation through\0\1"
	"PayPal to the following email address:\0\1"
	"support@power48.mobilevoodoo.com.\0\1"
	"\0\1"
	"Thank you very much,\0\4"
	"-Robert Hildinger\0\4"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"GPL statement\0\1"
	"\0\1"
	"This program is free software; you can redis-\0\1"
	"tribute it and/or modify it under the terms of\0\1"
	"the GNU General Public License as published by\0\1"
	"the Free Software Foundation; either version 2\0\1"
	"of the license, or (at your option) any later\0\1"
	"version.\0\1"
	"\0\1"
	"This program is distributed in the hope that it\0\1"
	"will be useful, but WITHOUT ANY WARRANTY;\0\1"
	"without even the implied warranty of \0\1"
	"MERCHANTABILITY or FITNESS FOR A PAR-\0\1"
	"TICULAR PURPOSE. See the GNU General Public\0\1"
	"License for more details.\0\1"
	"\0\1"
	"You should have received a copy of the GNU\0\1"
	"General Public License along with this program;\0\1"
	"if not, write to:\0\1"
	"The Free Software Foundation, Inc.\0\1"
	"59 Temple Place, Suite 330\0\1"
	"Boston, MA  02111-1307\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"\0\1"
	"QQ";


		
/***********************************************************************
 *
 *	Global Option variables
 *
 ***********************************************************************/


calcTarget 	optionTarget				= HP48GX;
Boolean 	optionVerbose				= false;
Boolean 	optionScrollFast			= true;
Boolean 	optionThreeTwentySquared	= false;
Boolean	 	option48GXBackdrop			= false;
Boolean	 	option48SXBackdrop			= false;
Boolean	 	option49GBackdrop			= false;
Boolean	 	option48GXColorCycle		= false;
Boolean	 	option48SXColorCycle		= false;
Boolean	 	option49GColorCycle			= false;
Boolean	 	option48GX128KExpansion		= false;
Boolean	 	option48SX128KExpansion		= false;
RGBColorType	option48GXPixelColor	= {0,0,0,80};
RGBColorType	option48GXBackColor		= {0,120,132,96};
RGBColorType	option48SXPixelColor	= {0,0,0,0};
RGBColorType	option48SXBackColor		= {0,144,152,120};
RGBColorType	option49GPixelColor		= {0,0,0,0};
RGBColorType	option49GBackColor		= {0,144,152,120};
PanelType	optionPanelType				= true;
Int16		optionPanelX				= 320-panelTabWidth;
Int16		optionPanelY				= 0;
Boolean		optionPanelOpen				= false;
Boolean		optionTrueSpeed				= false;
Boolean		optionDisplayZoomed			= false;
Boolean		optionDisplayMonitor		= false;
Boolean		optionKeyClick				= false;


Boolean 	previousOptionThreeTwentySquared = false;
UInt16	 	previousStatusOrientation		 = 99;


Boolean 	statusCompleteReset			= false;
Boolean 	statusSwitchEmulation 		= false;
Boolean 	status128KExpansion         = false;
Boolean		statusBackdrop				= false;
Boolean 	statusColorCycle			= false;
Boolean 	statusPortraitViaLandscape	= false;
RomLocation	status48SXROMLocation		= ROM_UNAVAILABLE;
RomLocation	status48GXROMLocation		= ROM_UNAVAILABLE;
RomLocation	status49GROMLocation		= ROM_UNAVAILABLE;
Boolean		statusInternalFiles			= false;
Boolean		statusVolumeMounted			= false;
Boolean		statusInUnmountDialog		= false;
Boolean		statusRUN					= false;
Boolean		statusSkipScreenDraw		= false;
UInt8		statusPrefTabActive		    = 1;    	// 1 =general,2=48GX,4=48SX,8=49G
UInt8		statusObjectLSActive		= 1;		// 1 = load, 2 = save
calcTarget 	statusNextTarget			= HP49G;
RGBColorType	*statusPixelColor		= &option48GXPixelColor;
RGBColorType	*statusBackColor		= &option48GXBackColor;
Boolean		statusDelayLCDToggle		= false;
MemLocation	statusUseSystemHeap			= ALL_IN_FTRMEM;
Boolean		statusLandscapeAvailable	= false;
Boolean		statusCoreLoaded			= false;
Boolean		statusPanelHidden			= false;
Boolean 	statusFastDevice			= false;
Boolean 	statusDisplayMode2x2		= false;
Boolean 	status5WayNavPresent		= false;
NavType 	status5WayNavType			= PALMONE;

void RegisterVFSExtensions(void);

/***********************************************************************
 *
 *	Internal Functions
 *
 ***********************************************************************/

SysAppInfoPtr 	SysGetAppInfo(SysAppInfoPtr *uiAppPP, SysAppInfoPtr *actionCodeAppPP)
							SYS_TRAP(sysTrapSysGetAppInfo);
#define memNewChunkFlagAllowLarge			0x1000

Err MemPtrNewLarge(Int32 size, void **newPtr)
{
	SysAppInfoPtr blankAppInfoP;
	UInt16 ownerID;
	
	ownerID = SysGetAppInfo(&blankAppInfoP, &blankAppInfoP)->memOwnerID;
	
	*newPtr = MemChunkNew(0, size, ownerID | memNewChunkFlagNonMovable | memNewChunkFlagAllowLarge);
	
	if (*newPtr==NULL) return memErrNotEnoughSpace;
	
	return errNone;
}


static UInt32 GetHeapSpace(void)
{
	UInt16 numHeaps0;
	UInt32 heapFree,heapMax;
	int    i;
	
	numHeaps0 = MemNumHeaps(0);
	for (i=0;i<numHeaps0;i++)
	{
		if (MemHeapDynamic(i))
		{
			MemHeapFreeBytes(i, &heapFree, &heapMax);
			return heapFree;
		}
	}
	return 0;
}


WinHandle OverwriteWindow(Coord width, Coord height, WindowFormatType format, UInt16 *error, WinHandle currWin)
{
	WinHandle newWin;
	
	if (currWin != NULL) WinDeleteWindow(currWin,false);
	
	newWin = WinCreateOffscreenWindow(width,height,format,error);
	
	return newWin;
}




/***********************************************************************
 *
 * FUNCTION:    FastDrawBitmap
 *
 * DESCRIPTION: This routine uncompresses an 8-bit, Packbits-compressed,
 *              version 3 bitmap
 *              with color table and displays it in the desired window. 
 *              It does not do any bounds checking.
 *              It does not support transparency. 
 *              The bitmap must fit completely
 *              inside the enclosing window.
 *
 *
 ***********************************************************************/
typedef struct {
	UInt32		*srcBmp;
	UInt32		*dstBmp;
	Int32		x;
	Int32		y;
	UInt32		landscape;
	UInt32		compType;
} fdbmType;


void FastDrawBitmap(BitmapPtr sourceBitmap, Coord x, Coord y)
{
	UInt8 *data;
	UInt16 *dest;
	UInt16 runCount;
	BitmapPtr destBitmap;
	Coord destWidth,destHeight;
	Coord srcWidth,srcHeight;
	UInt16 rowCount=0;
	UInt32 rowSkip;
	UInt16 c1;
	UInt16 colorTable[256];
	UInt16 numCTEntries;
	UInt16 i;
	UInt8 index,cr,cg,cb;
	UInt16 *ctPtr;
	Int32 numPixels;
	BitmapCompressionType cType;
	UInt16 rowB;
	
	fdbmType		*fdbmData;		
	MemHandle 		armH;
	MemPtr    		armP;
	UInt32			processorType;
	
	
	// get the processor type
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
				
	if (processorType == sysFtrNumProcessorx86)
	{
		destBitmap = WinGetBitmap(WinGetDrawWindow());
		dest = BmpGetBits(destBitmap);
		BmpGetDimensions(destBitmap,&destWidth,&destHeight,&rowB);
		
		if (statusPortraitViaLandscape)
			dest += ((Int32)destWidth*(Int32)(destHeight-1-x))+(Int32)y;
		else
			dest += ((Int32)destWidth*(Int32)y)+(Int32)x;
		
		BmpGetDimensions(sourceBitmap,&srcWidth,&srcHeight,&rowB);
		
		cType=BmpGetCompressionType(sourceBitmap);		
		
		if (statusPortraitViaLandscape)
			rowSkip = ((Int32)destWidth*(Int32)srcWidth)+1;
		else
			rowSkip = destWidth-srcWidth;
		
		numPixels = (Int32)srcWidth*(Int32)srcHeight;
		
		data = (UInt8*)BmpGetColortable(sourceBitmap);
		// data now points to color table
		numCTEntries=(data[0]<<8)|data[1];
		data+=2;
		
		for (i=0;i<numCTEntries;i++)
		{
			index=*data++;
			cr=(*data++)>>3;
			cg=(*data++)>>2;
			cb=(*data++)>>3;
			colorTable[index]=ByteSwap16(((UInt16)cr<<11)|((UInt16)cg<<5)|cb);
		}
		ctPtr=colorTable;
		
		data=BmpGetBits(sourceBitmap);
		
		if (cType==BitmapCompressionTypePackBits)
		{
			// *data now points to compressed data size indicator so skip it
			data+=4;

			if (statusPortraitViaLandscape)
			{
				do
				{
					if (*data&0x80)
					{ 	// repeat data
						runCount=-(*(Int8*)data++)+1;
						c1=ctPtr[*data++];
						
						numPixels-=runCount;

						rowCount+=runCount;
						do 
						{ 
							*dest = c1;
							dest-=destWidth;
						} while (--runCount);
						if (rowCount>=srcWidth)
						{
							rowCount=0;
							dest += rowSkip;
						}
					}
					else
					{	// run data
						runCount=(*data++)+1;

						numPixels-=runCount;

						rowCount+=runCount;
						do 
						{ 
							*dest = ctPtr[*data++]; 
							dest-=destWidth;
						} while (--runCount);
						if (rowCount>=srcWidth)
						{
							rowCount=0;
							dest += rowSkip;
						}
					}
				} while (numPixels>0);
			}
			else
			{
				do
				{
					if (*data&0x80)
					{ 	// repeat data
						runCount=-(*(Int8*)data++)+1;
						c1=ctPtr[*data++];
						
						numPixels-=runCount;

						rowCount+=runCount;
						do { *dest++ = c1; } while (--runCount);
						if (rowCount>=srcWidth)
						{
							rowCount=0;
							dest += rowSkip;
						}
					}
					else
					{	// run data
						runCount=(*data++)+1;

						numPixels-=runCount;

						rowCount+=runCount;
						do { *dest++ = ctPtr[*data++]; } while (--runCount);
						if (rowCount>=srcWidth)
						{
							rowCount=0;
							dest += rowSkip;
						}
					}
				} while (numPixels>0);
			}
		}
		else if (cType==BitmapCompressionTypeNone)
		{
			// *data now points directly to data

			if (statusPortraitViaLandscape)
			{
				do
				{
					numPixels--;
					rowCount++;
					*dest = ctPtr[*data++]; 
					dest-=destWidth;
					if (rowCount>=srcWidth)
					{
						rowCount=0;
						dest += rowSkip;
						data += (rowB-srcWidth);
					}
				} while (numPixels>0);
			}
			else
			{
				do
				{
					numPixels--;
					rowCount++;
					*dest++ = ctPtr[*data++];
					if (rowCount>=srcWidth)
					{
						rowCount=0;
						dest += rowSkip;
						data += (rowB-srcWidth);
					}
				} while (numPixels>0);
			}
		}
	}
	else
	{
		// Do it via armlet
		fdbmData = (fdbmType*) MemPtrNew(sizeof(fdbmType));
		
		fdbmData->srcBmp 	= (UInt32*)sourceBitmap;
		fdbmData->dstBmp 	= (UInt32*)WinGetBitmap(WinGetDrawWindow());
		fdbmData->x			= (Int32)x;
		fdbmData->y			= (Int32)y;
		fdbmData->landscape	= (UInt32)statusPortraitViaLandscape;
		fdbmData->compType	= (UInt32)BmpGetCompressionType(sourceBitmap);
		
		armH = DmGetResource('ARMC', 0x3040);
		armP = MemHandleLock(armH);
		
		PceNativeCall(armP, fdbmData);
		
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
		MemPtrFree(fdbmData);
	}
}








#define ReadUnaligned32Swapped(addr)  \
	( ((((unsigned char *)(addr))[3]) << 24) | \
	  ((((unsigned char *)(addr))[2]) << 16) | \
	  ((((unsigned char *)(addr))[1]) <<  8) | \
	  ((((unsigned char *)(addr))[0])) )
#define ReadUnaligned16Swapped(addr)  \
	( ((((unsigned char *)(addr))[1]) <<  8) | \
	  ((((unsigned char *)(addr))[0])) )
/***********************************************************************
 *
 * FUNCTION:    BmpExtractor24
 *
 * DESCRIPTION: This routine "extracts" a 24-bit bitmap from a windows 
 *              formatted .bmp source buffer and converts it to a 16-bit
 *              bitmap and stores it in win.
 *
 *
 ***********************************************************************/
typedef struct {
	UInt8		*buf;
	UInt32		bufSize;
	UInt32		**win;
} be24Type;

void BmpExtractor24(UInt8 *buf, UInt32 bufSize, WinHandle win)
{
	UInt32 fileSize;
	UInt32 picOffset;
	UInt32 width;
	UInt32 height;
	UInt16 depth;
	UInt32 compression;
	UInt8  *imageData,*imageDataBase;
	UInt16 *winData;
	Int16 winHeight,winWidth;
	BitmapPtr winBitmap;
	Int32 i,j;
	UInt8 r,g,b;
	UInt32 bytesPerRow;
	UInt32 rowModulus;
	
	be24Type		be24Data;		
	MemHandle 		armH;
	MemPtr    		armP;
	UInt32			processorType;

	if ((buf[0]!='B') || (buf[1]!='M')) return;
	
	// get the processor type
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);

	if (processorType == sysFtrNumProcessorx86)
	{
		fileSize=ReadUnaligned32Swapped(&buf[2]);
		picOffset=ReadUnaligned32Swapped(&buf[10]);
		width=ReadUnaligned32Swapped(&buf[18]);
		height=ReadUnaligned32Swapped(&buf[22]);
		depth=ReadUnaligned16Swapped(&buf[28]);
		compression=ReadUnaligned32Swapped(&buf[30]);
		
		
		imageDataBase=buf+picOffset;
		
		if ((depth!=24) || (compression!=0)) return;
		
		bytesPerRow = width*3;
		rowModulus = bytesPerRow%4;
		if (rowModulus) bytesPerRow += (4-rowModulus);
		
		winBitmap = WinGetBitmap(win);
		BmpGetDimensions(winBitmap,&winWidth,&winHeight,NULL);
		winData = BmpGetBits(winBitmap);
		
		if ((winWidth!=width)||(winHeight!=height)) return;
		
		if (bufSize<((bytesPerRow*height)+picOffset)) return;
		
		for (j=height-1;j>=0;j--)
		{
			imageData=imageDataBase+(j*bytesPerRow);
			for (i=0;i<width;i++)
			{
				b=(*imageData++)>>3;
				g=(*imageData++)>>2;
				r=(*imageData++)>>3;
				*winData++ = ByteSwap16(((UInt16)r<<11)|((UInt16)g<<5)|b);
			}
		}
	}
	else
	{
		// Do it via armlet		
		be24Data.buf 		= buf;
		be24Data.bufSize 	= bufSize;
		be24Data.win		= (UInt32**)win;
		
		armH = DmGetResource('ARMC', 0x3080);
		armP = MemHandleLock(armH);
		
		PceNativeCall(armP, &be24Data);
		
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
	}	
}




/***********************************************************************
 *
 * FUNCTION:    GreyOutRectangle
 *
 * DESCRIPTION: This routine "Greys Out" the portion of the main display
 *              window specified by OpRect
 *
 *
 ***********************************************************************/
typedef struct {
	UInt16		*winBase;
	UInt32		x;
	UInt32		y;
	UInt32		extentX;
	UInt32		extentY;
	UInt32		rowInts;
} gorType;

void GreyOutRectangle(RectangleType opRect, WinHandle win)
{
	UInt16 *winBase;
	UInt16 valRed,valGreen,valBlue;
	UInt16 deltaRed,deltaGreen,deltaBlue;
	UInt16 baseRed,baseGreen,baseBlue;
	UInt16 rowInts;
	BitmapPtr tempBitmap;
	Coord height;
	int i,j;

	gorType			*gorData;		
	MemHandle 		armH;
	MemPtr    		armP;
	UInt32			processorType;
	

	tempBitmap = WinGetBitmap(win);
	winBase = BmpGetBits(tempBitmap);
	BmpGetDimensions(tempBitmap,NULL,&height,&rowInts);
	rowInts>>=1;
	
	if (statusPortraitViaLandscape) swapRectPVL(&opRect,height);
	
	// get the processor type
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
				
	if (processorType == sysFtrNumProcessorx86)
	{
		baseRed = 24;  // 16 bit representation of gray dialog background
		baseGreen=48;
		baseBlue= 24;
		
		winBase += ((Int32)opRect.topLeft.y*rowInts)+opRect.topLeft.x;
		
		for (i=0;i<opRect.extent.y;i++)
		{
			for (j=0;j<opRect.extent.x;j++)
			{
				valRed = ByteSwap16(*winBase);
				valBlue=valRed&0x1f;
				valRed>>=5;
				valGreen=valRed&0x3f;
				valRed>>=6;
					
				deltaRed   = ((baseRed-valRed)/2);
				deltaBlue  = ((baseBlue-valBlue)/2);
				deltaGreen = ((baseGreen-valGreen)/2);
					 
				valRed=(valRed + deltaRed)&0x1f;
				valBlue=(valBlue + deltaBlue)&0x1f;
				valGreen=(valGreen + deltaGreen)&0x3f;
					
				valRed<<=6;
				valRed|=valGreen;
				valRed<<=5;
				valRed|=valBlue;
				*winBase++ = ByteSwap16(valRed);
			}
			winBase += rowInts-opRect.extent.x;
		}
	}
	else
	{
		// Do it via armlet

		gorData = (gorType*) MemPtrNew(sizeof(gorType));
	
		gorData->winBase 	= winBase;
		gorData->x 			= (UInt32)opRect.topLeft.x; 
		gorData->y			= (UInt32)opRect.topLeft.y;
		gorData->extentX	= (UInt32)opRect.extent.x;
		gorData->extentY	= (UInt32)opRect.extent.y;
		gorData->rowInts	= (UInt32)rowInts;
	
		armH = DmGetResource('ARMC', 0x3050);
		armP = MemHandleLock(armH);
		
		PceNativeCall(armP, gorData);
	
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
		MemPtrFree(gorData);
	}
}




/***********************************************************************
 *
 * FUNCTION:    DrawProgressBar(UInt8 percent)
 *
 * DESCRIPTION: This function is responsible for drawing and updating
 *              a 0 to 100% progress bar.
 * 
 *              Bar type: STARTUP: 0 to 100
 *                        SWITCH:  0 to 200
 *                        SHUTDOWN:0 to 100
 *
 ***********************************************************************/
UInt8 previousProgress=0;
UInt8 progressMax=100;
UInt8 divisor = 1;

static void DrawProgressBar(UInt8 progress, ProgBarType progBar)
{
	RectangleType opRect;
	RGBColorType prevRGB;
	UInt8 currentProgress;
	
	if (optionVerbose) return;
		
	WinSetForeColorRGB(statusPixelColor,NULL);
	WinSetTextColorRGB(statusPixelColor,&prevRGB);
	WinSetBackColorRGB(statusBackColor, NULL);
		
	if (progress==0)
	{
		if (progBar == SWITCH)
		{
			divisor = 2;
			progressMax=200;
		}
		else
		{
			divisor = 1;
			progressMax=100;
		}
		
		display_clear();
		
		// draw initial bar
		if (statusDisplayMode2x2)
		{
			// double size display bar
			FntSetFont(boldFont);
			if (progBar==STARTUP)
				DrawCharsExpanded("Starting Up...", 14, fullScreenOriginX+30, fullScreenOriginY+44,0,winOverlay,DOUBLE_DENSITY);
			else if (progBar==SWITCH)
				DrawCharsExpanded("Switching Target...", 19, fullScreenOriginX+30, fullScreenOriginY+44,0,winOverlay,DOUBLE_DENSITY);
			else if (progBar==SHUTDOWN)
				DrawCharsExpanded("Shutting Down...", 16, fullScreenOriginX+30, fullScreenOriginY+44,0,winOverlay,DOUBLE_DENSITY);
			opRect.topLeft.x=fullScreenOriginX+30;
			opRect.topLeft.y=fullScreenOriginY+70;
			opRect.extent.x =204;
			opRect.extent.y =16;
			if (statusPortraitViaLandscape) swapRectPVL(&opRect,screenHeight);
			WinDrawRectangleFrame(simpleFrame, &opRect);
		}
		else
		{
			// regular size display bar
			FntSetFont(stdFont);
			WinSetDrawMode(winOverlay);
			if (progBar==STARTUP)
				DrawCharsExpanded("Starting Up...", 14, lcd1x1FSOriginX+15, lcd1x1FSOriginY+16,0,winOverlay,SINGLE_DENSITY);
			else if (progBar==SWITCH)
				DrawCharsExpanded("Switching Target...", 19, lcd1x1FSOriginX+15, lcd1x1FSOriginY+16,0,winOverlay,SINGLE_DENSITY);
			else if (progBar==SHUTDOWN)
				DrawCharsExpanded("Shutting Down...", 16, lcd1x1FSOriginX+15, lcd1x1FSOriginY+16,0,winOverlay,SINGLE_DENSITY);
			opRect.topLeft.x=lcd1x1FSOriginX+15;
			opRect.topLeft.y=lcd1x1FSOriginY+30;
			opRect.extent.x =102;
			opRect.extent.y =10;
			WinDrawRectangleFrame(simpleFrame, &opRect);
		}
		previousProgress=0;
	}
	else
	{
		currentProgress = previousProgress+progress;
		
		if (currentProgress>progressMax) currentProgress=progressMax;
		
		// increase bar length by progress
		if (statusDisplayMode2x2)
		{
			opRect.topLeft.x=fullScreenOriginX+32+((previousProgress*2)/divisor);
			opRect.topLeft.y=fullScreenOriginY+72;
			opRect.extent.x =(progress*2)/divisor;
			opRect.extent.y =12;
			if (statusPortraitViaLandscape) swapRectPVL(&opRect,screenHeight);
			WinDrawRectangle(&opRect, 0);
		}
		else
		{
			opRect.topLeft.x=lcd1x1FSOriginX+16+(previousProgress/divisor)-(previousProgress!=0);
			opRect.extent.x =(progress/divisor)+(previousProgress!=0);
			opRect.topLeft.y=lcd1x1FSOriginY+31;
			opRect.extent.y =8;
			WinDrawRectangle(&opRect, 0);
		}
		previousProgress=currentProgress;
	}
	WinSetForeColorRGB(&prevRGB,NULL);
}







/***********************************************************************
 *
 * FUNCTION:    CrossFade
 *
 * DESCRIPTION: This function performs a crossfade from one bitmap to the next
 *
 ***********************************************************************/
typedef struct {
	UInt32		**fromWin;
	UInt32		**toWin;
	UInt32		**destWin;
	UInt32		offsetX;
	UInt32		offsetY;
	UInt32		transparent;
	UInt32		stepping;
} crossFadeDataType;



void CrossFade(WinHandle srcWin, WinHandle destWin, UInt32 offsetX, UInt32 offsetY, Boolean transparent,UInt16 stepping)
{
	crossFadeDataType	*cfData;		
	MemHandle 		armH;
	MemPtr    		armP;
	WinHandle		fromWin;
	Err				error;
	Coord			width,height;
	RectangleType	copyRect;
	UInt32			processorType;
	Int16			toWidth,toHeight;

	BmpGetDimensions(WinGetBitmap(srcWin), &width, &height, NULL);
	fromWin = WinCreateOffscreenWindow(width,height,nativeFormat,&error);
	
	// get the processor type
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
				
	if ((processorType == sysFtrNumProcessorx86) || (error))
	{
		// Either we're on the simulator or we couldn't allocate enough memory for offscreen win
		BmpGetDimensions(WinGetBitmap(srcWin), &toWidth, &toHeight, NULL);
		copyRect.topLeft.x = 0;
		copyRect.topLeft.y = 0;
		copyRect.extent.y = toHeight; 
		copyRect.extent.x = toWidth;
		
		WinCopyRectangle(srcWin, mainDisplayWindow,&copyRect,offsetX,offsetY,winPaint);
	}
	else
	{
		copyRect.topLeft.x = offsetX;
		copyRect.topLeft.y = offsetY;
		copyRect.extent.y = height; 
		copyRect.extent.x = width;
		WinCopyRectangle(mainDisplayWindow,fromWin,&copyRect,0,0,winPaint);
	
		if (!statusFastDevice) stepping<<=1;
		
		// Do it via armlet
		cfData = (crossFadeDataType*) MemPtrNew(sizeof(crossFadeDataType));
		
		cfData->fromWin 	= (UInt32**)fromWin;
		cfData->toWin		= (UInt32**)srcWin; 
		cfData->destWin		= (UInt32**)destWin;
		cfData->offsetX		= offsetX;
		cfData->offsetY		= offsetY;
		cfData->transparent	= (UInt32)transparent;
		cfData->stepping	= (UInt32)stepping;
		
		armH = DmGetResource('ARMC', 0x3030);
		armP = MemHandleLock(armH);
		
		PceNativeCall(armP, cfData);
		
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
		MemPtrFree(cfData);
		
		WinDeleteWindow(fromWin,false);
	}	
}


/***********************************************************************
 *
 * FUNCTION:    ByteUtility
 *
 *
 ***********************************************************************/
typedef struct {
	UInt8 		*buf;
	UInt32		fileSize;
	UInt32		operation;
} utilityDataType;

UInt32 ByteUtility(UInt8* buf, UInt32 tSize, compOpType operation)
{
	utilityDataType	data;		
	MemHandle 		armH;
	MemPtr    		armP;
	UInt32			processorType;
	UInt8			*a,*b;
	UInt32			rVal=0;
	UInt32			dVal,i;
	UInt32			addHighBit;
	
	// get the processor type
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
				
	if (processorType == sysFtrNumProcessorx86)
	{
		if (operation==COMPRESS)
		{
			a=&buf[tSize];
			b=a;
			tSize>>=1;
			
			do
			{
				*--a = *--b<<4;
				*a |= *--b&0xf;
			} while(--tSize);
		}
		else if (operation==UNCOMPRESS)
		{
			a=&buf[0];
			b=&buf[tSize];
			
			do
			{
				*a++=*b&0xf;
				*a++=*b++>>4;
			} while(--tSize);
		}
		else if (operation==CHECKSUM)
		{
			rVal=0;
			for (i=0;i<tSize;i++)
			{
				addHighBit=false;
				dVal=buf[i];
				dVal^=0xFF;
				rVal+=dVal;
				if (rVal&0x1) addHighBit=true;
				rVal>>=1;
				if (addHighBit) rVal|=0x80000000;
			}
		}
	}
	else
	{
		data.buf 		= buf;
		data.fileSize 	= tSize;
		data.operation	= (UInt32)operation;
		
		armH = DmGetResource('ARMC', 0x3011);
		armP = MemHandleLock(armH);
			
		rVal = PceNativeCall(armP, &data);
		
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
	}
	return rVal;
}




/***********************************************************************
 *
 * FUNCTION:    DrawCharsExpanded
 *
 *
 ***********************************************************************/

void DrawCharsExpanded (char *text, UInt16 length, Int16 x, Int16 y, Int16 maxWidth, WinDrawOperation operation, densType density)
{
	Int16 lineHeight,lineWidth;
	Int16 drawHeight;
	UInt16 drawRowInts;
	WinHandle tempWin;
	UInt16 *srcPtr,*destPtr,*destPtrBase;
	UInt16 srcVal;
	UInt16 h,w;
	BitmapPtr drawBMP;

		
	lineHeight=FntLineHeight();
	lineWidth=FntLineWidth(text,length);
		
	if (operation==winOverlay) WinSetBackColorRGB(&whiteRGB,NULL);
		
	tempWin=WinGetDrawWindow();

	WinSetDrawWindow(textWindow);
	if (density==SINGLE_DENSITY)
	{
		lineHeight>>=1;
		lineWidth>>=1;
		BmpSetDensity(textBitmap,kDensityLow);
		if (maxWidth)
		{
			WinDrawTruncChars(text,length,0,0,maxWidth<<1);
			if (lineWidth>maxWidth) lineWidth=maxWidth;
		}
		else
		{
			WinDrawChars(text,length,0,0);
		}
		BmpSetDensity(textBitmap,kDensityDouble);
	}
	else
	{
		if (maxWidth)
		{
			WinDrawTruncChars(text,length,0,0,maxWidth);
			if (lineWidth>maxWidth) lineWidth=maxWidth;
		}
		else
		{
			WinDrawChars(text,length,0,0);
		}
	}

	WinSetDrawWindow(tempWin);
		
	srcPtr=textBitmapPtr;
	drawBMP=WinGetBitmap(tempWin);
	BmpGetDimensions(drawBMP,NULL,&drawHeight,&drawRowInts);
	drawRowInts>>=1;

	destPtrBase=BmpGetBits(drawBMP);
		
	if (!statusPortraitViaLandscape)
	{
		destPtr = destPtrBase + ((UInt32)drawRowInts*(Int32)y)+(Int32)x;
			
		if (operation==winOverlay)
		{
			for (h=0;h<lineHeight;h++)
			{
				for(w=0;w<lineWidth;w++)
				{
					srcVal=*srcPtr++;
					if (srcVal!=0xffff)
						*destPtr=srcVal;
					destPtr++;
				}
				destPtr+=drawRowInts-lineWidth;
				srcPtr+=maxExpandedTextWidth-lineWidth;
			}
		}
		else
		{
			for (h=0;h<lineHeight;h++)
			{
				for(w=0;w<lineWidth;w++)
				{
					*destPtr++ = *srcPtr++;
				}
				destPtr+=drawRowInts-lineWidth;
				srcPtr+=maxExpandedTextWidth-lineWidth;
			}
		}
	}
	else
	{
		destPtrBase += ((UInt32)drawRowInts*(Int32)(drawHeight-1-x))+(Int32)y;
			
		if (operation==winOverlay)
		{
			for (h=0;h<lineHeight;h++)
			{
				destPtr=destPtrBase+h;
				for(w=0;w<lineWidth;w++)
				{
					srcVal=*srcPtr++;
					if (srcVal!=0xffff)
						*destPtr=srcVal;
					destPtr-=drawRowInts;
				}
				srcPtr+=maxExpandedTextWidth-lineWidth;
			}
		}
		else
		{
			for (h=0;h<lineHeight;h++)
			{
				destPtr=destPtrBase+h;
				for(w=0;w<lineWidth;w++)
				{
					*destPtr=*srcPtr++;
					destPtr-=drawRowInts;
				}
				srcPtr+=maxExpandedTextWidth-lineWidth;
			}
		}
	}
}



/***********************************************************************
 *
 * FUNCTION:    swapRectPVL
 *
 ***********************************************************************/

void swapRectPVL(RectangleType *r,UInt16 height)
{
	Coord temp;
	
	temp=r->extent.x;
	r->extent.x=r->extent.y;
	r->extent.y=temp;
	temp=r->topLeft.x;
	r->topLeft.x=r->topLeft.y;
	r->topLeft.y=height-temp-r->extent.y;
}



/***********************************************************************
 *
 * FUNCTION:    P48WinCopyRectangle
 *
 ***********************************************************************/

void P48WinCopyRectangle(WinHandle srcWin, WinHandle dstWin, RectangleType *srcRect, Coord destX, Coord destY, WinDrawOperation mode)
{
	RectangleType rect;
	Coord width;
	
	if (statusPortraitViaLandscape)
	{
		// swap all the variables
		WinGetBounds(srcWin,&rect);
		width = srcRect->extent.x;
		swapRectPVL(srcRect,rect.extent.y);
		WinGetBounds(dstWin,&rect);
		WinCopyRectangle(srcWin,dstWin,srcRect,destY,rect.extent.y-destX-width,mode);
	}
	else
	{
		WinCopyRectangle(srcWin,dstWin,srcRect,destX,destY,mode);
	}
}


/***********************************************************************
 *
 * FUNCTION:    P48WinDrawRectangle
 *
 ***********************************************************************/

void P48WinDrawRectangle(RectangleType *rP, UInt16 cornerDiam)
{
	RectangleType rect;
	
	if (statusPortraitViaLandscape)
	{
		// swap all the variables
		WinGetBounds(WinGetDrawWindow(),&rect);
		swapRectPVL(rP,rect.extent.y);
		WinDrawRectangle(rP,cornerDiam);
	}
	else
	{
		WinDrawRectangle(rP,cornerDiam);
	}
}


/***********************************************************************
 *
 * FUNCTION:    P48WinPaintRectangle
 *
 ***********************************************************************/

void P48WinPaintRectangle(RectangleType *rP, UInt16 cornerDiam)
{
	RectangleType rect;
	
	if (statusPortraitViaLandscape)
	{
		// swap all the variables
		WinGetBounds(WinGetDrawWindow(),&rect);
		swapRectPVL(rP,rect.extent.y);
		WinPaintRectangle(rP,cornerDiam);
	}
	else
	{
		WinPaintRectangle(rP,cornerDiam);
	}
}


/***********************************************************************
 *
 * FUNCTION:    P48WinEraseRectangle
 *
 ***********************************************************************/

void P48WinEraseRectangle(RectangleType *rP, UInt16 cornerDiam)
{
	RectangleType rect;
	
	if (statusPortraitViaLandscape)
	{
		// swap all the variables
		WinGetBounds(WinGetDrawWindow(),&rect);
		swapRectPVL(rP,rect.extent.y);
		WinEraseRectangle(rP,cornerDiam);
	}
	else
	{
		WinEraseRectangle(rP,cornerDiam);
	}
}


/***********************************************************************
 *
 * FUNCTION:    GetNextTarget
 *
 *
 ***********************************************************************/
static calcTarget GetNextTarget(calcTarget currentTarget)
{
	calcTarget nextTarget;
	
	if (currentTarget==HP48SX)
	{
		if (status48GXROMLocation)
			nextTarget=HP48GX;
		else
			nextTarget=HP49G;
	}
	else if (currentTarget==HP48GX)
	{
		if (status49GROMLocation)
			nextTarget=HP49G;
		else
			nextTarget=HP48SX;
	}
	else
	{
		if (status48SXROMLocation)
			nextTarget=HP48SX;
		else
			nextTarget=HP48GX;
	}
	
	return nextTarget;
}






/***********************************************************************
 *
 * FUNCTION:    LoadBackgroundImage
 *
 *
 ***********************************************************************/
void LoadBackgroundImage(calcTarget loadTarget)
{	
	FileRef			myRef;
	UInt32			fileSize;
	UInt32			numRead;
	void			*mem;
	Boolean 		tileBackground;
	Err				error=errNone;
	DmResID			tileID;
	int				h,w;
	MemHandle 		tileHandle;
	BitmapPtr 		tileBitmap;
	Coord 			tileWidth,tileHeight;
	RectangleType	screenRect;
	UInt16			*srcPtr,*destPtr,*destPtrBase;
	UInt32			lineskip;
	
	if (!statusBackdrop) return;
	
	backdrop1x1 = OverwriteWindow(148,64,nativeFormat,&error,backdrop1x1);
	if (error)
	{
		statusBackdrop=false;
		return;
	}
	backdrop2x2 = OverwriteWindow(262,142,nativeFormat,&error,backdrop2x2);
	if (error)
	{
		WinDeleteWindow(backdrop1x1,false);
		backdrop1x1=NULL;
		statusBackdrop=false;
		return;
	}
	if (statusLandscapeAvailable)
	{
		backdrop2x2PVL = OverwriteWindow(142,262,nativeFormat,&error,backdrop2x2PVL);
		if (error)
		{
			WinDeleteWindow(backdrop2x2,false);
			WinDeleteWindow(backdrop1x1,false);
			backdrop1x1=NULL;
			backdrop2x2=NULL;
			statusBackdrop=false;
			return;
		}
	}
	
	Q->backgroundImagePtr1x1=BmpGetBits(WinGetBitmap(backdrop1x1));
	Q->backgroundImagePtr2x2=BmpGetBits(WinGetBitmap(backdrop2x2));
	Q->backgroundImagePtr1x1=(UInt16*)ByteSwap32(Q->backgroundImagePtr1x1);
	Q->backgroundImagePtr2x2=(UInt32*)ByteSwap32(Q->backgroundImagePtr2x2);
		
	tileBackground=true;
	if (loadTarget==HP48SX)
	{
		error = VFSFileOpen(msVolRefNum,sxVFSBackImage2x2Name,vfsModeRead,&myRef);
		tileID = Hp48sxTileBitmapFamily;
	}
	else if (loadTarget==HP48GX)
	{
		error = VFSFileOpen(msVolRefNum,gxVFSBackImage2x2Name,vfsModeRead,&myRef);
		tileID = Hp48gxTileBitmapFamily;
	}
	else
	{
		error = VFSFileOpen(msVolRefNum,g49VFSBackImage2x2Name,vfsModeRead,&myRef);
		tileID = Hp49gTileBitmapFamily;
	}
	
	if (!error)
	{
		if (!VFSFileSize(myRef, &fileSize))
		{
			if (!FtrPtrNew(appFileCreator, tempBMPFtrNum, fileSize, &mem))
			{
				if (!VFSFileReadData(myRef,fileSize,mem,0,&numRead))
				{
					BmpExtractor24(mem,fileSize,backdrop2x2);
					tileBackground=false;
				}
				FtrPtrFree(appFileCreator, tempBMPFtrNum);
			}
		}
		VFSFileClose(myRef);
	}
	if (tileBackground)
	{
		// get background tile for display
		tileHandle = DmGetResource(bitmapRsc, tileID);
		tileBitmap = MemHandleLock(tileHandle);
		BmpGetDimensions(tileBitmap,&tileWidth,&tileHeight,NULL);
		WinSetDrawWindow(backdrop2x2);
		for (h=0;h<142;h+=tileHeight)
			for (w=0;w<262;w+=tileWidth)
				WinDrawBitmap(tileBitmap,w,h);
		MemHandleUnlock(tileHandle);
		DmReleaseResource(tileHandle);
	}
	
	// Make rotated copy of backdrop2x2 window into backdrop2x2PVL
	if (statusLandscapeAvailable)
	{
		lineskip=142;
		destPtrBase=BmpGetBits(WinGetBitmap(backdrop2x2PVL));
		destPtrBase+=142L*261L;
		srcPtr=BmpGetBits(WinGetBitmap(backdrop2x2));
		for (h=0;h<142;h++)
		{
			destPtr=destPtrBase+h;
			for(w=0;w<262;w++)
			{
				*destPtr=*srcPtr++;
				destPtr-=lineskip;
			}
		}
		
	}
	
			
	tileBackground=true;
	if (loadTarget==HP48SX)
		error = VFSFileOpen(msVolRefNum,sxVFSBackImage1x1Name,vfsModeRead,&myRef);
	else if (loadTarget==HP48GX)
		error = VFSFileOpen(msVolRefNum,gxVFSBackImage1x1Name,vfsModeRead,&myRef);
	else
		error = VFSFileOpen(msVolRefNum,g49VFSBackImage1x1Name,vfsModeRead,&myRef);

	if (!error)
	{
		if (!VFSFileSize(myRef, &fileSize))
		{
			if (!FtrPtrNew(appFileCreator, tempBMPFtrNum, fileSize, &mem))
			{
				if (!VFSFileReadData(myRef,fileSize,mem,0,&numRead))
				{
					BmpExtractor24(mem,fileSize,backdrop1x1);
					tileBackground=false;
				}
				FtrPtrFree(appFileCreator, tempBMPFtrNum);
			}
		}
		VFSFileClose(myRef);
	}
	if (tileBackground)
	{
		// get background tile for display
		tileHandle = DmGetResource(bitmapRsc, tileID);
		tileBitmap = MemHandleLock(tileHandle);
		BmpGetDimensions(tileBitmap,&tileWidth,&tileHeight,NULL);
		WinSetDrawWindow(backdrop1x1);
		for (h=0;h<64;h+=tileHeight)
			for (w=0;w<148;w+=tileWidth)
				WinDrawBitmap(tileBitmap,w,h);
				
		screenRect.topLeft.x = 12;
		screenRect.topLeft.y = 0;
		screenRect.extent.x = lcd1x1DividerExtentX;
		screenRect.extent.y = lcd1x1DividerExtentY;
		WinSetForeColorRGB(&greyRGB,NULL);
		WinDrawRectangle(&screenRect, 0);
		
		MemHandleUnlock(tileHandle);
		DmReleaseResource(tileHandle);
	}
	
	WinSetDrawWindow(mainDisplayWindow);
}



/***********************************************************************
 *
 * FUNCTION:    CheckDisplayTypeForGUIRedraw
 *
 *
 ***********************************************************************/
Boolean CheckDisplayTypeForGUIRedraw(void)
{
	Boolean 		guiRedrawNecessary=false;
	FormType 		*frmP;
	RectangleType	formBounds;
	WinHandle		frmWinH; 

	// Determine display characteristics based on current state of VG area
	WinScreenGetAttribute(winScreenHeight, &screenHeight);

	frmP = FrmGetActiveForm();
	
	//resize form and determine whether to use 320^2 or 320x450 GUI
	WinGetDisplayExtent(&displayExtX,&displayExtY);
	frmWinH = FrmGetWindowHandle(frmP);
	WinGetBounds(frmWinH, &formBounds);
	formBounds.extent.x = displayExtX;
	formBounds.extent.y = displayExtY;
	WinSetBounds( frmWinH, &formBounds );

	optionThreeTwentySquared = (displayExtX==displayExtY);
	
	if (optionThreeTwentySquared!=previousOptionThreeTwentySquared) guiRedrawNecessary=true;
	
	return guiRedrawNecessary;
}



/***********************************************************************
 *
 * FUNCTION:    GUIRedraw
 *
 *
 ***********************************************************************/
void GUIRedraw(void)
{
	Int16 i;
	UInt32 temp;
	
	WinScreenGetAttribute(winScreenRowBytes, &screenRowInt16s);
	screenRowInt16s>>=1;
	Q->rowInt16s = ByteSwap32(screenRowInt16s);
	temp=(27*screenRowInt16s)-skipToKeyD;
	Q->labelOffset[3]=ByteSwap32(temp);
	
	DisplayHP48Screen(optionTarget);
	
	init_lcd();
	
	for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
	display_clear();
	Q->lcdPreviousAnnunciatorState=0;
	Q->statusChangeAnn=true;
}



/***********************************************************************
 *
 * FUNCTION:    DrawHP48Graphic
 *
 *
 ***********************************************************************/
static void DrawHP48Graphic(calcTarget loadTarget)
{
	DmResID			part1BF,part2BF,part3BF,part4BF;
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;

	if (loadTarget==HP48SX)
	{
		if (optionThreeTwentySquared)
		{
			part1BF = Hp48sx320p1BitmapFamily;
			part2BF = Hp48sx320p2BitmapFamily;
			part3BF = Hp48sx320p3BitmapFamily;
		}
		else
		{
			part1BF = Hp48sxp1BitmapFamily;
			part2BF = Hp48sxp2BitmapFamily;
			part3BF = Hp48sxp3BitmapFamily;
			part4BF = Hp48sxp4BitmapFamily;
		}
	}
	else if (loadTarget==HP48GX)
	{
		if (optionThreeTwentySquared)
		{
			part1BF = Hp48gx320p1BitmapFamily; 
			part2BF = Hp48gx320p2BitmapFamily;
			part3BF = Hp48gx320p3BitmapFamily;
		}
		else
		{
			part1BF = Hp48gxp1BitmapFamily;
			part2BF = Hp48gxp2BitmapFamily;
			part3BF = Hp48gxp3BitmapFamily;
			part4BF = Hp48gxp4BitmapFamily;
		}
	}
	else
	{
		if (optionThreeTwentySquared)
		{
			part1BF = Hp49g320p1BitmapFamily; 
			part2BF = Hp49g320p2BitmapFamily;
			part3BF = Hp49g320p3BitmapFamily;
		}
		else
		{
			part1BF = Hp49gp1BitmapFamily;
			part2BF = Hp49gp2BitmapFamily;
			part3BF = Hp49gp3BitmapFamily;
			part4BF = Hp49gp4BitmapFamily;
		}
	}

	picHandle = DmGetResource(bitmapRsc, part1BF);
	picBitmap = MemHandleLock(picHandle);
	FastDrawBitmap(picBitmap,0,0);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
	picHandle = DmGetResource(bitmapRsc, part2BF);
	picBitmap = MemHandleLock(picHandle);
	FastDrawBitmap(picBitmap,0,120);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
	picHandle = DmGetResource(bitmapRsc, part3BF);
	picBitmap = MemHandleLock(picHandle);
	FastDrawBitmap(picBitmap,0,240);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
	if (!optionThreeTwentySquared)
	{
		picHandle = DmGetResource(bitmapRsc, part4BF);
		picBitmap = MemHandleLock(picHandle);
		FastDrawBitmap(picBitmap,0,360);
		MemHandleUnlock(picHandle);
		DmReleaseResource(picHandle);
	}
}



/***********************************************************************
 *
 * FUNCTION:    DisplayHP48Screen
 *
 *
 ***********************************************************************/
Boolean firstTime=true;

Err DisplayHP48Screen(calcTarget loadTarget)
{	
	DmResID			miniBF;
	calcTarget		nextTarget;
	Err				error;
	Int16			temp;
	
	
	previousOptionThreeTwentySquared=optionThreeTwentySquared;

	statusPortraitViaLandscape=false;
	if (optionThreeTwentySquared)
	{
		statusDisplayMode2x2=false;
		if (optionDisplayZoomed)
		{
			optionDisplayZoomed=false;
			if (!statusRUN)
			{
				statusDelayLCDToggle=true;
			}
			else
			{
				Q->delayEvent=0x3200;  // wait 50 keyboard ticks before triggering a LCD zoom.
				Q->delayKey=virtKeyVideoSwapLE;
			}
		}
		
		virtLayout = (squareDef*)virtLayout1x1;
		if (loadTarget==HP49G)
			keyLayout = (keyDef*)keyLayout320_hp49;
		else
			keyLayout = (keyDef*)keyLayout320;
	}
	else
	{
		statusDisplayMode2x2=true;
		if (statusLandscapeAvailable)
		{
			statusPortraitViaLandscape = true;
			temp=displayExtY;
			displayExtY=displayExtX;
			displayExtX=temp;
		}
		virtLayout = (squareDef*)virtLayout2x2;
		if (loadTarget==HP49G)
			keyLayout = (keyDef*)keyLayout450_hp49;
		else
			keyLayout = (keyDef*)keyLayout450;
	}		

	if (loadTarget==HP48SX)
	{
		miniBF  = Hp48gxMiniBitmapFamily;	// only applicable in DRY-RUN scenario
		statusBackdrop = option48SXBackdrop;
		statusPixelColor = &option48SXPixelColor;
		statusBackColor	= &option48SXBackColor;
		statusColorCycle = option48SXColorCycle;
	}
	else if (loadTarget==HP48GX)
	{
		miniBF  = Hp49gMiniBitmapFamily;	// only applicable in DRY-RUN scenario
		statusBackdrop = option48GXBackdrop;
		statusPixelColor = &option48GXPixelColor;
		statusBackColor	= &option48GXBackColor;
		statusColorCycle = option48GXColorCycle;
	}
	else
	{
		miniBF  = Hp48sxMiniBitmapFamily;	// only applicable in DRY-RUN scenario
		statusBackdrop = option49GBackdrop;
		statusPixelColor = &option49GPixelColor;
		statusBackColor	= &option49GBackColor;
		statusColorCycle = option49GColorCycle;
	}
	
	
	if (statusPixelColor->g>248) statusPixelColor->g=248;   // Don't allow pure white pixel color
	    												 	// it interferes with rotated text algorithm.
	if (statusRUN)	// override miniBF if emulator is running
	{
		nextTarget=GetNextTarget(loadTarget); // set miniBF based on next available ROM target
		if (nextTarget==HP48SX)
			miniBF = Hp48sxMiniBitmapFamily;
		else if (nextTarget==HP48GX)
			miniBF = Hp48gxMiniBitmapFamily;
		else
			miniBF = Hp49gMiniBitmapFamily;
	}
	
	mainDisplayWindow=WinGetDisplayWindow();
	
	// Setup backStore window
	if (optionThreeTwentySquared)
	{
		backStoreWindow = OverwriteWindow(320,320,nativeFormat,&error,backStoreWindow);
	}
	else
	{
		if (statusLandscapeAvailable)
			backStoreWindow = OverwriteWindow(450,320,nativeFormat,&error,backStoreWindow);
		else
			backStoreWindow = OverwriteWindow(320,450,nativeFormat,&error,backStoreWindow);
	}
	if (error)
	{
		SysFatalAlert("Drastic Memory error. Please reset the device.");
	}
	
	
	WinSetDrawWindow(backStoreWindow);

	// Display graphic representation of HP48
	DrawHP48Graphic(loadTarget);
	
	LoadBackgroundImage(loadTarget);
	
	InitializePanelDisplay(true);

	CrossFade(backStoreWindow,mainDisplayWindow,0,0,false,((firstTime)?256:64));
	firstTime=false;
	
	UpdatePanelDisplay();
	
	Q->statusDisplayMode2x2=statusDisplayMode2x2;
	Q->statusPortraitViaLandscape=statusPortraitViaLandscape;
	Q->statusBackdrop=statusBackdrop;
	Q->statusColorCycle=statusColorCycle;
	
	WinSetDrawWindow(mainDisplayWindow);
	
	return errNone;
}



/***********************************************************************
 *
 * FUNCTION:    DisplayAboutScreen
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
void DisplayAboutScreen(void)
{
	WinHandle 		textScrollWindow;
	WinHandle 		buildWindow;
	WinHandle		buildWindowPVL;
	WinHandle		tileWindow;
	WinHandle		tileCopyWindow;
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;
	MemHandle 		tileHandle;
	BitmapPtr 		tileBitmap;
	Coord 			tileWidth,tileHeight;
	Err				error;
	int				i,j,h,w;
	Int16			penX,penY;
	Int16 			tmp;
	Boolean			penDown;
	Boolean			revertScreen=false;
	char 			*line,*test;
	int				numLines;
	UInt8			val;
	HRFontID 		font;
	Boolean 		center;
	RectangleType	drawRect,tileCopyRect;
	RectangleType 	vacatedRect,scrollRect,copyRect;
	Int16			length,position,height;
	Int16			offset=0;
	UInt32			nextTick;
	Boolean 		logoDisplay;
	DmResID			tileID;
	UInt16 			*srcPtr,*destPtr,*destPtrBase;
	RGBColorType	offWhiteRGB={0,240,240,240};
	
	if ((optionThreeTwentySquared)&&(!statusDisplayMode2x2))
	{
		toggle_lcd(); // put calc screen in 2x2 mode
		revertScreen=true;
	}
	else
		display_clear();
	
	if (optionTarget==HP48SX)
		tileID = Hp48sxTileBitmapFamily;
	else if (optionTarget==HP48GX)
		tileID = Hp48gxTileBitmapFamily;
	else
		tileID = Hp49gTileBitmapFamily;

	// get background tile for display
	tileHandle = DmGetResource(bitmapRsc, tileID);
	tileBitmap = MemHandleLock(tileHandle);
	BmpGetDimensions(tileBitmap,&tileWidth,&tileHeight,NULL);




	buildWindow = WinCreateOffscreenWindow(fullScreenExtentX+tileWidth,fullScreenExtentY,nativeFormat,&error);
	if (error)
	{
		goto dasexit;
	}
	if (statusPortraitViaLandscape)
		buildWindowPVL = WinCreateOffscreenWindow(fullScreenExtentY,fullScreenExtentX+tileWidth,nativeFormat,&error);

	if (error)
	{
		WinDeleteWindow(buildWindow,false);
		goto dasexit;
	}
	WinSetDrawWindow(buildWindow);
	for (h=0;h<fullScreenExtentY;h+=tileHeight)
		for (w=0;w<(fullScreenExtentX+tileWidth);w+=tileWidth)
			WinDrawBitmap(tileBitmap,w+(offset>>1),h);
	
	tileCopyWindow=buildWindow;
	
		// Make rotated copy 
	if (statusPortraitViaLandscape)
	{
		destPtrBase=BmpGetBits(WinGetBitmap(buildWindowPVL));
		destPtrBase+=(UInt32)fullScreenExtentY*(UInt32)(fullScreenExtentX+tileWidth-1);
		srcPtr=BmpGetBits(WinGetBitmap(buildWindow));
		for (h=0;h<fullScreenExtentY;h++)
		{
			destPtr=destPtrBase+h;
			for(w=0;w<(fullScreenExtentX+tileWidth);w++)
			{
				*destPtr=*srcPtr++;
				destPtr-=fullScreenExtentY;
			}
		}
		WinDeleteWindow(buildWindow,false);
		tileCopyWindow=buildWindowPVL;
	}
	
	if (statusPortraitViaLandscape)
		tileWindow = WinCreateOffscreenWindow(fullScreenExtentY,fullScreenExtentX,nativeFormat,&error);
	else 
		tileWindow = WinCreateOffscreenWindow(fullScreenExtentX,fullScreenExtentY,nativeFormat,&error);
	if (error)
	{
		WinDeleteWindow(tileCopyWindow,false);
		goto dasexit;
	}
	
	
	if (statusPortraitViaLandscape)
		textScrollWindow = WinCreateOffscreenWindow(fullScreenExtentY+30,fullScreenExtentX,nativeFormat,&error);
	else
		textScrollWindow = WinCreateOffscreenWindow(fullScreenExtentX,fullScreenExtentY+30,nativeFormat,&error);
	if (error)
	{
		WinDeleteWindow(tileCopyWindow,false);
		WinDeleteWindow(tileWindow,false);
		goto dasexit;
	}

	drawRect.topLeft.x = 0;
	drawRect.topLeft.y = 0;
	drawRect.extent.x = fullScreenExtentX;
	drawRect.extent.y = fullScreenExtentY+30;
	if (statusPortraitViaLandscape)
	{
		drawRect.extent.y = fullScreenExtentX;
		drawRect.extent.x = fullScreenExtentY+30;
	}
	WinSetForeColorRGB(&blackRGB,NULL);
	WinSetBackColorRGB(&blackRGB,NULL);
	WinSetDrawWindow(textScrollWindow);
	WinDrawRectangle(&drawRect,0);
	
	// get Title logo for display
	picHandle = DmGetResource(bitmapRsc, TitleLogoBitmapFamily);
	picBitmap = MemHandleLock(picHandle);
	FastDrawBitmap(picBitmap,14,18);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);

	line=(char*)aboutText;
	test=line;
	val=0;
	numLines=0;
	
	while(1)
	{
		val=*test++;
		if (val==0) numLines++;
		if (val=='Q')
			if (*test=='Q') break;
	}
	
	copyRect.topLeft.x = 0;
	copyRect.topLeft.y = 0;
	copyRect.extent.x = fullScreenExtentX;
	copyRect.extent.y = fullScreenExtentY;
	if (statusPortraitViaLandscape)
	{
		copyRect.extent.y = fullScreenExtentX;
		copyRect.extent.x = fullScreenExtentY;
	}
	
	tileCopyRect.topLeft.x = 0;
	tileCopyRect.topLeft.y = 0;
	tileCopyRect.extent.x = fullScreenExtentX;
	tileCopyRect.extent.y = fullScreenExtentY;
	if (statusPortraitViaLandscape)
	{
		tileCopyRect.extent.y = fullScreenExtentX;
		tileCopyRect.extent.x = fullScreenExtentY;
	}
		

	WinCopyRectangle(tileCopyWindow,tileWindow,&tileCopyRect,0,0,winPaint);	
	WinCopyRectangle(textScrollWindow,tileWindow,&copyRect,0,0,winOverlay);
	if (statusPortraitViaLandscape)
		WinCopyRectangle(tileWindow,mainDisplayWindow,&drawRect,fullScreenOriginY,screenHeight-fullScreenOriginX-fullScreenExtentX,winPaint);
	else
		WinCopyRectangle(tileWindow,mainDisplayWindow,&drawRect,fullScreenOriginX,fullScreenOriginY,winPaint);
	logoDisplay=true;
	
	nextTick=TimGetTicks();
	test=line;
	for (i=0;i<numLines;i++)
	{
		if (!logoDisplay)
		{
			while (*test++!=0) {}
			val=*test++;
			switch (val)
			{
				case 1:
					WinSetTextColorRGB(&offWhiteRGB,NULL);
					font=boldFont;
					center=true;
					break;
				case 2:
					WinSetTextColorRGB(&offWhiteRGB,NULL);
					font=largeBoldFont;
					center=true;
					break;
				case 3:
					WinSetTextColorRGB(&redRGB,NULL);
					font=largeBoldFont;
					center=true;
					break;
				// case 4:
				default:
					WinSetTextColorRGB(&offWhiteRGB,NULL);
					font=boldFont;
					center=false;
					break;
			}
			FntSetFont(font);
			height = (FntLineHeight()>>1);
			
			WinSetDrawWindow(textScrollWindow);
			if (line != NULL)
			{
				if (center)
				{
					length = (FntCharsWidth(line, StrLen(line))>>1);
					if (length<(fullScreenExtentX-3))
						position=(((fullScreenExtentX-2)-length)/2)+1;
					else
						position=1;
					DrawCharsExpanded(line,  StrLen(line), position, fullScreenExtentY, fullScreenExtentX-2,winPaint,SINGLE_DENSITY);
				}
				else
				{
					DrawCharsExpanded(line, StrLen(line), 1, fullScreenExtentY, fullScreenExtentX-2,winPaint,SINGLE_DENSITY);
				}
			}
			
			scrollRect.topLeft.x = 0;
			scrollRect.topLeft.y = 0;
			scrollRect.extent.x = fullScreenExtentX;
			scrollRect.extent.y = fullScreenExtentY+height+1;
			if (statusPortraitViaLandscape)
			{
				scrollRect.extent.y = fullScreenExtentX;
				scrollRect.extent.x = fullScreenExtentY+height+1;
			}
		}
		else
		{
			height=70;  // draw Title logo for the equivalent of 70 anim frames.
		}
		for (j=0;j<height;j++)
		{
			do 
			{
				EvtGetPen(&penX, &penY, &penDown);
				if (penDown)
				{
					do 
					{
						EvtGetPen(&penX, &penY, &penDown);
						if (statusPortraitViaLandscape)
						{
							tmp=penX;
							penX=(screenHeight>>1)-1-penY;
							penY=tmp;
						}
						if ((penX<(fullScreenOriginX>>1)) || (penX>((fullScreenOriginX+fullScreenExtentX)>>1)) ||
						    (penY<(fullScreenOriginY>>1)) || (penY>((fullScreenOriginY+fullScreenExtentY)>>1)))
						{
							EvtFlushPenQueue( );
							i=numLines;
							j=height;
							
						}
					} while (penDown);
				}
			} while (TimGetTicks()<nextTick);
			
			nextTick=TimGetTicks()+3;
			
			EvtFlushPenQueue( );
			
			WinCopyRectangle(textScrollWindow,tileWindow,&copyRect,0,0,winOverlay);	
			if (statusPortraitViaLandscape)
				WinCopyRectangle(tileWindow,mainDisplayWindow,&drawRect,fullScreenOriginY,screenHeight-fullScreenOriginX-fullScreenExtentX,winPaint);
			else
				WinCopyRectangle(tileWindow,mainDisplayWindow,&drawRect,fullScreenOriginX,fullScreenOriginY,winPaint);
			
			if (!logoDisplay)
			{
				WinSetDrawWindow(textScrollWindow);
				if (statusPortraitViaLandscape)
					WinScrollRectangle(&scrollRect, winLeft, 1, &vacatedRect);
				else
					WinScrollRectangle(&scrollRect, winUp, 1, &vacatedRect);
			}
			offset++;
			if (offset>(tileWidth<<1)) offset=0;
			if (statusPortraitViaLandscape)
				tileCopyRect.topLeft.y = (offset>>1);
			else
				tileCopyRect.topLeft.x = (offset>>1);
			WinCopyRectangle(tileCopyWindow,tileWindow,&tileCopyRect,0,0,winPaint);	
		}
		line=test;
		if (logoDisplay) logoDisplay=false;
	} 
	
	WinSetDrawWindow(mainDisplayWindow);
	WinSetTextColorRGB(&blackRGB,NULL);
	
	MemHandleUnlock(tileHandle);
	DmReleaseResource(tileHandle);
	
	WinDeleteWindow(textScrollWindow,false);
	WinDeleteWindow(tileCopyWindow,false);
	WinDeleteWindow(tileWindow,false);

dasexit:	
	if (revertScreen)
		toggle_lcd();
	else
	{
		display_clear(); // clear calcscreen	
		for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;
	}
}




/***********************************************************************
 *
 * FUNCTION:    DisplaySwitchGraphic
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
void DisplaySwitchGraphic(void)
{
	RectangleType	copyRect;
	Int16			penX,penY;
	UInt16			i=0;
	Boolean			penDown=true;
	Boolean			quitLoop = false;
	calcTarget		nextTarget;
	Err				error;
	drawInfo		*dInfo=NULL;
	Coord 			toWidth,toHeight;
	WinHandle		toWindow=NULL;
	Boolean			noAnimation=false;
	MemHandle 		armH;
	MemPtr    		armP;
	UInt32			processorType;
	UInt16			rowB;
	UInt32			position;
	Int32			fadeOffset;

	
	if (optionTarget==HP48SX)
	{
		if ((!status48GXROMLocation) && (!status49GROMLocation) && (statusRUN))
			return;		// 48GX/49G ROM not available, don't switch unless in dry run mode
	}
	else if (optionTarget==HP48GX)
	{
		if ((!status48SXROMLocation) && (!status49GROMLocation)  && (statusRUN))
			return;		// 48SX/49G ROM not available, don't switch unless in dry run mode
	}
	else
	{
		if ((!status48SXROMLocation) && (!status48GXROMLocation)  && (statusRUN))
			return;		// 48SX/48GX ROM not available, don't switch unless in dry run mode
	}
	
	nextTarget = GetNextTarget(optionTarget);
		
	copyRect.topLeft.x = 0;
	copyRect.topLeft.y = 0;
	copyRect.extent.x = 320;
	copyRect.extent.y = 150;  // only resave the LCD screen, the rest hasn't changed
	P48WinCopyRectangle(mainDisplayWindow,backStoreWindow,&copyRect,0,0,winPaint);
	
	// Setup the 'To' window for animated crossfade
	toWidth=320; 
	toHeight=320;
	if (!optionThreeTwentySquared)
	{
		if (statusLandscapeAvailable)
			toWidth=450; 
		else
			toHeight=450;
	}
	toWindow = WinCreateOffscreenWindow(toWidth,toHeight,nativeFormat,&error);
	
	if (error) noAnimation=true;
	
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
	if (processorType == sysFtrNumProcessorx86) noAnimation=true;

	if (!noAnimation)
	{
		WinSetDrawWindow(toWindow);
		DrawHP48Graphic(nextTarget);
		WinSetDrawWindow(mainDisplayWindow);
		
		dInfo = (drawInfo*) MemPtrNew(sizeof(drawInfo));
		
		dInfo->fromBasePtr=BmpGetBits(WinGetBitmap(backStoreWindow));
		dInfo->toBasePtr=BmpGetBits(WinGetBitmap(toWindow));
		dInfo->destBasePtr=BmpGetBits(WinGetBitmap(mainDisplayWindow));
		BmpGetDimensions(WinGetBitmap(mainDisplayWindow),NULL,NULL,&rowB);
		dInfo->destRowInt16s = rowB>>1;
		dInfo->toWidth = toWidth;
		dInfo->toHeight = toHeight;
		dInfo->offsetX = 0;
		dInfo->offsetY = 0;
		dInfo->effectType 	= TYPE_TWO;
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
		fadeOffset=-512;
		
		armH = DmGetResource('ARMC', 0x3090);
		armP = MemHandleLock(armH);
		
		do
		{
			dInfo->position=position;
			dInfo->fadeOffset=fadeOffset;
			position+=4;
			if (fadeOffset<512) {fadeOffset+=64;position-=2;}
			if (position>=180) position=0;
			PceNativeCall(armP, dInfo);
			
			EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
		} while (penDown);
		
		MemHandleUnlock(armH);
		DmReleaseResource(armH);
		MemPtrFree(dInfo);
	}
	
	if (toWindow!=NULL) WinDeleteWindow(toWindow,false);
	
	statusSwitchEmulation = 1;
	Q->statusQuit = true;              // stop the emulator so it can be restarted.
	statusNextTarget = GetNextTarget(optionTarget);
}







/***********************************************************************
 *
 * FUNCTION:    ErrorPrint
 *
 * DESCRIPTION: Print Error messages to HP48 simulation
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
void ErrorPrint( char* screenText, Boolean newLine )
{
	RectangleType scrollRect,vacatedRect;
	Boolean doNotIncrease=false;
	

	WinSetDrawWindow(mainDisplayWindow);
	// Set background color and text color to match HP48 screen
	WinSetBackColorRGB(&greyRGB,NULL);
	WinSetTextColorRGB(&blackRGB,NULL);
	WinSetForeColorRGB(&greyRGB,NULL);
	// Set the appropriate Font
	FntSetFont(stdFont);
	
	if (newLine)
	{
		if ((errorScreenY+lineDiff)>((((optionThreeTwentySquared)?errorPrint320ExtentY:errorPrintExtentY)+errorPrintOriginY)-lineDiff))
		{
			if (errorSkipOnce)
			{
				errorSkipOnce = 0;
			}
			else
			{
				// Scroll screen up 1 line
				scrollRect.topLeft.x = errorPrintOriginX;
				scrollRect.topLeft.y = errorPrintOriginY;
				scrollRect.extent.x = errorPrintExtentX;
				scrollRect.extent.y = errorPrintExtentY;
				if (statusPortraitViaLandscape)
				{
					swapRectPVL(&scrollRect,screenHeight);
					WinScrollRectangle(&scrollRect, winLeft, lineDiff, &vacatedRect);
				}
				else
				{
					WinScrollRectangle(&scrollRect, winUp, lineDiff, &vacatedRect);
				}
				WinDrawRectangle(&vacatedRect, 0);
			}
			doNotIncrease=true;
		}
	}
	
	if (screenText != NULL)	
	{
		DrawCharsExpanded(screenText, StrLen(screenText), errorPrintOriginX, errorScreenY, errorPrintExtentX,winPaint,SINGLE_DENSITY);
	}
	if (newLine)
		if (!doNotIncrease)
			errorScreenY=errorScreenY+lineDiff;	
}



/***********************************************************************
 *
 * FUNCTION:    ScreenPrint
 *
 * DESCRIPTION: Print info messages to HP48 screen simulation
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
void ScreenPrint( char* screenText, Boolean newLine )
{
	RectangleType scrollRect,vacatedRect;
	Boolean doNotIncrease=false;
	
	if (!optionVerbose) return;
	
	if ((optionThreeTwentySquared)&&(!statusDisplayMode2x2))
	{
		toggle_lcd(); // put calc screen in 2x2 mode
		statusDelayLCDToggle=true;
	}


	// Set background color and text color to match HP48 screen
	WinSetBackColorRGB(statusBackColor,NULL);
	WinSetForeColorRGB(statusBackColor,NULL);
	WinSetTextColorRGB(&textRGB,NULL);

	// Set the appropriate Font
	FntSetFont(stdFont);
	
	if (newLine)
	{
		if ((screenY+lineDiff)>(screenPrintExtentY-lineDiff))
		{
			if (skipOnce)
			{
				skipOnce = 0;
			}
			else
			{
				// Scroll screen up 1 line
				scrollRect.topLeft.x = screenPrintOriginX;
				scrollRect.topLeft.y = screenPrintOriginY;
				scrollRect.extent.x = screenPrintExtentX;
				scrollRect.extent.y = screenPrintExtentY;
				if (statusPortraitViaLandscape)
				{
					swapRectPVL(&scrollRect,screenHeight);
					WinScrollRectangle(&scrollRect, winLeft, lineDiff, &vacatedRect);
				}
				else
				{
					WinScrollRectangle(&scrollRect, winUp, lineDiff, &vacatedRect);
				}
				WinDrawRectangle(&vacatedRect, 0);
			}
			doNotIncrease=true;
		} 
	}
	
	if (screenText != NULL)
	{
		DrawCharsExpanded(screenText, StrLen(screenText), screenPrintOriginX, screenY, screenPrintExtentX,winPaint,SINGLE_DENSITY);
	}
	
	if (newLine)
		if (!doNotIncrease)
			screenY=screenY+lineDiff;
				
	if (newLine) 
	{
		if (optionScrollFast)
			SysTaskDelay(10);
		else
			SysTaskDelay(50);
	}
}


/***********************************************************************
 *
 * FUNCTION:    ScreenPrintReset
 *
 * DESCRIPTION: Print info messages to HP48 screen simulation
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
void ScreenPrintReset( )
{
	RectangleType screenRect;
	RGBColorType prevRGB;
	
	WinSetForeColorRGB(&backRGB,&prevRGB);
		
	screenRect.topLeft.x = screenPrintOriginX;
	screenRect.topLeft.y = screenPrintOriginY;
	screenRect.extent.x = screenPrintExtentX;
	screenRect.extent.y = screenPrintExtentY;
	
	WinDrawRectangle(&screenRect, 0);
	
	WinSetForeColorRGB(&prevRGB,NULL);
	
	screenY = lcdOriginY-14;
	skipOnce = 1;
}






/***********************************************************************
 *
 * FUNCTION:    AllocateRAM()
 *
 * DESCRIPTION: This function is responsible for allocating a 256 meg
 *              chunk of memory from the storage heap
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

static Boolean AllocateRAM(void)
{	
	void    *mem;
	UInt32  memSize;
	UInt16 	ftrNum;
	Err		error;
	
	ramBase=NULL;
	port1Base=NULL;
	port2Base=NULL;
	
	if (optionTarget==HP48SX)
	{
		ftrNum = ramMemSXFtrNum;
		memSize = RAM_SIZE_SX;
		status128KExpansion = option48SX128KExpansion;
	}
	else if (optionTarget==HP48GX)
	{
		ftrNum = ramMemGXFtrNum;
		memSize = RAM_SIZE_GX;
		status128KExpansion = option48GX128KExpansion;
	}
	else
	{
		ftrNum = ramMem49FtrNum;
		memSize = IRAM_SIZE_49+ERAM_SIZE_49;
		status128KExpansion = false;
	}
	
	if ((status128KExpansion) && (optionTarget!=HP49G))
		memSize += 0x40000;

	if (statusUseSystemHeap==ALL_IN_FTRMEM)
		error = FtrPtrNew(appFileCreator, ftrNum, memSize+16, &mem); // The extra 16 bytes is just a precautionary cushion
	else
		error = MemPtrNewLarge(memSize+16, &mem); // The extra 16 bytes is just a precautionary cushion

	if (error)
	{
		ScreenPrint("...Unable to allocate memory for RAM",true);
		OpenAlertConfirmDialog(alert,((status128KExpansion)?unableToAllocateRAMPORT1String:unableToAllocateRAMString),false);
		return false;
	}
	
	ramBase=mem;
	
	if (status128KExpansion)
	{
		if (optionTarget==HP48SX)
		{
			port1Base=ramBase+RAM_SIZE_SX;
		}
		else if (optionTarget==HP48GX)
		{
			port1Base=ramBase+RAM_SIZE_GX;
		}
	}
	if (optionTarget==HP49G)
	{
		port1Base=ramBase+IRAM_SIZE_49;
		port2Base=port1Base+(ERAM_SIZE_49/2);
	}
		

	return true;
}




/***********************************************************************
 *
 * FUNCTION:    AllocateRAM()
 *
 * DESCRIPTION: This function is responsible for allocating a 256 meg
 *              chunk of memory from the storage heap
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

static Boolean AllocateROM(void)
{	
	void    *mem;
	UInt32	romSize;
	UInt16 	ftrNum;
	Err		error;
	
	romBase=NULL;

	if (optionTarget==HP48SX)
	{
		romSize = ROM_SIZE_SX;
		ftrNum  = romMemSXFtrNum;
	}
	else if (optionTarget==HP48GX)
	{
		romSize = ROM_SIZE_GX;
		ftrNum  = romMemGXFtrNum;
	}
	else
	{
		romSize = ROM_SIZE_49;
		ftrNum  = romMem49FtrNum;
	}
	
	if (statusUseSystemHeap==ALL_IN_DHEAP)
		error = MemPtrNewLarge(romSize, &mem);
	else
		error = FtrPtrNew(appFileCreator, ftrNum, romSize, &mem);

	if (error)
	{
		ScreenPrint("...Unable to allocate memory for ROM",true);
		OpenAlertConfirmDialog(alert,unableToAllocateROMString,false);
		return false;
	}
	
	romBase = mem;
	
	return true;
}





/***********************************************************************
 *
 * FUNCTION:    CheckForRemount()
 *
 * DESCRIPTION: This function is responsible for checking if a newly 
 *              mounted volume is our storage volume for RAM / ROM
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
void CheckForRemount(void)
{
	UInt32		volIterator = vfsIteratorStart;
	FileRef		dirRef;
	Boolean		dirFound=false;
	Err			error;
	
	while (volIterator != vfsIteratorStop)
	{
		if (VFSVolumeEnumerate(&msVolRefNum, &volIterator)) return;
		error = VFSFileOpen(msVolRefNum,VFSROMRAMDirectory,vfsModeRead,&dirRef);
		if (!error) 
		{
			dirFound=true;
			break;
		}
	}
	VFSFileClose(dirRef);
	if (!dirFound) 
	{
		return;
	}
	statusVolumeMounted = true;	
}





/***********************************************************************
 *
 * FUNCTION:    CheckROMAvailability()
 *
 * DESCRIPTION: This function is responsible for checking for all ROMs 
 *              that this emulator supports.
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void CheckROMAvailability(void)
{
	UInt32		volIterator = vfsIteratorStart;
	FileRef		myRef,dirRef;
	Boolean		dirFound=false;
	Err			error;
	FileHand 	fileHandle;
	
	
	fileHandle = FileOpen(0, gxINTROMFileName, RomFileType, appFileCreator, fileModeReadOnly, &error);
	if (!error) 
	{
		status48GXROMLocation=ROM_INTERNAL;
		FileClose(fileHandle);
	}
	
	fileHandle = FileOpen(0, sxINTROMFileName, RomFileType, appFileCreator, fileModeReadOnly, &error);
	if (!error) 
	{
		status48SXROMLocation=ROM_INTERNAL;
		FileClose(fileHandle);
	}
	
	fileHandle = FileOpen(0, g49INTROMFileName, RomFileType, appFileCreator, fileModeReadOnly, &error);
	if (!error) 
	{
		status49GROMLocation=ROM_INTERNAL;
		FileClose(fileHandle);
	}
	
	// let's see if there is a VFS card available
	while (volIterator != vfsIteratorStop)
	{
		if (VFSVolumeEnumerate(&msVolRefNum, &volIterator)) return;
		error = VFSFileOpen(msVolRefNum,VFSROMRAMDirectory,vfsModeRead,&dirRef);
		if (!error) 
		{
			dirFound=true;
			break;
		}
	}
	VFSFileClose(dirRef);
	if (!dirFound) 
	{
		return;
	}
	statusVolumeMounted = true;
	
	if ((status48GXROMLocation) && (status48SXROMLocation) && (status49GROMLocation))
	{
		// All ROMS in internal memory, don't bother looking in VFS
		return;
	}
	
	if (!status48GXROMLocation)
	{
		error = VFSFileOpen(msVolRefNum,gxVFSROMFileName,vfsModeRead,&myRef);
		if (!error)
		{
			VFSFileClose(myRef);
			status48GXROMLocation=ROM_CARD;
		}
	}
	
	if (!status48SXROMLocation)
	{
		error = VFSFileOpen(msVolRefNum,sxVFSROMFileName,vfsModeRead,&myRef);
		if (!error)
		{
			VFSFileClose(myRef);
			status48SXROMLocation=ROM_CARD;
		}
	}
	
	if (!status49GROMLocation)
	{
		error = VFSFileOpen(msVolRefNum,g49VFSROMFileName,vfsModeRead,&myRef);
		if (!error)
		{
			VFSFileClose(myRef);
			status49GROMLocation=ROM_CARD;
		}
	}
}







/***********************************************************************
 *
 * FUNCTION:    LoadRAMFile()
 *
 * DESCRIPTION: This function is responsible for loading in the saved RAM
 *              file that contains a sanpshot of the calc RAM last time
 *              the application stopped.
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean LoadRAMFile(void)
{
	Err				error;
	UInt32			ramSize;
	UInt32			port1Size;
	Boolean			romSwap=false;
	UInt32			numRead;
	FileRef			myRef;
	UInt32			fileSize;
	UInt32			ramChecksum=0,cSum;
	UInt32			portChecksum=0;
	char 			tmp[40];
	FileHand 		fileHandle;
	
	// load main RAM image
	
	if (optionTarget==HP48SX)
	{
		ramSize = RAM_SIZE_SX;
		port1Size = PORT1_SIZE;
	}
	else if (optionTarget==HP48GX)
	{
		ramSize = RAM_SIZE_GX;
		port1Size = PORT1_SIZE;
	}
	else
	{
		ramSize = IRAM_SIZE_49;
		port1Size = ERAM_SIZE_49;
	}
		
	fileSize=(ramSize>>1);  // half the size in nibbles

	if (!statusInternalFiles)
	{
		// load RAM image from VFS card
		if (optionTarget==HP48SX)
			error = VFSFileOpen(msVolRefNum,sxVFSRAMFileName,vfsModeRead,&myRef);
		else if (optionTarget==HP48GX)
			error = VFSFileOpen(msVolRefNum,gxVFSRAMFileName,vfsModeRead,&myRef);
		else
			error = VFSFileOpen(msVolRefNum,g49VFSIRAMFileName,vfsModeRead,&myRef);

		if (!error)
		{
			// attempt to reload database
			ScreenPrint("...VFS RAM image found", true);
		}
		else
		{
			ScreenPrint("...Unable to find VFS RAM image", true);
			OpenAlertConfirmDialog(alert,unableToFindRAMFileString,false);
			return false;
		}
			
		ScreenPrint("...Loading RAM image from VFS", true);
		
		VFSFileRead(myRef,4,&magicNumberRAM,NULL);
		VFSFileRead(myRef,4,&ramChecksum,NULL);
		
		StrPrintF(tmp,"...File checksum = %lx",ramChecksum);
		ScreenPrint(tmp,true);

		if (statusUseSystemHeap==ALL_IN_FTRMEM)
			error=VFSFileReadData(myRef,fileSize,ramBase,fileSize,&numRead);
		else
			error=VFSFileRead(myRef,fileSize,(void*)((UInt32)ramBase+fileSize),&numRead);

		VFSFileClose(myRef);
		if (error)
		{
			ScreenPrint("...Error reading VFS RAM image", true);
			OpenAlertConfirmDialog(alert,errorReadingRAMString,false);
			return false;
		}
	}
	else
	{
		// load RAM image from internal memory
		if (optionTarget==HP48SX)
			fileHandle = FileOpen(0, sxINTRAMFileName, RamFileType, appFileCreator, fileModeReadOnly, &error);
		else if (optionTarget==HP48GX)
			fileHandle = FileOpen(0, gxINTRAMFileName, RamFileType, appFileCreator, fileModeReadOnly, &error);
		else
			fileHandle = FileOpen(0, g49INTIRAMFileName, RamFileType, appFileCreator, fileModeReadOnly, &error);

		if (!error)
		{
			// attempt to reload database
			ScreenPrint("...Internal RAM image found", true);
		}
		else
		{
			ScreenPrint("...Unable to find internal RAM image", true);
			OpenAlertConfirmDialog(alert,unableToFindRAMFileString,false);
			return false;
		}
			
		ScreenPrint("...Loading RAM image from internal memory", true);
		
		numRead = FileRead(fileHandle,&magicNumberRAM,1,4,&error);
		numRead = FileRead(fileHandle,&ramChecksum,1,4,&error);
		
		StrPrintF(tmp,"...File checksum = %lx",ramChecksum);
		ScreenPrint(tmp,true);

		if (statusUseSystemHeap==ALL_IN_FTRMEM)
			numRead = FileDmRead(fileHandle,ramBase,fileSize,1,fileSize,&error);
		else
			numRead = FileRead(fileHandle,(void*)((UInt32)ramBase+fileSize),1,fileSize,&error);


		FileClose(fileHandle);
		if (error)
		{
			ScreenPrint("...Error reading internal RAM image", true);
			OpenAlertConfirmDialog(alert,errorReadingRAMString,false);
			return false;
		}
	}
	
    ScreenPrint("...Calculating RAM image checksum...",true);
    
	cSum = ByteUtility(ramBase+fileSize,fileSize,CHECKSUM);
    
	StrPrintF(tmp,"...Checksum = %lx",cSum);
	ScreenPrint(tmp,true);

	if (cSum==ramChecksum)
	{
		ScreenPrint("...Checksums match.",true);
	}
	else
	{
		ScreenPrint("...Checksum mismatch - Image corrupt",true);
		OpenAlertConfirmDialog(alert,errorRAMImageCorruptString,false);
		return false;
	}
	
    ScreenPrint("...Expanding RAM image...",true);
	
	MemSemaphoreReserve(true);

	ByteUtility(ramBase,fileSize,UNCOMPRESS);

	MemSemaphoreRelease(true);

	ScreenPrint("...Expansion finished",true);
	
	magicNumberPORT=magicNumberRAM; // just in case we don't load a PORT file
	
	// load port1 image if needed
	if ((status128KExpansion) || (optionTarget==HP49G))
	{
		fileSize=(port1Size>>1);  // half the size in nibbles

		if (!statusInternalFiles)
		{
			// load PORT1 image from VFS card
			if (optionTarget==HP48SX)
				error = VFSFileOpen(msVolRefNum,sxVFSPORT1FileName,vfsModeRead,&myRef);
			else if (optionTarget==HP48GX)
				error = VFSFileOpen(msVolRefNum,gxVFSPORT1FileName,vfsModeRead,&myRef);
			else
				error = VFSFileOpen(msVolRefNum,g49VFSERAMFileName,vfsModeRead,&myRef);
					
			if (!error)
			{
				// attempt to reload database
				ScreenPrint("...VFS Port1 image found", true);
			}
			else
			{
				ScreenPrint("...Unable to find VFS Port1 image", true);
				OpenAlertConfirmDialog(alert,unableToFindPort1FileString,false);
				return false;
			}
					
			ScreenPrint("...Loading Port1 image from VFS", true);
			
			VFSFileRead(myRef,4,&magicNumberPORT,NULL);
			VFSFileRead(myRef,4,&portChecksum,NULL);
			
			StrPrintF(tmp,"...File checksum = %lx",portChecksum);
			ScreenPrint(tmp,true);

			if (statusUseSystemHeap==ALL_IN_FTRMEM)
				error=VFSFileReadData(myRef,fileSize,ramBase,ramSize+fileSize,&numRead);
			else
				error=VFSFileRead(myRef,fileSize,(void*)((UInt32)ramBase+ramSize+fileSize),&numRead);
			
			VFSFileClose(myRef);
			if (error)
			{
				ScreenPrint("...Error reading VFS Port1 image", true);
				OpenAlertConfirmDialog(alert,errorReadingPort1String,false);
				return false;
			}
		}
		else
		{
			// load PORT1 image from internal memory
			if (optionTarget==HP48SX)
				fileHandle = FileOpen(0, sxINTPORT1FileName, RamFileType, appFileCreator, fileModeReadOnly, &error);
			else if (optionTarget==HP48GX)
				fileHandle = FileOpen(0, gxINTPORT1FileName, RamFileType, appFileCreator, fileModeReadOnly, &error);
			else
				fileHandle = FileOpen(0, g49INTERAMFileName, RamFileType, appFileCreator, fileModeReadOnly, &error);
					
			if (!error)
			{
				// attempt to reload database
				ScreenPrint("...Internal Port1 image found", true);
			}
			else
			{
				ScreenPrint("...Unable to find internal Port1 image", true);
				OpenAlertConfirmDialog(alert,unableToFindPort1FileString,false);
				return false;
			}
					
			ScreenPrint("...Loading Port1 image from internal memory", true);
			
			numRead = FileRead(fileHandle,&magicNumberPORT,1,4,&error);
			numRead = FileRead(fileHandle,&portChecksum,1,4,&error);
			
			StrPrintF(tmp,"...File checksum = %lx",portChecksum);
			ScreenPrint(tmp,true);

			if (statusUseSystemHeap==ALL_IN_FTRMEM)
				numRead = FileDmRead(fileHandle,ramBase,ramSize+fileSize,1,fileSize,&error);
			else
				numRead = FileRead(fileHandle,(void*)((UInt32)ramBase+ramSize+fileSize),1,fileSize,&error);

			
			FileClose(fileHandle);
			if (error)
			{
				ScreenPrint("...Error reading internal Port1 image", true);
				OpenAlertConfirmDialog(alert,errorReadingPort1String,false);
				return false;
			}
		}
		
	    ScreenPrint("...Calculating Port1 image checksum...",true);
	    
		cSum = ByteUtility(ramBase+fileSize+ramSize,fileSize,CHECKSUM);
	    
		StrPrintF(tmp,"...Checksum = %lx",cSum);
		ScreenPrint(tmp,true);

		if (cSum==portChecksum)
		{
			ScreenPrint("...Checksums match.",true);
		}
		else
		{
			ScreenPrint("...Checksum mismatch - Image corrupt",true);
			OpenAlertConfirmDialog(alert,errorPORT1ImageCorruptString,false);
			return false;
		}

	    ScreenPrint("...Expanding Port1 image...",true);
		
		MemSemaphoreReserve(true);

		ByteUtility(port1Base,fileSize,UNCOMPRESS);

		MemSemaphoreRelease(true);

		ScreenPrint("...Expansion finished",true);
	}

	return true;
}







/***********************************************************************
 *
 * FUNCTION:    LoadROMFile()
 *
 * DESCRIPTION: This function is responsible for loading in a ROM database 
 *              file
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean LoadROMFile(void)
{
	Err				error;
	UInt32			romSize;
	UInt16 			ftrNum;
	Boolean			badHeader=false;
	unsigned char 	four[4];
	UInt32			numRead;
	FileRef			myRef;
	UInt32			fileSize;
	UInt32			addr;
	UInt32 			cSum=0,fileChecksum=0;
	char			tmp[40];
	FileHand 		fileHandle;
	
	if (optionTarget==HP48SX)
	{
		romSize = ROM_SIZE_SX;
		ftrNum  = romMemSXFtrNum;
	}
	else if (optionTarget==HP48GX)
	{
		romSize = ROM_SIZE_GX;
		ftrNum  = romMemGXFtrNum;
	}
	else
	{
		romSize = ROM_SIZE_49;
		ftrNum  = romMem49FtrNum;
	}
	
	fileSize=(romSize>>1);

	if (!statusInternalFiles)
	{
		// load ROM image from VFS card
		if (optionTarget==HP48SX)
			error = VFSFileOpen(msVolRefNum,sxVFSROMFileName,vfsModeRead,&myRef);
		else if (optionTarget==HP48GX)
			error = VFSFileOpen(msVolRefNum,gxVFSROMFileName,vfsModeRead,&myRef);
		else
			error = VFSFileOpen(msVolRefNum,g49VFSROMFileName,vfsModeRead,&myRef);
		
		
		if (!error)
		{
			// attempt to reload database
			ScreenPrint("...VFS ROM image found", true);
		}
		else
		{
			ScreenPrint("...Unable to find VFS ROM image", true);
			OpenAlertConfirmDialog(alert,unableToFindROMFileString,false);
			return false;
		}
					
		ScreenPrint("...Loading ROM from VFS", true);
		
		VFSFileRead(myRef,4,&fileChecksum,&numRead); // get checksum
		
		if (statusUseSystemHeap==ALL_IN_DHEAP)
			error=VFSFileRead(myRef,fileSize,(void*)((UInt32)romBase+fileSize),&numRead);
		else
			error=VFSFileReadData(myRef,fileSize,romBase,fileSize,&numRead);

		VFSFileClose(myRef);
		if (error)
		{
			ScreenPrint("...Error reading VFS ROM image", true);
			OpenAlertConfirmDialog(alert,errorReadingROMString,false);
			return false;
		}
	}
	else
	{
		// load ROM image from internal memory
		if (optionTarget==HP48SX)
			fileHandle = FileOpen(0, sxINTROMFileName, RomFileType, appFileCreator, fileModeReadOnly, &error);
		else if (optionTarget==HP48GX)
			fileHandle = FileOpen(0, gxINTROMFileName, RomFileType, appFileCreator, fileModeReadOnly, &error);
		else
			fileHandle = FileOpen(0, g49INTROMFileName, RomFileType, appFileCreator, fileModeReadOnly, &error);
		
		
		if (!error)
		{
			// attempt to reload database
			ScreenPrint("...Internal ROM image found", true);
		}
		else
		{
			ScreenPrint("...Unable to find internal ROM image", true);
			OpenAlertConfirmDialog(alert,unableToFindROMFileString,false);
			return false;
		}
			
		ScreenPrint("...Loading ROM from internal memory", true);
		
		numRead = FileRead(fileHandle,&fileChecksum,1,4,&error);

		if (statusUseSystemHeap==ALL_IN_DHEAP)
			numRead = FileRead(fileHandle,(void*)((UInt32)romBase+fileSize),1,fileSize,&error);
		else
			numRead = FileDmRead(fileHandle,romBase,fileSize,1,fileSize,&error);

		FileClose(fileHandle);
		if (error)
		{
			ScreenPrint("...Error reading internal ROM image", true);
			OpenAlertConfirmDialog(alert,errorReadingROMString,false);
			return false;
		}
	}
		
			
    ScreenPrint("...Calculating Checksum...",true);
    
	cSum = ByteUtility(romBase+fileSize,fileSize,CHECKSUM);
    
	StrPrintF(tmp,"...Checksum = %lx",cSum);
	ScreenPrint(tmp,true);
	StrPrintF(tmp,"...Header Checksum = %lx",fileChecksum);
	ScreenPrint(tmp,true);

	if (cSum!=fileChecksum)
	{
		ScreenPrint("...Invalid ROM checksum",true);
		OpenAlertConfirmDialog(alert,invalidROMChecksum,false);
		return false;
	}
	
	
	MemSemaphoreReserve(true);
	
	if (optionTarget==HP49G)
	{
		if ((romBase[fileSize] != 0x26)   || (romBase[fileSize+1] != 0x49) ||
			(romBase[fileSize+2] != 0x80) || (romBase[fileSize+3] != 0x10))
			badHeader=true;
	}
	else
	{
		if ((romBase[fileSize] != 0x32)   || (romBase[fileSize+1] != 0x96) ||
			(romBase[fileSize+2] != 0x1b) || (romBase[fileSize+3] != 0x80))
			badHeader=true;
	}
	
	if (badHeader)
	{
		MemSemaphoreRelease(true);
		ScreenPrint("...Invalid ROM file header",true);
		OpenAlertConfirmDialog(alert,invalidROMHeaderString,false);
		return false;
	}
	
	
    ScreenPrint("...ROM file header valid, expanding...",true);
		
	ByteUtility(romBase,fileSize,UNCOMPRESS);
		
	MemSemaphoreRelease(true);

	ScreenPrint("...Expansion finished",true);
		    
	ScreenPrint("Patching ROM for BEEP extension",true);

	four[0]=0x8;
	four[1]=0x1;
	four[2]=0xB;
	four[3]=0x1;
	if (optionTarget==HP49G)
		addr=0x4157A;
	else
		addr=0x017A6;
		
	if (statusUseSystemHeap==ALL_IN_DHEAP)
		MemMove(romBase+addr, four, 4);
	else
		DmWrite(romBase, addr, four, 4);
  
	ScreenPrint("ROM patched.",true);

	return true;
}






/***********************************************************************
 *
 * FUNCTION:    LoadState()
 *
 * DESCRIPTION: This function is responsible for loading the previous 
 *              saved state of the calculator.
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
static Boolean LoadState()
{
	Boolean stateError=false;
	FileHand fileHandle;
	Err	error;
	UInt32 stateChecksum=0;
	UInt32 cSum;
	char tmp[40];
	
	if (optionTarget==HP48SX)
		fileHandle = FileOpen(0, sxStateFileName, StateFileType, appFileCreator, fileModeReadOnly, &error);
	else if (optionTarget==HP48GX)
		fileHandle = FileOpen(0, gxStateFileName, StateFileType, appFileCreator, fileModeReadOnly, &error);
	else
		fileHandle = FileOpen(0, g49StateFileName, StateFileType, appFileCreator, fileModeReadOnly, &error);
	
	if (error) return false;
	
	FileRead(fileHandle,&magicNumberState,1,4,NULL);
	FileRead(fileHandle,&stateChecksum,1,4,NULL);
	
	StrPrintF(tmp,"...State file checksum = %lx",stateChecksum);
	ScreenPrint(tmp,true);

	if (FileRead(fileHandle,Q,1,sizeof(emulState),NULL) != sizeof(emulState) ) stateError = true;
	
	FileClose(fileHandle);
	
	if (Q->structureID != STRUCTURE_HEADER) stateError = true;
	
	if (stateError) 
	{
		ScreenPrint("...unable to read state file correctly", true);
		OpenAlertConfirmDialog(alert,unableToLoadStateFileString,false);
		return false;
	}
		
    ScreenPrint("...Calculating state image checksum...",true);
    
	cSum = ByteUtility((UInt8*)Q,sizeof(emulState),CHECKSUM);
    
	StrPrintF(tmp,"...Checksum = %lx",cSum);
	ScreenPrint(tmp,true);

	if (cSum==stateChecksum)
	{
		ScreenPrint("...Checksums match.",true);
	}
	else
	{
		ScreenPrint("...Checksum mismatch - Image corrupt",true);
		OpenAlertConfirmDialog(alert,errorStateImageCorruptString,false);
		return false;
	}
		
	return true;
}



/***********************************************************************
 *
 * FUNCTION:    SaveState()
 *
 * DESCRIPTION: This function is responsible for storing a snapshot of 
 *              the calculator state for quick return next time. 
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
static void SaveState(void)
{
	Boolean 		stateError=false;
	Err				error;
	FileHand		fileHandle;
	UInt32			stateChecksum=0;
	UInt16 			attributes;
	LocalID			id;
	char			tmp[40];
	
	ScreenPrint("Saving state data...",true);

	if (optionTarget==HP48SX)
		fileHandle = FileOpen(0, sxStateFileName, StateFileType, appFileCreator, fileModeReadWrite, &error);
	else if (optionTarget==HP48GX)
		fileHandle = FileOpen(0, gxStateFileName, StateFileType, appFileCreator, fileModeReadWrite, &error);
	else
		fileHandle = FileOpen(0, g49StateFileName, StateFileType, appFileCreator, fileModeReadWrite, &error);
    
	if (!error)
	{
		stateChecksum = ByteUtility((UInt8*)Q,sizeof(emulState),CHECKSUM);
		
		StrPrintF(tmp,"...Checksum = %lx",stateChecksum);
		ScreenPrint(tmp,true);

		FileWrite(fileHandle,&newMagicNumber,1,4,NULL);
		FileWrite(fileHandle,&stateChecksum,1,4,NULL);
		
		if (FileWrite(fileHandle,Q,1,sizeof(emulState),NULL) != sizeof(emulState) ) stateError = true;
	}
	
	if (stateError) 
	{
		OpenAlertConfirmDialog(alert,unableToSaveStateFileString,false);
	}
	
	FileClose(fileHandle);
	
	// set the backup bit on the state file
	if (optionTarget==HP48SX)
		id = DmFindDatabase(0, sxStateFileName);
	else if (optionTarget==HP48GX)
		id = DmFindDatabase(0, gxStateFileName);
	else
		id = DmFindDatabase(0, g49StateFileName);
	DmDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
				   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	attributes |= dmHdrAttrBackup;
	DmSetDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
					  NULL, NULL, NULL, NULL, NULL, NULL, NULL);		
	
	DrawProgressBar(10,ANY);
	ScreenPrint("...Finished",true);
}



/***********************************************************************
 *
 * FUNCTION:    SaveRAM()
 *
 * DESCRIPTION: This function is responsible for storing a snapshot of 
 *              calculator RAM for quick return next time.
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
Boolean SaveRAM(Boolean skipToPort1)
{
	Err				error;
	UInt32			ramSize;
	UInt32			volIterator = vfsIteratorStart;
	Boolean			dirFound=false;
	UInt32			fileSize;
	//UInt32			count;
	UInt8			*buf;
	//UInt8			*a,*b;
	FileRef			myRef;
	UInt32			numWritten;
	UInt32			ramChecksum=0;
	UInt32			portChecksum=0;
	VolumeInfoType	info;
	char 			tmp[40];
	FileHand 		fileHandle;
	UInt16 			attributes;
	LocalID			id;
	
	// save main RAM image
	if (optionTarget==HP48SX)
		ramSize = RAM_SIZE_SX;
	else if (optionTarget==HP48GX)
		ramSize = RAM_SIZE_GX;
	else
		ramSize = IRAM_SIZE_49;

	if (skipToPort1) goto saveport1;

	ScreenPrint("Saving RAM image...",true);

	fileSize=(ramSize>>1);

	buf=(UInt8*)Q->ram;

	MemSemaphoreReserve(true);
	
	ByteUtility(buf,ramSize,COMPRESS);
	
	MemSemaphoreRelease(true);
	
	DrawProgressBar(20,ANY);
	
	if (!statusInternalFiles)
	{	
		while (!statusVolumeMounted)
		{
			if (!OpenAlertConfirmDialog(confirm,retryAfterInsertString,false))
				return false;
		}
	}
	
retrysaveram:

	if (!statusInternalFiles)
	{	
		if (optionTarget==HP48SX)
		{
			VFSFileDelete(msVolRefNum,sxVFSRAMFileName);
			VFSFileCreate(msVolRefNum,sxVFSRAMFileName);
		}
		else if (optionTarget==HP48GX)
		{
			VFSFileDelete(msVolRefNum,gxVFSRAMFileName);
			VFSFileCreate(msVolRefNum,gxVFSRAMFileName);
		}
		else
		{
			VFSFileDelete(msVolRefNum,g49VFSIRAMFileName);
			VFSFileCreate(msVolRefNum,g49VFSIRAMFileName);
		}
		
		VFSVolumeInfo(msVolRefNum,&info);
		if (info.attributes&vfsVolumeAttrReadOnly)
		{
			if (OpenAlertConfirmDialog(confirm,volumeProtectedString,false))
				goto retrysaveram;
			else
				return false;
		}
	}
	
	if (!statusInternalFiles)
	{
		// save RAM image to VFS card
		if (optionTarget==HP48SX)
			error = VFSFileOpen(msVolRefNum,sxVFSRAMFileName,vfsModeWrite,&myRef);
		else if (optionTarget==HP48GX)
			error = VFSFileOpen(msVolRefNum,gxVFSRAMFileName,vfsModeWrite,&myRef);
		else
			error = VFSFileOpen(msVolRefNum,g49VFSIRAMFileName,vfsModeWrite,&myRef);
	    
	    if (error)
	    {
	    	OpenAlertConfirmDialog(alert,unableToSaveRAMImageString,false);
	    }
	    else
	    {
			ramChecksum = ByteUtility(&buf[fileSize],fileSize,CHECKSUM);

			StrPrintF(tmp,"...Checksum = %lx",ramChecksum);
			ScreenPrint(tmp,true);

			VFSFileWrite(myRef,4,&newMagicNumber,NULL);
			VFSFileWrite(myRef,4,&ramChecksum,NULL);
			
			VFSFileWrite(myRef,fileSize,&buf[fileSize],&numWritten);
			VFSFileClose(myRef);
			
			if (numWritten!=fileSize)
			{
				if (OpenAlertConfirmDialog(confirm,savedRAMFileInvalidString,false))
					goto retrysaveram;
			}
		}
	}
	else
	{
		// save RAM image to internal memory
		if (optionTarget==HP48SX)
			fileHandle = FileOpen(0, sxINTRAMFileName, RamFileType, appFileCreator, fileModeReadWrite, &error);
		else if (optionTarget==HP48GX)
			fileHandle = FileOpen(0, gxINTRAMFileName, RamFileType, appFileCreator, fileModeReadWrite, &error);
		else
			fileHandle = FileOpen(0, g49INTIRAMFileName, RamFileType, appFileCreator, fileModeReadWrite, &error);
	    
	    if (error)
	    {
	    	OpenAlertConfirmDialog(alert,unableToSaveRAMImageString,false);
	    }
	    else
	    {
			ramChecksum = ByteUtility(&buf[fileSize],fileSize,CHECKSUM);

			StrPrintF(tmp,"...Checksum = %lx",ramChecksum);
			ScreenPrint(tmp,true);

			FileWrite(fileHandle,&newMagicNumber,1,4,NULL);
			FileWrite(fileHandle,&ramChecksum,1,4,NULL);
			
			numWritten = FileWrite(fileHandle,&buf[fileSize],1,fileSize,NULL);
			
			FileClose(fileHandle);
			
			if (numWritten!=fileSize)
			{
				if (OpenAlertConfirmDialog(confirm,savedRAMFileInvalidString,false))
					goto retrysaveram;
			}
			
			// set the backup bit on the state file
			if (optionTarget==HP48SX)
				id = DmFindDatabase(0, sxINTRAMFileName);
			else if (optionTarget==HP48GX)
				id = DmFindDatabase(0, gxINTRAMFileName);
			else
				id = DmFindDatabase(0, g49INTIRAMFileName);
			DmDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
						   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			attributes |= dmHdrAttrBackup;
			DmSetDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
							  NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		}
	}
	DrawProgressBar(20,ANY);
	ScreenPrint("...Finished.",true);


	// save Port1 image if necessary
	if ((status128KExpansion) || (optionTarget==HP49G))
	{
saveport1:	
		
		ScreenPrint("Saving PORT image...",true);
		
		if (optionTarget==HP49G)
			ramSize = ERAM_SIZE_49;
		else
			ramSize = PORT1_SIZE;
	
		fileSize=(ramSize>>1);	

		buf=(UInt8*)Q->port1;

		MemSemaphoreReserve(true);
		
		ByteUtility(buf,ramSize,COMPRESS);
		
		MemSemaphoreRelease(true);
		
retrysaveport1:

		if (!statusInternalFiles)
		{	
			// save PORT1 image to VFS card
			if (optionTarget==HP48SX)
			{
				VFSFileDelete(msVolRefNum,sxVFSPORT1FileName);
				VFSFileCreate(msVolRefNum,sxVFSPORT1FileName);
			}
			else if (optionTarget==HP48GX)
			{
				VFSFileDelete(msVolRefNum,gxVFSPORT1FileName);
				VFSFileCreate(msVolRefNum,gxVFSPORT1FileName);
			}
			else
			{
				VFSFileDelete(msVolRefNum,g49VFSERAMFileName);
				VFSFileCreate(msVolRefNum,g49VFSERAMFileName);
			}
			
			if (optionTarget==HP48SX)
				error = VFSFileOpen(msVolRefNum,sxVFSPORT1FileName,vfsModeWrite,&myRef);
			else if (optionTarget==HP48GX)
				error = VFSFileOpen(msVolRefNum,gxVFSPORT1FileName,vfsModeWrite,&myRef);
			else
				error = VFSFileOpen(msVolRefNum,g49VFSERAMFileName,vfsModeWrite,&myRef);
		    
		    if (error)
		    {
		    	OpenAlertConfirmDialog(alert,unableToSavePORT1ImageString,false);
		    }
		    else
		    {
				portChecksum = ByteUtility(&buf[fileSize],fileSize,CHECKSUM);
				
				StrPrintF(tmp,"...Checksum = %lx",portChecksum);
				ScreenPrint(tmp,true);

				VFSFileWrite(myRef,4,&newMagicNumber,NULL);
				VFSFileWrite(myRef,4,&portChecksum,NULL);
				
				VFSFileWrite(myRef,fileSize,&buf[fileSize],&numWritten);
				VFSFileClose(myRef);
				if (numWritten!=fileSize)
				{
					if (OpenAlertConfirmDialog(confirm,savedPORT1FileInvalidString,false))
						goto retrysaveport1;
				}
			}
		}
		else
		{
			// save PORT1 image to VFS card
			if (optionTarget==HP48SX)
				fileHandle = FileOpen(0, sxINTPORT1FileName, RamFileType, appFileCreator, fileModeReadWrite, &error);
			else if (optionTarget==HP48GX)
				fileHandle = FileOpen(0, gxINTPORT1FileName, RamFileType, appFileCreator, fileModeReadWrite, &error);
			else
				fileHandle = FileOpen(0, g49INTERAMFileName, RamFileType, appFileCreator, fileModeReadWrite, &error);
		    
		    if (error)
		    {
		    	OpenAlertConfirmDialog(alert,unableToSavePORT1ImageString,false);
		    }
		    else
		    {
				portChecksum = ByteUtility(&buf[fileSize],fileSize,CHECKSUM);
				
				StrPrintF(tmp,"...Checksum = %lx",portChecksum);
				ScreenPrint(tmp,true);

				FileWrite(fileHandle,&newMagicNumber,1,4,NULL);
				FileWrite(fileHandle,&portChecksum,1,4,NULL);
				
				numWritten = FileWrite(fileHandle,&buf[fileSize],1,fileSize,NULL);
				
				FileClose(fileHandle);
				
				if (numWritten!=fileSize)
				{
					if (OpenAlertConfirmDialog(confirm,savedPORT1FileInvalidString,false))
						goto retrysaveport1;
				}
			
				// set the backup bit on the state file
				if (optionTarget==HP48SX)
					id = DmFindDatabase(0, sxINTPORT1FileName);
				else if (optionTarget==HP48GX)
					id = DmFindDatabase(0, gxINTPORT1FileName);
				else
					id = DmFindDatabase(0, g49INTERAMFileName);
				DmDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
							   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
				attributes |= dmHdrAttrBackup;
				DmSetDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
								  NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			}
		}		
		ScreenPrint("...Finished.",true);
	}
	if (!skipToPort1) DrawProgressBar(40,ANY);
	return true;
}



/***********************************************************************
 *
 * FUNCTION:    CleanUpRAM()
 *
 * DESCRIPTION: This function is responsible for clearing all allocated pointers
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void CleanUpRAM(void)
{
	if (statusUseSystemHeap==ALL_IN_DHEAP)
	{
		if (romBase!=NULL) MemPtrFree(romBase);
	}

	if ((statusUseSystemHeap==ALL_IN_DHEAP) || (statusUseSystemHeap==ROM_IN_FTRMEM))
	{
		if (ramBase!=NULL) MemPtrFree(ramBase);
	}

	if ((statusUseSystemHeap==ALL_IN_FTRMEM) || (statusUseSystemHeap==ROM_IN_FTRMEM))
	{
		FtrPtrFree(appFileCreator, romMemSXFtrNum);
		FtrPtrFree(appFileCreator, ramMemSXFtrNum);
		FtrPtrFree(appFileCreator, romMemGXFtrNum);
		FtrPtrFree(appFileCreator, ramMemGXFtrNum);
		FtrPtrFree(appFileCreator, romMem49FtrNum);
		FtrPtrFree(appFileCreator, ramMem49FtrNum);
	}
		
	DrawProgressBar(10,ANY);
}




/***********************************************************************
 *
 * FUNCTION:    PinResetEmulator()
 *
 * DESCRIPTION: This function is responsible for clearing the calculator 
 *              state variables to simulate a press of the reset button.
 *
 ***********************************************************************/
 
void PinResetEmulator(void)
{
	int i;
	
	// set rom bank to correct bank
	if (optionTarget==HP49G)
	{
		Q->flashBankHi=0;
		Q->flashBankLo=0;
		Q->flashRomHi=(UInt8*)ByteSwap32(ByteSwap32(Q->flashAddr[Q->flashBankHi])-0x40000);
		Q->flashRomLo=Q->flashAddr[Q->flashBankLo];
	}
	
	// set pc to 0
	Q->statusPerformReset=true;
		
	Q->rstkp=0;
	for (i=0;i<8;i++) Q->rstk[i]=0;
	Q->HST=0;
	Q->SHUTDN=0x0100;
	Q->INTERRUPT_ENABLED=0x0100;
	Q->INTERRUPT_PENDING=0;
	Q->INTERRUPT_PROCESSING=0;
	Q->crc=0;
	
	// reset IO
	for (i=0;i<64;i++)  Q->ioram[i]=0;
	
	// reset MMU
	Q->mcIOAddrConfigured=false;
	Q->mcIOBase=0x00001000;
	Q->mcIOCeil=0;

	Q->mcRAMSizeConfigured=false;
	Q->mcRAMAddrConfigured=false;
	Q->mcRAMBase=0x100000;
	Q->mcRAMCeil=0;
	Q->mcRAMSize=0;
	Q->mcRAMDelta=0;

	Q->mcCE1SizeConfigured=false;
	Q->mcCE1AddrConfigured=false;
	Q->mcCE1Base=0x00001000;
	Q->mcCE1Ceil=0;
	Q->mcCE1Size=0;
	Q->mcCE1Delta=0;

	Q->mcCE2SizeConfigured=false; 
	Q->mcCE2AddrConfigured=false;
	Q->mcCE2Base=0x00001000;
	Q->mcCE2Ceil=0;
	Q->mcCE2Size=0;
	Q->mcCE2Delta=0;

	Q->mcNCE3SizeConfigured=false;
	Q->mcNCE3AddrConfigured=false;
	Q->mcNCE3Base=0x00001000;
	Q->mcNCE3Ceil=0;
	Q->mcNCE3Size=0;
	Q->mcNCE3Delta=0;
	
	Q->t1=0;
	Q->t2=0;
	Q->lcdLineCount=0;
	Q->lcdByteOffset=0;
	Q->lcdContrastLevel=0;
	Q->lcdPixelOffset=0;
	Q->IO_TIMER_RUN=(Q->ioram[0x2f]&1);
	
	Q->instCount=0;
	Q->prevTickStamp = ByteSwap32(TimGetTicks());
}




/***********************************************************************
 *
 * FUNCTION:    ClearCalculator()
 *
 * DESCRIPTION: This function is responsible for clearing the calculator 
 *              RAM and state variables to simulate a factory default
 *              condition.
 *
 ***********************************************************************/
 
static void ClearCalculator(void)
{
	Int16			i;
	Int32           j;
	UInt32 			ramSize;
	void            *dummy=NULL;

	if (optionTarget==HP48SX)
		ramSize = RAM_SIZE_SX;
	else if (optionTarget==HP48GX)
		ramSize = RAM_SIZE_GX;
	else
		ramSize = IRAM_SIZE_49+ERAM_SIZE_49;
		
	if (status128KExpansion)
		ramSize += PORT1_SIZE;
		
	ScreenPrint("...clearing RAM",true); 

	if (statusUseSystemHeap!=ALL_IN_FTRMEM)
	{
		MemSet(ramBase,ramSize,0);
	}
	else
	{
		dummy=MemPtrNew(0x8000);
		if (dummy!=0)
		{
			MemSet(dummy,0x8000,0);
			for (j=0;j<ramSize;j+=0x8000)
				DmWrite(ramBase,j,dummy,0x8000);
			MemPtrFree(dummy);
		}
	}
	
	ScreenPrint("...clearing state information",true);

	MemSet(Q,sizeof(emulState),0);
		
	Q->MODE=16;
	Q->mcIOBase=0x100000;
	Q->mcRAMBase=0x100000;
	Q->mcCE1Base=0x100000;
	Q->mcCE2Base=0x100000;
	Q->mcNCE3Base=0x100000;
	for (i=0;i<16;i++)
	{
		Q->F_s[i]=F_s_init[i];
		Q->F_l[i]=F_l_init[i];
	}
	Q->delayKey=noKey;
	Q->delayKey2=noKey;
}







/***********************************************************************
 *
 * FUNCTION:    LoadEmulator()
 *
 * DESCRIPTION: This function is responsible for loading up the RAM and
 *              ROM files and starting the main emulator loop.
 *
 * PARAMETERS:  
 *
 * RETURNED:   
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

static Err LoadEmulator(void) 
{
	Int16 		i;
	UInt32 processorType;
	UInt32 normalExit;
	
	// Okay, this is where it all goes down...
	
	// Note that Q is not stable for writing to until after the point labelled "Q STABLE FROM HERE ON..."
	
	// First, check which ROMs are available
	CheckROMAvailability();
	
	// if none are available, display the main screen and go to dry-run mode
	if ((!status48SXROMLocation)&&(!status48GXROMLocation)&&(!status49GROMLocation))
	{
		if (!statusSkipScreenDraw) 
		{
			CheckDisplayTypeForGUIRedraw();
			DisplayHP48Screen(optionTarget);
		}
		statusSkipScreenDraw=false;
		OpenAlertConfirmDialog(alert,noROMFilesString,false);

		return errUnableToFindROMFiles;
	}
	else
	// otherwise, deal with the case where the last target run no longer has a ROM image and just move to the next
	{
		if ((optionTarget==HP48SX)&&(!status48SXROMLocation))
		{
			if (status48GXROMLocation)
				optionTarget=HP48GX;       // 48SX ROM not available, start 48GX instead
			else
				optionTarget=HP49G;        // 48GX ROM not available, start 49G instead
		}
		else if ((optionTarget==HP48GX)&&(!status48GXROMLocation))
		{
			if (status49GROMLocation)
				optionTarget=HP49G;       // 48GX ROM not available, start 49G instead
			else
				optionTarget=HP48SX;       // 49G ROM not available, start 48SX instead
		}
		else if ((optionTarget==HP49G)&&(!status49GROMLocation))
		{
			if (status48SXROMLocation)
				optionTarget=HP48SX;       // 49G ROM not available, start 48SX instead
			else
				optionTarget=HP48GX;       // 48SX ROM not available, start 48GX instead
		}
	}
	
	if (optionTarget==HP48SX)
		statusInternalFiles = (status48SXROMLocation==ROM_INTERNAL);
	else if (optionTarget==HP48GX)
		statusInternalFiles = (status48GXROMLocation==ROM_INTERNAL);
	else
		statusInternalFiles = (status49GROMLocation==ROM_INTERNAL);

	// Draw the main screen for the appropriate target, unless this is a target-switch scenario,
	// in which case the screen will already have been drawn			
	if (!statusSkipScreenDraw) 
	{
		CheckDisplayTypeForGUIRedraw();
		DisplayHP48Screen(optionTarget);
		DrawProgressBar(0,STARTUP);
	}
		

	ScreenPrint("Emulator startup commencing...",true);
	
	DrawProgressBar(10,ANY);

	
	// Now load up the previously saved state file
	// we need the magic number to verify against the RAM and ROM images
	if (!statusCompleteReset)
	{
		ScreenPrint("Loading calculator state file...",true);
		if (LoadState())
		{
			ScreenPrint("     SUCCEEDED",true);
		}
		else
		{
			ScreenPrint("     FAILED",true);
			statusCompleteReset=true;
		}
		ScreenPrint(NULL,true);
	}
	
	DrawProgressBar(10,ANY);
	
	
	// Allocate memory for RAM/port 1 images
	ScreenPrint("Allocating mem for RAM/port1...",true);
	if (AllocateRAM())
	{
		ScreenPrint("     SUCCEEDED",true);
	}
	else
	{
		ScreenPrint("     FAILED",true);
		CleanUpRAM();
		statusSkipScreenDraw=false;
		return errUnableToAllocateRAM;
	}
	ScreenPrint(NULL,true);
	
	DrawProgressBar(10,ANY);
	
		
	// Now load up the previously saved RAM images and get their magic numbers		
	if (!statusCompleteReset)
	{			
		ScreenPrint("Loading RAM and Port1 images from saved files...",true);
		if (LoadRAMFile())
		{
			ScreenPrint("     SUCCEEDED",true);
		}
		else
		{
			ScreenPrint("     FAILED",true);
			statusCompleteReset=true;
		}
		ScreenPrint(NULL,true);
	}
	
	DrawProgressBar(20,ANY);
	

	// Allocate memory for ROM image
	ScreenPrint("Allocating mem for ROM...",true);
	if (AllocateROM())
	{
		ScreenPrint("     SUCCEEDED",true);
	}
	else
	{
		ScreenPrint("     FAILED",true);
		CleanUpRAM();
		statusSkipScreenDraw=false;
		return errUnableToAllocateROM;
	}
	ScreenPrint(NULL,true);
	
	DrawProgressBar(10,ANY);


	// Now load up the ROM image		
	ScreenPrint("Loading ROM from saved ROM File...",true);
	if (LoadROMFile())
	{
		ScreenPrint("     SUCCEEDED",true);
	}
	else
	{
		ScreenPrint("     FAILED",true);
		CleanUpRAM();
		statusSkipScreenDraw=false;
		return errUnableToLoadROMFile;
	}
	ScreenPrint(NULL,true);
	
	DrawProgressBar(20,ANY);

	

	// if the statusCompleteReset flag has been set then erase all state and RAM data
	if (statusCompleteReset)
	{
		ScreenPrint("Resetting all calculator data...",true);
		ClearCalculator();
		ScreenPrint("     FINISHED",true);
		ScreenPrint(NULL,true);
		statusCompleteReset=false;
	}
	else
	// Check magic numbers - if not all equal then ask user if they'd like to do reset...
	{
		if ((magicNumberState!=magicNumberRAM) || (magicNumberRAM!=magicNumberPORT))
		{
			if (OpenAlertConfirmDialog(confirm,badMagicString,false))
			{
				if (OpenAlertConfirmDialog(confirm,resetAlertString,false))
				{
					ScreenPrint("Resetting all calculator data due to corrupted files...",true);
					ClearCalculator();
					ScreenPrint("     FINISHED",true);
					ScreenPrint(NULL,true);
					statusCompleteReset=false;
				}
				else
				{
					OpenAlertConfirmDialog(alert,corruptionWarningString,false);
				}
			}
			else
			{
				OpenAlertConfirmDialog(alert,corruptionWarningString,false);
			}
		}
	}
		
	DrawProgressBar(10,ANY);

	
// Q STABLE FROM HERE ON...
// Q STABLE FROM HERE ON...
// Q STABLE FROM HERE ON...


	// Q has just been completely overwritten, so rewrite some of the Q members.
	Q->F_s[0]=Q->P;
	Q->F_l[1]=Q->P+1;
	Q->statusQuit=0;
	Q->structureID = STRUCTURE_HEADER;
	Q->structureVersion = STRUCTURE_VERSION;
	
	Q->statusDisplayMode2x2=statusDisplayMode2x2;
	Q->statusPortraitViaLandscape=statusPortraitViaLandscape;
	Q->statusBackdrop=statusBackdrop;
	Q->statusColorCycle=statusColorCycle;
	
	Q->backgroundImagePtr1x1=BmpGetBits(WinGetBitmap(backdrop1x1));
	Q->backgroundImagePtr2x2=BmpGetBits(WinGetBitmap(backdrop2x2));
	Q->backgroundImagePtr1x1=(UInt16*)ByteSwap32(Q->backgroundImagePtr1x1);
	Q->backgroundImagePtr2x2=(UInt32*)ByteSwap32(Q->backgroundImagePtr2x2);

	if (statusDelayLCDToggle)
	{
		Q->delayEvent=50;  // wait 50 keyboard ticks before triggering a LCD zoom.
		Q->delayKey=virtKeyVideoSwap;
		statusDelayLCDToggle=false;
	}
	  
	// setup peripheral card status flag
	Q->CARDSTATUS=0;
	
	if (status128KExpansion)
	{
		if (optionTarget==HP48SX)
			Q->CARDSTATUS=0x5;
		else if (optionTarget==HP48GX)
			Q->CARDSTATUS=0xa;
	}		
	if (optionTarget==HP49G)
		Q->CARDSTATUS=0xF;
	
	
	// setup MMU information
	Q->ram=ramBase;
	Q->rom=romBase;
	Q->port1=port1Base;
	Q->port2=port2Base;

	if (optionTarget==HP48SX)
	{
		Q->ce1addr=Q->port1;
		Q->ce2addr=Q->port2;
		Q->nce3addr=NULL;
	}
	else
	{
		Q->ce1addr=NULL;
		Q->ce2addr=Q->port1;
		Q->nce3addr=Q->port2;
	}
	if (Q->mcRAMAddrConfigured) Q->mcRAMDelta = (long)Q->ram-Q->mcRAMBase;
	if (Q->mcCE1AddrConfigured) Q->mcCE1Delta = (long)Q->ce1addr-Q->mcCE1Base;
	if (Q->mcCE2AddrConfigured) Q->mcCE2Delta = (long)Q->ce2addr-Q->mcCE2Base;
	if (Q->mcNCE3AddrConfigured) Q->mcNCE3Delta = (long)Q->nce3addr-Q->mcNCE3Base;
	
	if (optionTarget==HP49G)
	{
		for (i=0;i<8;i++)
			Q->flashAddr[i]=Q->rom+((long)i*0x40000);
		
		for (i=8;i<16;i++)
			Q->flashAddr[i]=Q->rom+0x1ffd00+((long)(i-8)*0x20);
			
		Q->flashRomHi=Q->flashAddr[Q->flashBankHi]-0x40000;  // Subtract 0x40000 here so we don't have to later
		Q->flashRomLo=Q->flashAddr[Q->flashBankLo];
		
		Q->status49GActive=true;
	}
	else
		Q->status49GActive=false;
	
		
	DrawProgressBar(10,ANY);
	
	// Initialize the LCD driver
	ScreenPrint("Initializing LCD...",true);
	init_lcd();
	ScreenPrint("     FINISHED",true);
	ScreenPrint(NULL,true);
	
		
	DrawProgressBar(5,ANY);

	display_clear();
    
	Q->statusPerformReset=false;
	statusRUN=true;
	
	// get the processor type
	FtrGet(sysFileCSystem, sysFtrNumProcessorID, &processorType);
		
	if (processorType == sysFtrNumProcessorx86)
	{
		optionVerbose=true;
		
		ScreenPrint("Power48 will not load the core",true);
		ScreenPrint("when run on the simulator.", true);
		ScreenPrint("The emulator will return to", true);
		ScreenPrint("Dry run mode in five seconds", true);
		SysTaskDelay(500);
	}
	else
	{	
		// Start the Emulator
		MemSemaphoreReserve(true);

		normalExit=emulate();
		
		MemSemaphoreRelease(true);
	}
	
	statusRUN=false;

	// Emulator is quitting, save state information
	exit_lcd();
	
	if (!normalExit)
	{
		OpenAlertConfirmDialog(alert,coreProblemString,false);
	}
	
	if (statusSwitchEmulation)
	{
		CheckDisplayTypeForGUIRedraw();
		DisplayHP48Screen(statusNextTarget);
		statusSkipScreenDraw=true;
		DrawProgressBar(0,SWITCH);
	}				
	else
	{
		statusSkipScreenDraw=false;
		DrawProgressBar(0,SHUTDOWN);
	}
	ScreenPrint(NULL,true);
	ScreenPrint(NULL,true);
	ScreenPrint("Emulator core shutdown.",true);

	newMagicNumber = SysRandom(TimGetTicks());
	if (SaveRAM(false))     // ensure that state is saved only if matching RAM is saved
		SaveState();   // Prevents corruption if user elects to not save RAM to VFS
		
	CleanUpRAM();
	ScreenPrint("RAM deallocated.",true);

	EvtFlushPenQueue( );

	ScreenPrint("END",true);
	for (i=0;i<5;i++) ScreenPrint(NULL,true);
	
	if (processorType == sysFtrNumProcessorx86)
	{
		return errOnSimulator;
	}
  	return errNone;
}






/***********************************************************************
 *
 * FUNCTION:    DryEmulate()
 *
 * DESCRIPTION: This function is bringing up the shell of the emulator
 *              without actually running it. It provides an event loop
 *              so the user can alter preferences.
 *
 ***********************************************************************/

static void DryEmulate(void) 
{
	
restartDEloop:
	statusRUN=false;
	Q->statusQuit=false;
	statusSwitchEmulation=false;
	
	Q->status49GActive=(optionTarget==HP49G);
	
	display_clear();
	while (!Q->statusQuit)
	{
		WinSetForeColorRGB(statusPixelColor, NULL);
		WinSetTextColorRGB(statusPixelColor, NULL);
		WinSetBackColorRGB(statusBackColor, NULL);
		WinSetDrawMode(winOverlay);
		FntSetFont(boldFont);
		if (statusDisplayMode2x2)
			DrawCharsExpanded("Dry run mode", 12, fullScreenOriginX+66, fullScreenOriginY+56, 0, winOverlay,DOUBLE_DENSITY);
		else
			DrawCharsExpanded("Dry run mode", 12, lcd1x1FSOriginX+1, lcd1x1FSOriginY+19, 0, winOverlay,DOUBLE_DENSITY);
		ProcessPalmEvent(evtWaitForever);
	}

	if (statusSwitchEmulation)
	{
		Q->statusQuit=false;
		if (optionTarget==HP48SX)
			optionTarget=HP48GX;
		else if (optionTarget==HP48GX)
			optionTarget=HP49G;
		else
			optionTarget=HP48SX;
		CheckDisplayTypeForGUIRedraw();
		DisplayHP48Screen(optionTarget);		
		goto restartDEloop;
	}
	EvtFlushPenQueue( );
}
	    



/***********************************************************************
 *
 * FUNCTION:    MainFormHandleEvent
 *
 * DESCRIPTION: This routine is the event handler for the 
 *              "MainForm" of this application.
 *
 * PARAMETERS:  eventP  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean MainFormHandleEvent(EventPtr eventP)
{
	Boolean handled = false;
	FormPtr frmP; 
	Err error;
	EventType newEvent;
	RectangleType copyRect;
	Int16 i;
	
	switch (eventP->eType) 
		{
		case menuEvent:
			break; // no menu code here!

		case frmOpenEvent:
			frmP = FrmGetActiveForm();

			if ((collapsibleIA) && (PINVersion))
			{
				WinSetConstraintsSize(FrmGetWindowHandle (frmP), 160, 225, 225, 160, 160, 160);	
				FrmSetDIAPolicyAttr(frmP, frmDIAPolicyCustom);
				PINSetInputTriggerState(pinInputTriggerEnabled);
				error = PINSetInputAreaState(pinInputAreaUser);
			}
			
			if ((PINVersion) && (PINVersion!=pinAPIVersion1_0))
			{
				SysSetOrientation(sysOrientationPortrait);
				SysSetOrientationTriggerState(sysOrientationTriggerDisabled);
				WinScreenGetAttribute(winScreenRowBytes, &screenRowInt16s);
				screenRowInt16s>>=1;
				mainDisplayWindow=WinGetDisplayWindow();
			}
			
			FrmDrawForm ( frmP);
			
			WinSetCoordinateSystem(kCoordinatesNative);
					
			do
			{
				statusSwitchEmulation=false;  // this is now taken care of in DisplayHP48screen
						
				error=LoadEmulator();  // Load up the ROM and RAM contents and start the emulator
						
				if (error) DryEmulate();   // Just bring up the emulator shell
						
				if (statusSwitchEmulation)
				{
					Q->statusQuit=false;
					optionTarget=statusNextTarget;
				}
			} while (statusSwitchEmulation);
							
			WinSetCoordinateSystem(kCoordinatesStandard);
			
			// Send Appstop event message to form loop
			newEvent.eType = appStopEvent;
			EvtAddEventToQueue(&newEvent);

			handled = true;
			break;
			
		case frmUpdateEvent:
			//ErrorPrint("update detected",true);
			frmP = FrmGetActiveForm();
			FrmDrawForm (frmP);

			WinSetCoordinateSystem(kCoordinatesNative);
			copyRect.topLeft.x = 0;
			copyRect.topLeft.y = 0;
			copyRect.extent.x = 320;
			copyRect.extent.y = ((optionThreeTwentySquared)?320:450);
			P48WinCopyRectangle(backStoreWindow,mainDisplayWindow,&copyRect,0,0,winPaint);
			ShowPanel();
			for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
			WinSetCoordinateSystem(kCoordinatesStandard);

			handled = true;
			// To do any custom drawing here, first call FrmDrawForm(), then do your
			// drawing, and then set handled to true.
			break;
			
		default:
			break;
		
		}
	
	return handled;
}


/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static Boolean AppHandleEvent(EventPtr eventP)
{
	UInt16 formId;
	FormPtr frmP;
	
	if (eventP->eType == frmLoadEvent)
		{
		// Load the form resource.
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);
		
		// Set the event handler for the form.  The handler of the currently
		// active form is called by FrmHandleEvent each time is receives an
		// event.
		switch (formId)
			{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

			default:
//				ErrFatalDisplay("Invalid Form Load Event");
				break;

			}
		return true;
		}
	
	return false;
}


/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the temporary initial event loop for the
 *              Power48 application. It is only active until the screen
 *              size is changed and event handling switches to the main calc
 *              loop in Saturn.c
 *              
 ***********************************************************************/
static void AppEventLoop(void)
{
	UInt16 error;
	EventType event;

	do {
		EvtGetEvent(&event, evtWaitForever);

		if (! SysHandleEvent(&event))
			if (! MenuHandleEvent(0, &event, &error))
				if (! AppHandleEvent(&event))
					FrmDispatchEvent(&event);

	} while (event.eType != appStopEvent);
}



/***********************************************************************
 *
 * FUNCTION:     AppStart
 *
 * DESCRIPTION:  Get the current application's preferences.
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
 
#define kPalmDeviceIDZire72			'Zi72'	/**< Device ID for Zire72. */


static Err AppStart(void)
{
	UInt16 prefsSize;
	UInt32 depth;
	Boolean enableColor;
	UInt16 cardNo;
	LocalID dbID;
	Err error;
	Coord picWidth,picHeight;
	MemHandle picHandle;
	BitmapPtr picBitmap;
	pref_t options;
	pref_t_090b *options_090b;
	pref_t_100 *options_100;
	Boolean offscreenError=false;
	Boolean regError=false;
	Int16 myPrefVersion;
	UInt32 version,id;
	UInt32 attr;
	UInt16 vskState;
	UInt32 hSpace;
	
	
	
	// First, Read the preference information for the calculator app
	prefsSize = sizeof(pref_t);
	myPrefVersion = PrefGetAppPreferences(appFileCreator, appPrefID, &options, &prefsSize, true);
	if (myPrefVersion == noPreferenceFound)
	{
		statusCompleteReset=true;
	}
	else if (myPrefVersion == appPrefVersionNum_090b)
	{
		options_090b = (pref_t_090b*)&options;
		optionTarget               = ((options_090b->loadSX)?HP48SX:HP48GX);
		optionVerbose              = options_090b->verbose;
		optionScrollFast           = options_090b->scrollFast;
		optionThreeTwentySquared   = options_090b->oThreeTwentySquared;
		option48GX128KExpansion    = options_090b->o48GX128KExpansion;
		option48SX128KExpansion    = options_090b->o48SX128KExpansion;
	}
	else if (myPrefVersion == appPrefVersionNum_100)
	{
		options_100 = (pref_t_100*)&options;
		optionTarget               = options_100->target;
		optionVerbose              = options_100->verbose;
		optionScrollFast           = options_100->scrollFast;
		optionThreeTwentySquared   = options_100->oThreeTwentySquared;
		option48GXBackdrop         = options_100->o48GXBackdrop;
		option48SXBackdrop         = options_100->o48SXBackdrop;
		option49GBackdrop          = options_100->o49GBackdrop;
		option48GXColorCycle       = options_100->o48GXColorCycle;
		option48SXColorCycle       = options_100->o48SXColorCycle;
		option49GColorCycle        = options_100->o49GColorCycle;
		option48GX128KExpansion    = options_100->o48GX128KExpansion;
		option48SX128KExpansion    = options_100->o48SX128KExpansion;
		MemMove(&option48GXPixelColor,&options_100->o48GXPixelColor,sizeof(RGBColorType));
		MemMove(&option48GXBackColor,&options_100->o48GXBackColor,sizeof(RGBColorType));
		MemMove(&option48SXPixelColor,&options_100->o48SXPixelColor,sizeof(RGBColorType));
		MemMove(&option48SXBackColor,&options_100->o48SXBackColor,sizeof(RGBColorType));
		MemMove(&option49GPixelColor,&options_100->o49GPixelColor,sizeof(RGBColorType));
		MemMove(&option49GBackColor,&options_100->o49GBackColor,sizeof(RGBColorType));
	}
	else if (myPrefVersion == appPrefVersionNum)
	{
		optionTarget               = options.target;
		optionVerbose              = options.verbose;
		optionScrollFast           = options.scrollFast;
		optionThreeTwentySquared   = options.oThreeTwentySquared;
		option48GXBackdrop         = options.o48GXBackdrop;
		option48SXBackdrop         = options.o48SXBackdrop;
		option49GBackdrop          = options.o49GBackdrop;
		option48GXColorCycle       = options.o48GXColorCycle;
		option48SXColorCycle       = options.o48SXColorCycle;
		option49GColorCycle        = options.o49GColorCycle;
		option48GX128KExpansion    = options.o48GX128KExpansion;
		option48SX128KExpansion    = options.o48SX128KExpansion;
		MemMove(&option48GXPixelColor,&options.o48GXPixelColor,sizeof(RGBColorType));
		MemMove(&option48GXBackColor,&options.o48GXBackColor,sizeof(RGBColorType));
		MemMove(&option48SXPixelColor,&options.o48SXPixelColor,sizeof(RGBColorType));
		MemMove(&option48SXBackColor,&options.o48SXBackColor,sizeof(RGBColorType));
		MemMove(&option49GPixelColor,&options.o49GPixelColor,sizeof(RGBColorType));
		MemMove(&option49GBackColor,&options.o49GBackColor,sizeof(RGBColorType));
		optionPanelOpen 			= options.oPanelOpen;
		optionPanelType 			= options.oPanelType;
		optionPanelX 				= options.oPanelX;
		optionPanelY				= options.oPanelY;
		optionDisplayMonitor 		= options.oDisplayMonitor;
		optionTrueSpeed 			= options.oTrueSpeed;
		optionDisplayZoomed 		= options.oDisplayZoomed;
		optionKeyClick		 		= options.oKeyClick;
	}
	statusPanelHidden = !optionDisplayMonitor;
	
	// Second, make sure screen supports double density and set depth to 16 bits
	error=FtrGet(sysFtrCreator, sysFtrNumWinVersion, &version);
	if (version<4) 
	{
		// High-Density Feature Set not present
		// Display alert and quit
		FrmAlert(NoHiResLibraryAlert);
		return sysErrLibNotFound;
	}
	
	WinScreenGetAttribute(winScreenDensity, &attr);
	if (attr != kDensityDouble)
	{
		// Not a high-density screen
		// Display alert and quit
		FrmAlert(NoHDScreenAlert);
		return sysErrLibNotFound;
	}
	
	depth=16;
	enableColor=true;
	// Get current screen values so we can restore them on quit
	WinScreenMode(winScreenModeGet, NULL, NULL, &oldScreenDepth, NULL);
	error = WinScreenMode(winScreenModeSet, NULL, NULL, &depth, &enableColor);
	if (error)
	{
		FrmAlert(NoColorSupportAlert);
		return sysErrLibNotFound;
	}
	
  	
 	// Third, set up collapsible graffiti support for both Palm and Sony API's
	error = SysCurAppDatabase(&cardNo, &dbID);
	if (error) 
	{
		FrmAlert(UnableToRegisterAlert);
		return error;
	}

	error = FtrGet(pinCreator, pinFtrAPIVersion, &version);
	if (!error) 
	{
		// PalmOS Pen Input Manager exists - give this API priority over Sony's
		PINVersion = version;
    	collapsibleIA=true;
    	
		error = SysNotifyRegister( cardNo, dbID, sysNotifyDisplayResizedEvent, NULL, sysNotifyNormalPriority, NULL );		
	 	if (error)
		{
			FrmAlert(UnableToRegisterAlert);
			return sysErrNoFreeResource;
		}
	}
 	else
 	{
 		// PIN not available, try Sony SILK
 		// load up Sony SILK library if not already there
		if ((error = SysLibFind(sonySysLibNameSilk, &silkLibRefNum)))
		{
			if (error == sysErrLibNotFound) 
			{
				// couldn't find lib 
				error = SysLibLoad( 'libr', sonySysFileCSilkLib, &silkLibRefNum );
			}
		}

		if (!error)
		{
			// Sony SILK lib available for use - now get version
			error = FtrGet(sonySysFtrCreator, sonySysFtrNumVskVersion, &version);
			if (!error) 
			{
				// Version 2 or up is installed - Some added features is available 
				if (VskOpen(silkLibRefNum)==errNone)
				{
					SILKVersion=version;
	    			collapsibleIA=true;
					if (version>=vskVersionNum3)
					{
						// supports horizontal and vertical resize
						VskGetState(silkLibRefNum,vskStateResizeDirection,&vskState);
						if (vskState==vskResizeHorizontally) statusLandscapeAvailable=true;
						VskSetState(silkLibRefNum, vskStateEnable, (vskResizeHorizontally|vskResizeVertically));
					}
					else
					{
						// supports only vertical resize
						VskSetState(silkLibRefNum, vskStateEnable, vskResizeVertically);
					}
					error = SysNotifyRegister( cardNo, dbID, sysNotifyDisplayChangeEvent, NULL, sysNotifyNormalPriority, NULL );		
				 	if (error)
					{
						FrmAlert(UnableToRegisterAlert);
						return sysErrNoFreeResource;
					}
				}
			}
		}
 	}
 	
	// Fourth, change performance marker for fast devices
	if (!FtrGet(sysFtrCreator, sysFtrNumOEMDeviceID, &id)) 
	{
		if ((id == kPalmDeviceIDTungstenT3) ||
		    (id == kPalmDeviceIDTungstenC) ||
		    (id == kPalmDeviceIDZire72)) statusFastDevice=true;
	}
	
	// Fifth, look for presence of 5-way navigators
	if (!FtrGet(navFtrCreator, navFtrVersion, &version)) 
	{
		status5WayNavPresent=true;
		status5WayNavType=PALMONE;
		// block nav keys from sending events
		KeySetMask(keyBitsAll - (keyBitPageUp | keyBitPageDown | keyBitNavLeft | keyBitNavRight));
	}
	if (!FtrGet(twFtrCreator, twFtrAPIVersion, &version)) 
	{
		status5WayNavPresent=true;
		status5WayNavType=TAPWAVE;
		// block nav keys from sending events
		KeySetMask(keyBitsAll - (keyBitRockerUp | keyBitRockerDown | keyBitRockerLeft | keyBitRockerRight | keyBitRockerCenter));
	}

	// Finally, set up initial graphic windows...
	
	// iQue 3600 and other support.
	// The iQue 3600 has a 320x480 screen, but the video memory is laid out like 
	// it was a 336x480 screen, so we have to take into account the number of 
	// rowbytes that make up the screen bitmap.
	WinScreenGetAttribute(winScreenRowBytes, &screenRowInt16s);
	screenRowInt16s>>=1;
	
	mainDisplayWindow=WinGetDisplayWindow();
	
	// Get annunciator bitmap graphic
	picHandle = DmGetResource(bitmapRsc, AnnunciatorMapBitmapFamily);
	picBitmap = MemHandleLock(picHandle);
	BmpGetDimensions(picBitmap,&picWidth,&picHeight,NULL);
	annunciatorBitmapWindow = WinCreateOffscreenWindow(picWidth>>1,picHeight>>1,nativeFormat,&error);
	if (error) offscreenError=true;
	WinSetDrawWindow(annunciatorBitmapWindow);
	WinDrawBitmap(picBitmap,0,0);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);

	// Scratch window for drawing/rotating text
	textWindow=WinCreateOffscreenWindow(maxExpandedTextWidth>>1,maxExpandedTextHeight>>1,nativeFormat,&error);
	textBitmap=WinGetBitmap(textWindow);
	textBitmapPtr=BmpGetBits(textBitmap);
	if (error) offscreenError=true;

	if (offscreenError)
	{
		FrmAlert(NoOffscreenWindowsAlert);
		return sysErrNoFreeResource;
	}
	
	WinSetDrawWindow(mainDisplayWindow);

	
	// Register to receive various OS notifications	
	if (SysNotifyRegister(cardNo, dbID, sysNotifyVolumeUnmountedEvent, NULL, sysNotifyNormalPriority, NULL)) regError=true;	
	if (SysNotifyRegister(cardNo, dbID, sysNotifyVolumeMountedEvent, NULL, sysNotifyNormalPriority, NULL)) regError=true;	
	if (SysNotifyRegister(cardNo, dbID, sysNotifyTimeChangeEvent, NULL, sysNotifyNormalPriority, NULL)) regError=true;	
	if (SysNotifyRegister(cardNo, dbID, sysNotifyLateWakeupEvent, NULL, sysNotifyNormalPriority, NULL)) regError=true;	
	if (SysNotifyRegister(cardNo, dbID, sysNotifyDeviceUnlocked, NULL, sysNotifyNormalPriority, NULL)) regError=true;	
	if (regError)
	{
		FrmAlert(UnableToRegisterAlert);
		return sysErrNoFreeResource;
	}
	
	// Register default VFS directories
	RegisterVFSExtensions();
	
	Q = (emulState*)MemPtrNew(sizeof(emulState));
	MemSet(Q,sizeof(emulState),0);
		
	
	// okay now that we've allocated all the background stuff, see if there's enough 
	// dynamic heap space to do image allocations there instead of in program store.
	hSpace = GetHeapSpace();
	if (hSpace>0x380000)            // if dynamic heap space is > 0x380000 bytes then put RAM and ROM in it
		statusUseSystemHeap=ALL_IN_DHEAP;
	else if (hSpace>0x200000)       // if dynamic heap space is only > 0x200000 bytes then just put RAM in it
		statusUseSystemHeap=ROM_IN_FTRMEM;
	else                            // otherwise put it all in feature memory
		statusUseSystemHeap=ALL_IN_FTRMEM;
	
	return errNone;
}


/***********************************************************************
 *
 * FUNCTION:    AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/
static void AppStop(void)
{
	UInt16 prefsSize;
	pref_t options, oldOptions;
	UInt16 cardNo;
	LocalID dbID;
	Int16 myPrefVersion;
	
	
	// Write the preference information for the calculator app. This data 
	// will saved during a HotSync backup.
	MemSet(&options, sizeof(pref_t), 0);
	
	options.target                   = optionTarget;
	options.verbose                  = optionVerbose;
	options.scrollFast               = optionScrollFast;
	options.oThreeTwentySquared      = optionThreeTwentySquared;
	options.o48GXBackdrop            = option48GXBackdrop;
	options.o48SXBackdrop            = option48SXBackdrop;
	options.o49GBackdrop             = option49GBackdrop;
	options.o48GXColorCycle          = option48GXColorCycle;
	options.o48SXColorCycle          = option48SXColorCycle;
	options.o49GColorCycle           = option49GColorCycle;
	options.o48GX128KExpansion       = option48GX128KExpansion;
	options.o48SX128KExpansion       = option48SX128KExpansion;
	
	options.oDisplayMonitor			= optionDisplayMonitor;
	options.oTrueSpeed				= optionTrueSpeed;
	options.oPanelType				= optionPanelType;
	options.oPanelX					= optionPanelX;
	options.oPanelY					= optionPanelY;
	options.oPanelOpen				= optionPanelOpen;
	options.oDisplayZoomed			= optionDisplayZoomed;
	options.oKeyClick				= optionKeyClick;
	
	MemMove(&options.o48GXPixelColor,&option48GXPixelColor,sizeof(RGBColorType));
	MemMove(&options.o48GXBackColor,&option48GXBackColor,sizeof(RGBColorType));
	MemMove(&options.o48SXPixelColor,&option48SXPixelColor,sizeof(RGBColorType));
	MemMove(&options.o48SXBackColor,&option48SXBackColor,sizeof(RGBColorType));
	MemMove(&options.o49GPixelColor,&option49GPixelColor,sizeof(RGBColorType));
	MemMove(&options.o49GBackColor,&option49GBackColor,sizeof(RGBColorType));
	
	prefsSize = sizeof(pref_t);
	myPrefVersion = PrefGetAppPreferences(appFileCreator, appPrefID, &oldOptions, &prefsSize, true);   // get previous stored prefs
	// only save prefs if different from saved to reduce hotsyncs.
	if (MemCmp(&options,&oldOptions,sizeof(pref_t)))
	{
		PrefSetAppPreferences(appFileCreator, appPrefID, appPrefVersionNum, &options, sizeof(pref_t), true);
	}
	
	// Delete any existing memory handles
	if (backdrop1x1!=NULL) WinDeleteWindow(backdrop1x1,false);
	if (backdrop2x2!=NULL) WinDeleteWindow(backdrop2x2,false);
	if (backdrop2x2PVL!=NULL) WinDeleteWindow(backdrop2x2PVL,false);
	if (panelWindow!=NULL) WinDeleteWindow(panelWindow,false);
	if (digitsWindow!=NULL) WinDeleteWindow(digitsWindow,false);
	if (backStoreWindow!=NULL) WinDeleteWindow(backStoreWindow,false);
	
	WinDeleteWindow(textWindow,false);
	WinDeleteWindow(annunciatorBitmapWindow,false);
		
	MemPtrFree(Q); 
		
	// Revert screen mode?
	WinScreenMode(winScreenModeSet, NULL, NULL, &oldScreenDepth, NULL);
				
	// Close all the open forms.
	FrmCloseAllForms ();

	SysCurAppDatabase(&cardNo, &dbID);
	SysNotifyUnregister(cardNo, dbID, sysNotifyVolumeUnmountedEvent,
								sysNotifyNormalPriority);
	SysNotifyUnregister(cardNo, dbID, sysNotifyVolumeMountedEvent,
								sysNotifyNormalPriority);
	SysNotifyUnregister(cardNo, dbID, sysNotifyTimeChangeEvent,
								sysNotifyNormalPriority);
	SysNotifyUnregister(cardNo, dbID, sysNotifyLateWakeupEvent,
								sysNotifyNormalPriority);
	SysNotifyUnregister(cardNo, dbID, sysNotifyDeviceUnlocked,
								sysNotifyNormalPriority);
								
	if (collapsibleIA)
	{
		if (PINVersion)
		{
			SysNotifyUnregister(cardNo, dbID, sysNotifyDisplayResizedEvent, sysNotifyNormalPriority);
		}
		else
		{
			SysNotifyUnregister(cardNo, dbID, sysNotifyDisplayChangeEvent, sysNotifyNormalPriority);
			VskClose(silkLibRefNum);
		}
	}
	
	// restore events for nav keys
	if (status5WayNavPresent) 
	{
		KeySetMask(keyBitsAll);
	}

}










/* all code from here to end of file should use no global variables */
#pragma warn_a5_access on


void RegisterVFSExtensions(void)
{
	VFSRegisterDefaultDirectory(ROMFileExtension,expMediaType_Any,VFSROMRAMDirectory);
	VFSRegisterDefaultDirectory(RAMFileExtension,expMediaType_Any,VFSROMRAMDirectory);
	VFSRegisterDefaultDirectory(OBJFileExtension,expMediaType_Any,objectVFSDirName);
}

/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
    UInt32 romVersion;

    /* See if we're on in minimum required version of the ROM or later. */
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    if (romVersion < requiredVersion)
    {
        if ((launchFlags & 
            (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
            (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
        {
            FrmAlert (RomIncompatibleAlert);

            /* Palm OS versions before 2.0 will continuously relaunch this
             * app unless we switch to another safe one. */
            if (romVersion < kPalmOS20Version)
            {
                AppLaunchWithCommand(
                    sysFileCDefaultApp, 
                    sysAppLaunchCmdNormalLaunch, NULL);
            }
        }

        return sysErrRomIncompatible;
    }

    return errNone;
}



/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPBP - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 *
 * RETURNED:    Result of launch, errNone if all went OK
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;
	EventType newEvent;
	
	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	switch (cmd)
		{
		case sysAppLaunchCmdNormalLaunch:
			error = AppStart();
			if (error) 
				return error;
				
			FrmGotoForm(MainForm);
			AppEventLoop();
			AppStop();
			break;
			
		case sysAppLaunchCmdSyncNotify:
		case sysAppLaunchCmdSystemReset:
			RegisterVFSExtensions();
			break;
			
		case sysAppLaunchCmdNotify:
			if ((((SysNotifyParamType*)cmdPBP)->notifyType == sysNotifyDisplayChangeEvent) ||
				(((SysNotifyParamType*)cmdPBP)->notifyType == sysNotifyDisplayResizedEvent))
			{
				MemSet(&newEvent, sizeof(EventType), 0);
				newEvent.eType = winDisplayChangedEvent;
				EvtAddUniqueEventToQueue(&newEvent, 0, true);
			}
			if (((SysNotifyParamType*)cmdPBP)->notifyType == 
				sysNotifyVolumeUnmountedEvent)
			{
				newEvent.eType = volumeUnmountedEvent;
				newEvent.data.generic.datum[0] = (UInt16)((SysNotifyParamType*)cmdPBP)->notifyDetailsP;
				EvtAddEventToQueue(&newEvent);
			}
			if (((SysNotifyParamType*)cmdPBP)->notifyType == 
				sysNotifyVolumeMountedEvent)
			{
				((SysNotifyParamType*)cmdPBP)->handled |= vfsHandledStartPrc;
				((SysNotifyParamType*)cmdPBP)->handled |= vfsHandledUIAppSwitch;
				newEvent.eType = volumeMountedEvent;
				EvtAddEventToQueue(&newEvent);
			}
			if ((((SysNotifyParamType*)cmdPBP)->notifyType == sysNotifyTimeChangeEvent) ||
				(((SysNotifyParamType*)cmdPBP)->notifyType == sysNotifyLateWakeupEvent) ||
				(((SysNotifyParamType*)cmdPBP)->notifyType == sysNotifyDeviceUnlocked))
			{
				if (launchFlags & sysAppLaunchFlagSubCall)  // okay to use A5 globals
					AdjustTime();
			}
			break;

		default:
			break;

		}
	
	return errNone;
}

/* turn a5 warning off to prevent it being set off by C++
 * static initializer code generation */
#pragma warn_a5_access reset





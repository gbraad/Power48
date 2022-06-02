/*************************************************************************\
*
*  DISPLAY.C
*
*  This module is responsible for all emulator display and GUI elements of
*  the emulator
*
*  (c)2002 by Robert Hildinger
*
\*************************************************************************/


#include "Power48.h"
#include "Power48_core.h"
#include <PalmOS.h> 
#include <SonyCLIE.h>
#include "IO.H"
#include <PceNativeCall.h>




#define 	softKeyAIntX 	12
#define 	softKeyBIntX 	59
#define 	softKeyCIntX 	106
#define 	softKeyDIntX 	12
#define 	softKeyEIntX 	59
#define 	softKeyFIntX 	106

#define 	softKeyRow1Y	25
#define 	softKeyRow2Y	52

#define		softLabelStart	57






/*************************************************************************\
*
*  FUNCTION:  rect_sweep()
*
\*************************************************************************/
void rect_sweep(RectangleType *oldRect,RectangleType *newRect, int numSteps)
{
	int i;
	int stepX,stepY,stepExtX,stepExtY;
	RectangleType saveRect,copyRect;
	RGBColorType prevRGB;
	
	saveRect=*oldRect;
	
	stepX=(newRect->topLeft.x-oldRect->topLeft.x)/numSteps;
	stepY=(newRect->topLeft.y-oldRect->topLeft.y)/numSteps;
	stepExtX=(newRect->extent.x-oldRect->extent.x)/numSteps;
	stepExtY=(newRect->extent.y-oldRect->extent.y)/numSteps;
	
	WinSetForeColorRGB(&whiteRGB,&prevRGB);
	for (i=0;i<numSteps;i++)
	{
		oldRect->topLeft.x+=stepX;
		oldRect->topLeft.y+=stepY;
		oldRect->extent.x+=stepExtX;
		oldRect->extent.y+=stepExtY;
		WinDrawRectangleFrame(simpleFrame,oldRect);
		SysTaskDelay(1);
	}
	*oldRect=saveRect;
	for (i=0;i<numSteps;i++)
	{
		oldRect->topLeft.x+=stepX;
		oldRect->topLeft.y+=stepY;
		oldRect->extent.x+=stepExtX;
		oldRect->extent.y+=stepExtY;
		
		copyRect.topLeft.x=oldRect->topLeft.x-1;
		copyRect.topLeft.y=oldRect->topLeft.y-2;
		copyRect.extent.x =oldRect->extent.x+3;
		copyRect.extent.y =3;
		WinCopyRectangle(backStoreWindow,mainDisplayWindow,&copyRect,copyRect.topLeft.x,copyRect.topLeft.y,winPaint);
		copyRect.topLeft.y=oldRect->topLeft.y+oldRect->extent.y-1;
		WinCopyRectangle(backStoreWindow,mainDisplayWindow,&copyRect,copyRect.topLeft.x,copyRect.topLeft.y,winPaint);
		copyRect.topLeft.y=oldRect->topLeft.y-1;
		copyRect.topLeft.x-=1;
		copyRect.extent.x =3;
		copyRect.extent.y =oldRect->extent.y+3;
		WinCopyRectangle(backStoreWindow,mainDisplayWindow,&copyRect,copyRect.topLeft.x,copyRect.topLeft.y,winPaint);
		copyRect.topLeft.x=oldRect->topLeft.x+oldRect->extent.x-1;
		WinCopyRectangle(backStoreWindow,mainDisplayWindow,&copyRect,copyRect.topLeft.x,copyRect.topLeft.y,winPaint);

		SysTaskDelay(1);
	}
	WinSetForeColorRGB(&prevRGB,NULL);
}






/*************************************************************************\
*
*  FUNCTION:  toggle_lcd()
*
\*************************************************************************/
void toggle_lcd(void)
{
	int i;
	RGBColorType frameColorRGB={0,115,113,99},prevRGB;
	RectangleType opRect,oldRect,newRect;
	
	// toggle back and forth between 2x2 and 1x1 LCDs
	if (statusDisplayMode2x2)
	{
		// switch to 1x1 display mode
		statusDisplayMode2x2=false;
		Q->statusDisplayMode2x2=false;
		virtLayout = (squareDef*)virtLayout1x1;
		optionDisplayZoomed = false;
		
		// setup LCD for delayed complete refresh 
		for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;
		
		// do shrink graphic / clear out remnants of 2x2 mode via back store copy
		oldRect.topLeft.x=fullScreenOriginX-3;
		oldRect.topLeft.y=fullScreenOriginY-3;
		oldRect.extent.x=fullScreenExtentX+6;
		oldRect.extent.y=fullScreenExtentY+6;
		newRect.topLeft.x=lcd1x1AnnOriginX;
		newRect.topLeft.y=lcd1x1AnnOriginY;
		newRect.extent.x=lcd1x1FSExtentX+lcd1x1AnnExtentX+4;
		newRect.extent.y=lcd1x1FSExtentY;
		rect_sweep(&oldRect,&newRect,8);
		
		opRect.topLeft.x=fullScreenOriginX-4;
		opRect.topLeft.y=fullScreenOriginY-4;
		opRect.extent.x=fullScreenExtentX+8;
		opRect.extent.y=fullScreenExtentY+8;
		WinCopyRectangle(backStoreWindow,mainDisplayWindow,&opRect,opRect.topLeft.x,opRect.topLeft.y,winPaint);
		
		display_clear();
		ShowPanel();
	}
	else
	{
		HidePanel();
		// switch to 2x2 display mode
		statusDisplayMode2x2=true;
		Q->statusDisplayMode2x2=true;
		virtLayout = (squareDef*)virtLayout2x2;
		optionDisplayZoomed = true;
		
		// setup LCD for complete refresh 
		for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;
		
		// save copy of background
		opRect.topLeft.x=fullScreenOriginX-4;
		opRect.topLeft.y=fullScreenOriginY-4;
		opRect.extent.x=fullScreenExtentX+8;
		opRect.extent.y=fullScreenExtentY+8;
		WinCopyRectangle(mainDisplayWindow,backStoreWindow,&opRect,opRect.topLeft.x,opRect.topLeft.y,winPaint);

		// do expansion graphic
		newRect.topLeft.x=fullScreenOriginX-3;
		newRect.topLeft.y=fullScreenOriginY-3;
		newRect.extent.x=fullScreenExtentX+6;
		newRect.extent.y=fullScreenExtentY+6;
		oldRect.topLeft.x=lcd1x1AnnOriginX;
		oldRect.topLeft.y=lcd1x1AnnOriginY;
		oldRect.extent.x=lcd1x1FSExtentX+lcd1x1AnnExtentX+4;
		oldRect.extent.y=lcd1x1FSExtentY;
		rect_sweep(&oldRect,&newRect,8);
		
		// draw frame?
		WinSetForeColorRGB(&frameColorRGB,&prevRGB);
		opRect.topLeft.x=fullScreenOriginX-2;
		opRect.topLeft.y=fullScreenOriginY-2;
		opRect.extent.x=fullScreenExtentX+4;
		opRect.extent.y=fullScreenExtentY+4;
		WinDrawRectangleFrame(simpleFrame,&opRect);
		opRect.topLeft.x++;
		opRect.topLeft.y++;
		opRect.extent.x-=2;
		opRect.extent.y-=2;
		WinDrawRectangleFrame(simpleFrame,&opRect);
		opRect.topLeft.x++;
		opRect.topLeft.y++;
		opRect.extent.x-=2;
		opRect.extent.y-=2;
		WinDrawRectangleFrame(simpleFrame,&opRect);
		WinSetForeColorRGB(&prevRGB,NULL);
		
		display_clear();
	}
}




/*************************************************************************\
*
*  FUNCTION:  init_lcd()
*
\*************************************************************************/
void init_lcd() 
{
	UInt32 *drawMemPtrBase;
	UInt16 *ptr16;
	UInt32 *ptr32;
	
	drawMemPtrBase = BmpGetBits(WinGetBitmap(WinGetDisplayWindow()));
	
	ptr16 						= BmpGetBits(WinGetBitmap(annunciatorBitmapWindow));
	Q->annunciatorMemPtrBase	= (UInt16*)ByteSwap32(ptr16);

	ptr32 						= drawMemPtrBase + (((lcdOriginY*screenRowInt16s) + lcdOriginX)/2);   // Move to start of hp48 display area (2x2 mode)
	Q->lcd2x2MemPtrBase			= (UInt32*)ByteSwap32(ptr32);

	ptr32 						= drawMemPtrBase + ((((screenHeight-1-lcdOriginX)*screenRowInt16s) + lcdOriginY)/2);   // Move to start of hp48 display area (2x2 mode) (portrait via landscape)
	Q->lcd2x2MemPtrBasePVL		= (UInt32*)ByteSwap32(ptr32);

	ptr16 						= (UInt16*)drawMemPtrBase + (lcd1x1OriginY*screenRowInt16s) + lcd1x1OriginX;   // Move to start of hp48 display area (1x1 mode)
	Q->lcd1x1MemPtrBase			= (UInt16*)ByteSwap32(ptr16);

	ptr32 						= drawMemPtrBase + (((annunciator2x2OriginY*screenRowInt16s) + annunciator2x2OriginX)/2); // Move to start of annunciator area.
	Q->lcd2x2AnnAreaPtrBase		= (UInt32*)ByteSwap32(ptr32);

	ptr32 						= drawMemPtrBase + ((((screenHeight-1-annunciator2x2OriginX)*screenRowInt16s) + annunciator2x2OriginY)/2); // Move to start of annunciator area. (portrait via landscape)
	Q->lcd2x2AnnAreaPtrBasePVL	= (UInt32*)ByteSwap32(ptr32);

	ptr32 						= drawMemPtrBase + (((annunciator1x1OriginY*screenRowInt16s) + annunciator1x1OriginX)/2); // Move to start of annunciator area.
	Q->lcd1x1AnnAreaPtrBase		= (UInt32*)ByteSwap32(ptr32);

	ptr16 						= (UInt16*)drawMemPtrBase + (softKeyRow1Y*screenRowInt16s) + softKeyAIntX;   // Move to start of softKey label area (1x1 mode)
	Q->softKeyMemPtrBase		= (UInt16*)ByteSwap32(ptr16);

	ptr16 						= (UInt16*)drawMemPtrBase + ((lcd1x1OriginY+softLabelStart)*screenRowInt16s) + lcd1x1OriginX;   // Move to start of lcd label area (1x1 mode)
	Q->labelMemPtrBase			= (UInt16*)ByteSwap32(ptr16);

/*	ptr32 						= BmpGetBits(WinGetBitmap(WinGetDisplayWindow()));
	Q->displayPtr				= (UInt32*)ByteSwap32(ptr32);

	ptr32 						= BmpGetBits(WinGetBitmap(backStoreWindow));
	Q->backStorePtr				= (UInt32*)ByteSwap32(ptr32);*/
}



/*************************************************************************\
*
*  FUNCTION:  exit_lcd()
*
\*************************************************************************/
void exit_lcd()
{
	return;
}



/*************************************************************************\
*
*  FUNCTION:  display_clear()
*
\*************************************************************************/
void display_clear(void) 
{
	RectangleType screenRect;
	RGBColorType prevRGB;
	RGBColorType divColor={0,112,119,96};
	
	if (statusDisplayMode2x2)
	{
		if (!statusBackdrop)
		{
			WinSetForeColorRGB(statusBackColor,&prevRGB);
			screenRect.topLeft.x = fullScreenOriginX;
			screenRect.topLeft.y = fullScreenOriginY;
			screenRect.extent.x = fullScreenExtentX;
			screenRect.extent.y = fullScreenExtentY;
			P48WinDrawRectangle(&screenRect, 0);
			
			WinSetForeColorRGB(&prevRGB,NULL);
		}
		else
		{
			if (statusPortraitViaLandscape)
			{
				screenRect.topLeft.x = 0;
				screenRect.topLeft.y = 0;
				screenRect.extent.x = fullScreenExtentY;
				screenRect.extent.y = fullScreenExtentX;
				WinCopyRectangle(backdrop2x2PVL,mainDisplayWindow,&screenRect,
					fullScreenOriginY,screenHeight-fullScreenOriginX-fullScreenExtentX,winPaint);
			}
			else
			{
				screenRect.topLeft.x = 0;
				screenRect.topLeft.y = 0;
				screenRect.extent.x = fullScreenExtentX;
				screenRect.extent.y = fullScreenExtentY;
				WinCopyRectangle(backdrop2x2,mainDisplayWindow,&screenRect,
					fullScreenOriginX,fullScreenOriginY,winPaint);
			}
		}
	}
	else
	{
		if (!statusBackdrop)
		{
			WinSetForeColorRGB(statusBackColor,&prevRGB);
			screenRect.topLeft.x = lcd1x1FSOriginX;
			screenRect.topLeft.y = lcd1x1FSOriginY;
			screenRect.extent.x = lcd1x1FSExtentX;
			screenRect.extent.y = lcd1x1FSExtentY;
			WinDrawRectangle(&screenRect, 0);
			screenRect.topLeft.x = lcd1x1AnnOriginX;
			screenRect.topLeft.y = lcd1x1AnnOriginY;
			screenRect.extent.x = lcd1x1AnnExtentX;
			screenRect.extent.y = lcd1x1AnnExtentY;
			WinDrawRectangle(&screenRect, 0);
			screenRect.topLeft.x = lcd1x1DividerOriginX;
			screenRect.topLeft.y = lcd1x1DividerOriginY;
			screenRect.extent.x = lcd1x1DividerExtentX;
			screenRect.extent.y = lcd1x1DividerExtentY;
			WinSetForeColorRGB(&divColor,NULL);
			WinDrawRectangle(&screenRect, 0);
			
			WinSetForeColorRGB(&prevRGB,NULL);
		}
		else
		{
			screenRect.topLeft.x = 0;
			screenRect.topLeft.y = 0;
			screenRect.extent.x = lcd1x1TotalExtentX;
			screenRect.extent.y = lcd1x1TotalExtentY;
			WinCopyRectangle(backdrop1x1,mainDisplayWindow,&screenRect,
							lcd1x1AnnOriginX,lcd1x1AnnOriginY,winPaint);
		}
	}
}






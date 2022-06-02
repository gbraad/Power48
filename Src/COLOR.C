/*************************************************************************\
*
*  COLOR.C
*
*  This module is responsible for implementing the color selector dialog
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


RGBColorType	tempPixelColor;
RGBColorType	tempBackColor; 


const prefButton colorLayout[] =   // coordinates relative to upper left of color dialog
{
	{  22, 124,  85, 149, 0 },		// colorCancel
	{ 105, 124, 168, 149, 0 },		// colorDefault
	{ 188, 124, 251, 149, 0 },		// colorOK
	{  61,  43,  77, 110, 0 },		// colorBackRed
	{  86,  43, 102, 110, 0 },		// colorBackGreen
	{ 111,  43, 127, 110, 0 },		// colorBackBlue
	{ 196,  43, 212, 110, 0 },		// colorPixelRed
	{ 221,  43, 237, 110, 0 },		// colorPixelGreen
	{ 246,  43, 262, 110, 0 },		// colorPixelBlue
}; 



/***********************************************************************
 *
 * FUNCTION:    TranslatePenToColor(Int16 x, Int16 y)
 *
 * DESCRIPTION: This routine translates a penDown/penUp event into
 *              the press release of a color selector dialog button
 *
 * PARAMETERS:  
 *
 * RETURNED:    
 *
 * REVISION HISTORY:
 *
 *
 ***********************************************************************/

static UInt16 TranslatePenToColor(Int16 x, Int16 y)
{
	UInt16 i;
	Int16 tmp;
	
	if (statusPortraitViaLandscape)
	{
		tmp=x;
		x=(screenHeight-1)-y;
		y=tmp;
	}
	
	for (i=0;i<maxColorButtons;i++)
	{
		if ((x >= (colorLayout[i].upperLeftX+colorDialogOriginX)) &&
		    (x <= (colorLayout[i].lowerRightX+colorDialogOriginX)) &&
		    (y >= (colorLayout[i].upperLeftY+colorDialogOriginY)) &&
		    (y <= (colorLayout[i].lowerRightY+colorDialogOriginY)))
		{
			return i;
		}
	}
	return noColorButton;
}




static void UpdateColorBars(void)
{
	UInt8 br,bg,bb,pr,pg,pb;
	RectangleType opRect;
	
	br=(tempBackColor.r>>3)<<1;
	bg=tempBackColor.g>>2;
	bb=(tempBackColor.b>>3)<<1;
	pr=(tempPixelColor.r>>3)<<1;
	pg=tempPixelColor.g>>2;
	pb=(tempPixelColor.b>>3)<<1;
	
	WinSetBackColorRGB(&greyRGB,NULL);
	
	WinSetForeColorRGB(&redRGB,NULL);
	opRect.topLeft.x=colorDialogOriginX+63;
	opRect.topLeft.y=colorDialogOriginY+45+(63-br);
	opRect.extent.x =13;
	opRect.extent.y =br+1;
	P48WinDrawRectangle(&opRect, 0);
	opRect.topLeft.x=colorDialogOriginX+63;
	opRect.topLeft.y=colorDialogOriginY+44;
	opRect.extent.x =13;
	opRect.extent.y =64-br;
	P48WinEraseRectangle(&opRect, 0);
	
	opRect.topLeft.x=colorDialogOriginX+198;
	opRect.topLeft.y=colorDialogOriginY+45+(63-pr);
	opRect.extent.x =13;
	opRect.extent.y =pr+1;
	P48WinDrawRectangle(&opRect, 0);
	opRect.topLeft.x=colorDialogOriginX+198;
	opRect.topLeft.y=colorDialogOriginY+44;
	opRect.extent.x =13;
	opRect.extent.y =64-pr;
	P48WinEraseRectangle(&opRect, 0);
	
	WinSetForeColorRGB(&greenRGB,NULL);
	opRect.topLeft.x=colorDialogOriginX+88;
	opRect.topLeft.y=colorDialogOriginY+45+(63-bg);
	opRect.extent.x =13;
	opRect.extent.y =bg+1;
	P48WinDrawRectangle(&opRect, 0);
	opRect.topLeft.x=colorDialogOriginX+88;
	opRect.topLeft.y=colorDialogOriginY+44;
	opRect.extent.x =13;
	opRect.extent.y =64-bg;
	P48WinEraseRectangle(&opRect, 0);
	
	opRect.topLeft.x=colorDialogOriginX+223;
	opRect.topLeft.y=colorDialogOriginY+45+(63-pg);
	opRect.extent.x =13;
	opRect.extent.y =pg+1;
	P48WinDrawRectangle(&opRect, 0);
	opRect.topLeft.x=colorDialogOriginX+223;
	opRect.topLeft.y=colorDialogOriginY+44;
	opRect.extent.x =13;
	opRect.extent.y =64-pg;
	P48WinEraseRectangle(&opRect, 0);
	
	WinSetForeColorRGB(&blueRGB,NULL);
	opRect.topLeft.x=colorDialogOriginX+113;
	opRect.topLeft.y=colorDialogOriginY+45+(63-bb);
	opRect.extent.x =13;
	opRect.extent.y =bb+1;
	P48WinDrawRectangle(&opRect, 0);
	opRect.topLeft.x=colorDialogOriginX+113;
	opRect.topLeft.y=colorDialogOriginY+44;
	opRect.extent.x =13;
	opRect.extent.y =64-bb;
	P48WinEraseRectangle(&opRect, 0);

	opRect.topLeft.x=colorDialogOriginX+248;
	opRect.topLeft.y=colorDialogOriginY+45+(63-pb);
	opRect.extent.x =13;
	opRect.extent.y =pb+1;
	P48WinDrawRectangle(&opRect, 0);
	opRect.topLeft.x=colorDialogOriginX+248;
	opRect.topLeft.y=colorDialogOriginY+44;
	opRect.extent.x =13;
	opRect.extent.y =64-pb;
	P48WinEraseRectangle(&opRect, 0);
}



inline void ArmRedisplay(void)
{
	UInt16 			pColor,bColor;
	RectangleType	screenRect;
		
	pColor=(((UInt16)tempPixelColor.r<<8)&0xf800)|(((UInt16)tempPixelColor.g<<3)&0x07e0)|(tempPixelColor.b>>3);
	bColor=(((UInt16)tempBackColor.r<<8)&0xf800)|(((UInt16)tempBackColor.g<<3)&0x07e0)|(tempBackColor.b>>3);

	Q->colorPixelVal=ByteSwap32(((UInt32)pColor<<16)|pColor);
	Q->colorBackVal=ByteSwap32(((UInt32)bColor<<16)|bColor);
	
	if (!statusBackdrop)
	{
		if (statusDisplayMode2x2)
		{
			screenRect.topLeft.x = fullScreenOriginX;
			screenRect.topLeft.y = fullScreenOriginY;
			screenRect.extent.x = fullScreenExtentX;
			screenRect.extent.y = 14;
			WinSetForeColorRGB(&tempBackColor,NULL);
			P48WinDrawRectangle(&screenRect, 0);
		}
		else
		{
			WinSetForeColorRGB(&tempBackColor,NULL);
			screenRect.topLeft.x = lcd1x1AnnOriginX;
			screenRect.topLeft.y = lcd1x1AnnOriginY;
			screenRect.extent.x = lcd1x1AnnExtentX;
			screenRect.extent.y = lcd1x1AnnExtentY;
			WinDrawRectangle(&screenRect, 0);
			screenRect.topLeft.x = lcd1x1FSOriginX;
			screenRect.topLeft.y = lcd1x1FSOriginY;
			screenRect.extent.x = 2;
			screenRect.extent.y = lcd1x1FSExtentY;
			WinDrawRectangle(&screenRect, 0);
		}
	}
	
	Q->statusScreenRedraw=true;
	if (statusCoreLoaded) PceNativeCall(corePointer, Q);
}





static void DepressColorButton(UInt16 colorCode)
{
	RectangleType 	myRect;
	RGBColorType  	prevRGB;
	Int16			penX,penY;
	Boolean			penDown=false;
	Int8			cVal;
	
	WinSetForeColorRGB(&redRGB,&prevRGB);

	switch (colorCode)
	{
		case colorBackRed:
		case colorBackGreen:
		case colorBackBlue:
		case colorPixelRed:
		case colorPixelGreen:
		case colorPixelBlue:
			do 
			{
				EvtGetPen(&penX,&penY,&penDown);
				if (statusPortraitViaLandscape)
					penY=(penX*2)-colorDialogOriginY;
				else
					penY=(penY*2)-colorDialogOriginY;
				if ((penY>=45) && (penY<=108)) cVal=108-penY;
				if (penY<45) cVal=63;
				if (penY>108) cVal=0;
				switch (colorCode)
				{
					case colorBackRed:
						tempBackColor.r=(cVal>>1)<<3;
						break;
					case colorBackGreen:
						tempBackColor.g=cVal<<2;
						break;
					case colorBackBlue:
						tempBackColor.b=(cVal>>1)<<3;
						break;
					case colorPixelRed:
						tempPixelColor.r=(cVal>>1)<<3;
						break;
					case colorPixelGreen:
						if (cVal==63) cVal=62;      // Don't allow pure white pixel color
						tempPixelColor.g=cVal<<2;  // it interferes with rotated text algorithm.
						break;
					case colorPixelBlue:
						tempPixelColor.b=(cVal>>1)<<3;
						break;
					default:break;
				}
				UpdateColorBars();
				ArmRedisplay();					
			} while (penDown);
			break;
		case colorDefault:
		case colorCancel:
		case colorOK:
			WinSetBackColorRGB(&greyRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = colorLayout[colorCode].upperLeftX+colorDialogOriginX;
			myRect.topLeft.y = colorLayout[colorCode].upperLeftY+colorDialogOriginY;
			myRect.extent.x = colorLayout[colorCode].lowerRightX-colorLayout[colorCode].upperLeftX+1;
			myRect.extent.y = colorLayout[colorCode].lowerRightY-colorLayout[colorCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,10);
			WinSetDrawMode(winPaint);
			WinSetBackColorRGB(&backRGB,NULL);
			break;
		default:break;
	}
	
	WinSetForeColorRGB(&prevRGB,NULL);
}


static void ReleaseColorButton(UInt16 colorCode)
{
	RectangleType myRect;
	RGBColorType  prevRGB;
	
	WinSetForeColorRGB(&redRGB,&prevRGB);

	switch (colorCode)
	{
		case colorDefault:
		case colorCancel:
		case colorOK:
			WinSetBackColorRGB(&greyRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = colorLayout[colorCode].upperLeftX+colorDialogOriginX;
			myRect.topLeft.y = colorLayout[colorCode].upperLeftY+colorDialogOriginY;
			myRect.extent.x = colorLayout[colorCode].lowerRightX-colorLayout[colorCode].upperLeftX+1;
			myRect.extent.y = colorLayout[colorCode].lowerRightY-colorLayout[colorCode].upperLeftY+1;
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
 * FUNCTION:    ColorSelectorScreen
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
void ColorSelectorScreen(void)
{
	RectangleType 	copyRect;
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;
	Coord 			picWidth,picHeight;
	Int16			penX,penY;
	Boolean			penDown=false;
	int				applyChangesOrCancel=0;
	EventType		event;
	UInt16			colorCode;
	UInt16			colorReleasePending=noColorButton;
	Err				error;
	Boolean			revertScreen=false;
	WinHandle		colorBackStore;
	
	
	MemMove(&tempPixelColor,statusPixelColor,sizeof(RGBColorType));
	MemMove(&tempBackColor,statusBackColor,sizeof(RGBColorType));
	
	if ((optionThreeTwentySquared)&&(statusDisplayMode2x2))
	{
		toggle_lcd(); // put calc screen in 1x1 mode
		revertScreen=true;
	}
		
	HidePanel();
	
	copyRect.topLeft.x = 0;
	copyRect.topLeft.y = 0;
	copyRect.extent.x = 320;
	copyRect.extent.y = ((optionThreeTwentySquared)?320:450);
	P48WinCopyRectangle(mainDisplayWindow,backStoreWindow,&copyRect,0,0,winPaint);

redrawColors:
	picHandle = DmGetResource(bitmapRsc, ColorSelectorBitmapFamily);
	picBitmap = MemHandleLock(picHandle);
	BmpGetDimensions(picBitmap,&picWidth,&picHeight,NULL);
	if (statusPortraitViaLandscape)
		colorBackStore = WinCreateOffscreenWindow(picHeight,picWidth,nativeFormat,&error);
	else
		colorBackStore = WinCreateOffscreenWindow(picWidth,picHeight,nativeFormat,&error);
	if (!error) 
	{
		WinSetDrawWindow(colorBackStore);
		FastDrawBitmap(picBitmap,0,0);
	}
	else
	{
		WinSetDrawWindow(mainDisplayWindow);
		FastDrawBitmap(picBitmap,colorDialogOriginX,colorDialogOriginY);
	}
	WinSetDrawWindow(mainDisplayWindow);

	if (!error) 
	{
		if (statusPortraitViaLandscape)
			CrossFade(colorBackStore,mainDisplayWindow,colorDialogOriginY,screenHeight-colorDialogOriginX-picWidth,true,128);
		else
			CrossFade(colorBackStore,mainDisplayWindow,colorDialogOriginX,colorDialogOriginY,true,128);
	}
	
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
	if (!error) WinDeleteWindow(colorBackStore,false);	

	
	UpdateColorBars();
	if (optionTarget==HP48SX)
		Q->lcdContrastLevel = ByteSwap16(11);
	else
		Q->lcdContrastLevel = ByteSwap16(14);
	ArmRedisplay();					

	EvtFlushPenQueue();
	do
	{
		WinSetCoordinateSystem(kCoordinatesStandard);
        EvtGetEvent(&event, evtWaitForever);
		WinSetCoordinateSystem(kCoordinatesNative);

		if (event.eType!=nilEvent)
		{
			//process event
			if (event.eType==penDownEvent)
			{
				EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
				colorCode = TranslatePenToColor(penX,penY);
				if (colorCode != noColorButton)
				{
					DepressColorButton(colorCode);
					colorReleasePending = colorCode;
					continue;
				}
				else
				{
					colorReleasePending = noColorButton;
				}
			}
			if (event.eType==penUpEvent)
			{
				if (colorReleasePending != noColorButton)
				{
					EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
					colorCode = TranslatePenToColor(penX,penY);
					ReleaseColorButton(colorReleasePending);
					
					if (colorReleasePending == colorCode)
					{
						switch(colorReleasePending)
						{
							case colorCancel:
								applyChangesOrCancel=1;
								break;
							case colorOK:
								applyChangesOrCancel=2;
								break;
							case colorDefault:
								switch (optionTarget)
								{
									case HP48SX:
									case HP49G:
										tempPixelColor.r=0;
										tempPixelColor.g=0;
										tempPixelColor.b=0;
										tempBackColor.r=144;
										tempBackColor.g=152;
										tempBackColor.b=120;
										break;
									case HP48GX:
										tempPixelColor.r=0;
										tempPixelColor.g=0;
										tempPixelColor.b=80;
										tempBackColor.r=120;
										tempBackColor.g=132;
										tempBackColor.b=96;
										break;
									default:break;
								}
								UpdateColorBars();
								ArmRedisplay();					
								break;
							default: break;
						}
						continue;
					}
				}
			}
			
			if (event.eType==appStopEvent)  // respond to app-switching events.
			{
				DepressColorButton(colorCancel);
				SysTaskDelay(10);
				ReleaseColorButton(colorCancel);
				Q->statusQuit=true;
				applyChangesOrCancel=1;
				EvtAddEventToQueue(&event);  // put app stop back on queue
				continue;
			}

			if (event.eType==keyDownEvent)
			{
				if (event.data.keyDown.chr == chrLineFeed)
				{
					DepressColorButton(colorOK);
					SysTaskDelay(10);
					ReleaseColorButton(colorOK);
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
				goto redrawColors;
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
		MemMove(statusPixelColor,&tempPixelColor,sizeof(RGBColorType));
		MemMove(statusBackColor,&tempBackColor,sizeof(RGBColorType));		
	}
	else
	{
		MemMove(&tempPixelColor,statusPixelColor,sizeof(RGBColorType));
		MemMove(&tempBackColor,statusBackColor,sizeof(RGBColorType));		
	}
		
	ArmRedisplay();
	
	copyRect.topLeft.x = 0;
	copyRect.topLeft.y = 0;
	copyRect.extent.x = 320;
	copyRect.extent.y = 150;  // only resave the LCD screen, the rest hasn't changed
	P48WinCopyRectangle(mainDisplayWindow,backStoreWindow,&copyRect,0,0,winPaint);
	
	EvtFlushPenQueue();
	
	CrossFade(backStoreWindow,mainDisplayWindow,0,0,false,128);
	
	ShowPanel();
	
	if (revertScreen)
		toggle_lcd();
}




/*************************************************************************\
*
*  DIALOG.C
*
*  This module is responsible for presenting error and confirmation 
*  dialog boxes.
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



const objectButton ACLayout[] =  // coordinates relative to upper left of Alert/Confirm dialog
{
	{  22, 158,  85, 183, 2},		// AlertConfirmNO, scope field def: 1 = active in Alert dialog
	{ 188, 158, 251, 183, 3},		// AlertConfirmYESorOK,             2 = active in Confirm dialog
};                                  //                                  3 = active in both


static UInt16 TranslatePenToACButton(Int16 x, Int16 y, UInt8 alertOrConfim)
{
	UInt16 i;
	Int16 tmp;
	
	if (statusPortraitViaLandscape)
	{
		tmp=x;
		x=(screenHeight-1)-y;
		y=tmp;
	}
	
	for (i=0;i<maxAlertConfirmButtons;i++)
	{
		if ((x >= (ACLayout[i].upperLeftX+alertConfirmDialogOriginX)) &&
		    (x <= (ACLayout[i].lowerRightX+alertConfirmDialogOriginX)) &&
		    (y >= (ACLayout[i].upperLeftY+alertConfirmDialogOriginY)) &&
		    (y <= (ACLayout[i].lowerRightY+alertConfirmDialogOriginY)) &&
		    (alertOrConfim&ACLayout[i].scope))
		{
			return i;
		}
	}
	return noAlertConfirmButton;
}


static void DepressAlertConfirmButton(UInt16 acCode)
{
	RectangleType 	myRect;
	RGBColorType  	prevRGB;
	
	WinSetForeColorRGB(&redRGB,&prevRGB);
	WinSetBackColorRGB(&greyRGB,NULL);
	WinSetDrawMode(winSwap);
	
	myRect.topLeft.x = ACLayout[acCode].upperLeftX+alertConfirmDialogOriginX;
	myRect.topLeft.y = ACLayout[acCode].upperLeftY+alertConfirmDialogOriginY;
	myRect.extent.x = ACLayout[acCode].lowerRightX-ACLayout[acCode].upperLeftX+1;
	myRect.extent.y = ACLayout[acCode].lowerRightY-ACLayout[acCode].upperLeftY+1;

	P48WinPaintRectangle(&myRect,10);
	WinSetDrawMode(winPaint);
	WinSetBackColorRGB(&backRGB,NULL);	
	WinSetForeColorRGB(&prevRGB,NULL);
}


static void ReleaseAlertConfirmButton(UInt16 acCode)
{
	RectangleType myRect;
	RGBColorType  prevRGB;
	
	WinSetForeColorRGB(&redRGB,&prevRGB);
	WinSetBackColorRGB(&greyRGB,NULL);
	WinSetDrawMode(winSwap);
			
	myRect.topLeft.x = ACLayout[acCode].upperLeftX+alertConfirmDialogOriginX;
	myRect.topLeft.y = ACLayout[acCode].upperLeftY+alertConfirmDialogOriginY;
	myRect.extent.x = ACLayout[acCode].lowerRightX-ACLayout[acCode].upperLeftX+1;
	myRect.extent.y = ACLayout[acCode].lowerRightY-ACLayout[acCode].upperLeftY+1;
			
	P48WinPaintRectangle(&myRect,10);
	WinSetDrawMode(winPaint);
	WinSetForeColorRGB(&prevRGB,NULL);
	WinSetBackColorRGB(&backRGB,NULL);
}




/*************************************************************************\
*
*  FUNCTION:  Int8 OpenAlertConfirmDialog(UInt8 alertOrConfirm,DmResID stringID,Boolean skipSweep)
*
\*************************************************************************/
Int8 OpenAlertConfirmDialog(dialogType alertOrConfirm, DmResID stringID, Boolean skipSave)
{
	RectangleType 	opRect;
    EventType 		event;
	Int16 			penX, penY;
	Boolean 		penDown;
	RGBColorType 	prevRGB,prevRGB2;
	MemHandle 		textHandle;
	char			*textPtr,*movableTextPtr;
	UInt16			lineNumber,stringLength;
	Int16			wrapLength;
	UInt16			acCode;
	UInt16			acReleasePending=noAlertConfirmButton;
	int				applyChangesOrCancel=0;
	int				fileIndex=0;
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;
	WinHandle		dialogBackStore;
	WinHandle		tempWinStore;
	Err				error;
	Coord 			picWidth,picHeight;
	Boolean			revertScreen=false;
	Boolean			reshowPanel=false;
	
	if ((optionThreeTwentySquared)&&(statusDisplayMode2x2))
	{
		toggle_lcd(); // put calc screen in 1x1 mode
		revertScreen=true;
	}

	if (!statusPanelHidden) 
	{
		HidePanel();
		reshowPanel=true;
	}
	
	if (!skipSave)
	{
		// save area directly behind dialog window in new offscreen window
		if (statusPortraitViaLandscape)
			dialogBackStore = WinCreateOffscreenWindow(alertConfirmDialogExtentY,alertConfirmDialogExtentX,nativeFormat,&error);
		else
			dialogBackStore = WinCreateOffscreenWindow(alertConfirmDialogExtentX,alertConfirmDialogExtentY,nativeFormat,&error);
		if (error) return -1;
redrawdialogwithsave:
		opRect.topLeft.x=alertConfirmDialogOriginX;
		opRect.topLeft.y=alertConfirmDialogOriginY;
		opRect.extent.x=alertConfirmDialogExtentX;
		opRect.extent.y=alertConfirmDialogExtentY;
		P48WinCopyRectangle(mainDisplayWindow,dialogBackStore,&opRect,0,0,winPaint);
	}
	
redrawdialog:
	WinSetForeColorRGB(&greyRGB,&prevRGB);
	WinSetBackColorRGB(&greyRGB,&prevRGB2);
	WinSetTextColorRGB(&blackRGB,NULL);

	
	// Draw dialog box...
	picHandle = DmGetResource(bitmapRsc, ((alertOrConfirm==1)?AlertDialogBitmapFamily:ConfirmDialogBitmapFamily));
	picBitmap = MemHandleLock(picHandle);
	BmpGetDimensions(picBitmap,&picWidth,&picHeight,NULL);
	if (statusPortraitViaLandscape)
		tempWinStore = WinCreateOffscreenWindow(picHeight,picWidth,nativeFormat,&error);
	else
		tempWinStore = WinCreateOffscreenWindow(picWidth,picHeight,nativeFormat,&error);
	if (error) 
	{
		WinSetDrawWindow(mainDisplayWindow);
		FastDrawBitmap(picBitmap,alertConfirmDialogOriginX,alertConfirmDialogOriginY);
	}
	else
	{
		WinSetDrawWindow(tempWinStore);
		FastDrawBitmap(picBitmap,0,0);
		WinSetDrawWindow(mainDisplayWindow);
		if (statusPortraitViaLandscape)
			CrossFade(tempWinStore,mainDisplayWindow,alertConfirmDialogOriginY,screenHeight-alertConfirmDialogOriginX-picWidth,true,128);
		else
			CrossFade(tempWinStore,mainDisplayWindow,alertConfirmDialogOriginX,alertConfirmDialogOriginY,true,128);
		WinDeleteWindow(tempWinStore,false);
	}
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);

	WinSetDrawWindow(mainDisplayWindow);
	
	
	textHandle = DmGetResource(strRsc, stringID);
	textPtr = MemHandleLock(textHandle);
	
	stringLength = StrLen(textPtr);
	
	if (stringLength>126)
		FntSetFont(stdFont);
	else
		FntSetFont(boldFont);
		
	movableTextPtr = textPtr;
	lineNumber=0;
	do 
	{
		wrapLength=FntWordWrap(movableTextPtr,acDialogTextWidth);
		DrawCharsExpanded(movableTextPtr,wrapLength,acDialogTextOriginX,acDialogTextOriginY+(FntLineHeight()*lineNumber), 0, winPaint, DOUBLE_DENSITY);
		lineNumber++;
		movableTextPtr+=wrapLength;
	} while (((UInt32)movableTextPtr)<((UInt32)textPtr+stringLength));
	
	MemHandleUnlock(textHandle);
	DmReleaseResource (textHandle);
	
	 	
	WinSetForeColorRGB(&prevRGB,NULL);
	WinSetBackColorRGB(&prevRGB2,NULL);
	
	EvtFlushPenQueue();	
    do 
    {
		WinSetCoordinateSystem(kCoordinatesStandard);
        EvtGetEvent(&event, evtWaitForever);
		WinSetCoordinateSystem(kCoordinatesNative);
        
        if (event.eType==penDownEvent)
        {
        	EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
			acCode = TranslatePenToACButton(penX,penY,alertOrConfirm);
			if (acCode != noAlertConfirmButton)
			{
				DepressAlertConfirmButton(acCode);
				acReleasePending = acCode;
				continue;
			}
			else
			{
				acReleasePending = noAlertConfirmButton;
			}
		}
        if (event.eType==penUpEvent)
		{
			if (acReleasePending != noAlertConfirmButton)
			{
				EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
				acCode = TranslatePenToACButton(penX,penY,alertOrConfirm);
				ReleaseAlertConfirmButton(acReleasePending);
					
				if (acReleasePending == acCode)
				{
					switch(acReleasePending)
					{
						case AlertConfirmNO:
							applyChangesOrCancel=1;
							break;
						case AlertConfirmYESorOK:
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
			if (alertOrConfirm==alert)
				DepressAlertConfirmButton(AlertConfirmYESorOK);
			else
				DepressAlertConfirmButton(AlertConfirmNO);
			SysTaskDelay(10);
			if (alertOrConfirm==alert)
				ReleaseAlertConfirmButton(AlertConfirmYESorOK);
			else
				ReleaseAlertConfirmButton(AlertConfirmNO);
			Q->statusQuit=true;
			EvtAddEventToQueue(&event);  // put app stop back on queue
			applyChangesOrCancel=1;
			continue;
		}

		if (event.eType==keyDownEvent)
		{
			if (event.data.keyDown.chr == chrLineFeed)
			{
				DepressAlertConfirmButton(AlertConfirmYESorOK);
				SysTaskDelay(10);
				ReleaseAlertConfirmButton(AlertConfirmYESorOK);
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

		if ((event.eType==volumeUnmountedEvent) && (statusRUN))
		{
			if (!statusInternalFiles)
			{
				if (!statusInUnmountDialog)
				{
					if (event.data.generic.datum[0]==msVolRefNum)
					{
						// Restart dialog with volume unmounted string
						EvtFlushPenQueue();
						
						statusInUnmountDialog = true;
						
						if (OpenAlertConfirmDialog(alert,pleaseReinsertCardString,true))
							statusVolumeMounted=false;
						else
							statusVolumeMounted=true;
						statusInUnmountDialog = false;	
						
						goto redrawdialog;
					}			
				}
				continue;
			}
		}

		if (event.eType==volumeMountedEvent)
		{
			if (!statusInternalFiles)
			{
				CheckForRemount();
				if ((statusVolumeMounted) && (statusInUnmountDialog))
				{
					DepressAlertConfirmButton(AlertConfirmYESorOK);
					SysTaskDelay(10);
					ReleaseAlertConfirmButton(AlertConfirmYESorOK);
					applyChangesOrCancel=1;
				}
				continue;
			}
		}
		
		if (event.eType==winDisplayChangedEvent)
		{
			//EvtFlushPenQueue();
			if (CheckDisplayTypeForGUIRedraw())
			{
				GUIRedraw();
				goto redrawdialogwithsave;
			}
			else
			{
				goto redrawdialog;
			}
		}
		
    	WinSetCoordinateSystem(kCoordinatesStandard);
        if (! SysHandleEvent(&event))
        {
			FrmDispatchEvent(&event);
        }
        WinSetCoordinateSystem(kCoordinatesNative);
    } 
    while (!applyChangesOrCancel);

	
	EvtFlushPenQueue();
			
	if (!skipSave)
	{
		if (statusPortraitViaLandscape)
			CrossFade(dialogBackStore,mainDisplayWindow,alertConfirmDialogOriginY,screenHeight-alertConfirmDialogOriginX-alertConfirmDialogExtentX,false,128);
		else
			CrossFade(dialogBackStore,mainDisplayWindow,alertConfirmDialogOriginX,alertConfirmDialogOriginY,false,128);
		WinDeleteWindow(dialogBackStore,false);
	}
	
	if (reshowPanel) ShowPanel();
	
	if (revertScreen)
		toggle_lcd();

	if (applyChangesOrCancel==1)
		return 0;
		
	return 1;
}

/*************************************************************************\
*
*  IPSPANEL.C
*
*  This module is responsible for handling the IPS / MHz monitor
*
*  (c)2003 by Robert Hildinger
*
\*************************************************************************/

#include <PalmOS.h>
#include "Power48Rsc.h"
#include <SonyCLIE.h>
#include "Power48.h"
#include "Power48_core.h"
#include <PceNativeCall.h>

UInt32 lastInstCount=0;
UInt32 lastCycleCount=0;

const RGBColorType grey1RGB={0,136,136,136};
const RGBColorType grey2RGB={0,32,32,32};


/***********************************************************************
 *
 * FUNCTION:    InitializePanelDisplay
 *
 *
 ***********************************************************************/
 
void InitializePanelDisplay(Boolean total)
{
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;
	Err				error;
	RectangleType	copyRect;
	
	if (total)
	{
		// Load up speed monitor images
		if (statusPortraitViaLandscape)
			digitsWindow = OverwriteWindow(digitsHeight,digitsTotalWidth,nativeFormat,&error,digitsWindow);
		else
			digitsWindow = OverwriteWindow(digitsTotalWidth,digitsHeight,nativeFormat,&error,digitsWindow);
		
		if (error)
		{
			statusPanelHidden=true;
			optionDisplayMonitor=false;
			return;
		}
		
		if (statusPortraitViaLandscape)
			panelWindow = OverwriteWindow(panelHeight,panelWidth,nativeFormat,&error,panelWindow);
		else
			panelWindow = OverwriteWindow(panelWidth,panelHeight,nativeFormat,&error,panelWindow);

		if (error)
		{
			WinDeleteWindow(digitsWindow,false);
			statusPanelHidden=true;
			optionDisplayMonitor=false;
			return;
		}
	}
	
	WinSetDrawWindow(digitsWindow);
	picHandle = DmGetResource(bitmapRsc, digitsSmallBitmapFamily);
	picBitmap = MemHandleLock(picHandle);
	FastDrawBitmap(picBitmap,0,0);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
		
	WinPushDrawState();
	WinSetDrawMode(winSwap);
	WinSetBackColorRGB(&whiteRGB,NULL);
	if (optionPanelType==MHZ)
		WinSetForeColorRGB(&lightBlueRGB,NULL);
	else
		WinSetForeColorRGB(&lightGreenRGB,NULL);
		
	copyRect.topLeft.x=0;
	copyRect.topLeft.y=0;
	copyRect.extent.x=digitsTotalWidth;
	copyRect.extent.y=digitsHeight;
	P48WinPaintRectangle(&copyRect,0);
	WinPopDrawState();
	
	WinSetDrawWindow(panelWindow);
	picHandle = DmGetResource(bitmapRsc, (optionPanelType==MHZ)?saturnMHzBitmapFamily:saturnMIPSBitmapFamily);
	picBitmap = MemHandleLock(picHandle);
	FastDrawBitmap(picBitmap,0,0);
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
	
	WinSetDrawWindow(mainDisplayWindow);
	
	lastInstCount=ByteSwap32(Q->instCount);
	lastCycleCount=ByteSwap32(Q->cycleCount);
}

/***********************************************************************
 *
 * FUNCTION:    UpdatePanelDisplay
 *
 *
 ***********************************************************************/

void UpdatePanelDisplay(void)
{
	RectangleType	copyRect,eraseRect;
	Int16			xpos;
	Int16			i;
	Int16			digits[4];
	UInt32			instCount,cycleCount;
	UInt32			ips,cps;
	//float			countDelta;
	Int16			numDigs,decPos;
	UInt32			initialDivisor;
	
	
	// don't let panel stray beyond boundaries
	if (optionPanelOpen)
		optionPanelX=displayExtX-panelWidth;
	else
		optionPanelX=displayExtX-panelTabWidth;
	if (optionPanelY > (displayExtY-panelHeight)) optionPanelY=(displayExtY-panelHeight);
	if (optionPanelY < 0) optionPanelY=0;
	
	
	
	if (optionPanelType==MIPS)
	{
		// Use this code is TICKS_PER_UPDATE is not equal to 100
		//instCount = ByteSwap32(Q->instCount);
		//countDelta = (instCount-prevInstCount)*100.0;	
		//ips = (UInt32)(countDelta / (float)TICKS_PER_UPDATE);
		instCount = ByteSwap32(Q->instCount);
		ips = instCount-lastInstCount;
		
		digits[0]=0;
		if (ips>10000000)
		{
			numDigs=4;
			decPos=1;
			initialDivisor=10000;
		}
		else if (ips>1000000)
		{
			numDigs=4;
			decPos=0;
			initialDivisor=1000;
		}
		else
		{
			numDigs=3;
			decPos=0;
			initialDivisor=1000;
		}
		
		ips=ips/initialDivisor;
		
		for (i=0;i<numDigs;i++)
		{
			digits[3-i]=ips%10;
			ips=ips/10;
		}
		
		lastInstCount = instCount;
	}
	else
	{
		// Use this code is TICKS_PER_UPDATE is not equal to 100
		//cycleCount = ByteSwap32(Q->cycleCount);
		//countDelta = (cycleCount-prevCycleCount)*100.0;
		//cps = (UInt32)(countDelta / (float)TICKS_PER_UPDATE);
		cycleCount = ByteSwap32(Q->cycleCount);
		//cps = (cycleCount-prevCycleCount)*1.875;
		cps = (cycleCount-lastCycleCount)<<1;
		cps = (cps*15)>>4;
		
		digits[0]=0;
		if      (cps>100000000)
		{
			numDigs=4;
			decPos=2;
			initialDivisor=100000;
		}
		else if (cps>10000000)
		{
			numDigs=4;
			decPos=1;
			initialDivisor=10000;
		}
		else if (cps>1000000)
		{
			numDigs=4;
			decPos=0;
			initialDivisor=1000;
		}
		else
		{
			numDigs=3;
			decPos=0;
			initialDivisor=1000;
		}
		
		cps=cps/initialDivisor;
		
		for (i=0;i<numDigs;i++)
		{
			digits[3-i]=cps%10;
			cps=cps/10;
		}
		
		lastCycleCount = cycleCount;
	}
	
	
	
	xpos=digitsOffsetX;

	WinSetDrawWindow(panelWindow);
	WinSetForeColorRGB(&panelRGB,NULL);
	for (i=0;i<4;i++)
	{		
		copyRect.topLeft.x=digitsWidth*digits[i];
		copyRect.topLeft.y=0;
		copyRect.extent.x=digitsWidth;
		copyRect.extent.y=digitsHeight;
		
		eraseRect.topLeft.x=xpos;
		eraseRect.topLeft.y=digitsOffsetY;
		eraseRect.extent.x=digitsWidth+digitsSpace;
		eraseRect.extent.y=digitsHeight;
		
		P48WinDrawRectangle(&eraseRect,0);
		P48WinCopyRectangle(digitsWindow,panelWindow,&copyRect,xpos,digitsOffsetY,winOverlay);

		xpos+=digitsWidth+digitsSpace;
		if (i==decPos) 
		{
			copyRect.topLeft.x=digitsWidth*10;
			copyRect.topLeft.y=0;
			copyRect.extent.x=digitsPeriodWidth;
			copyRect.extent.y=digitsHeight;
			
			eraseRect.topLeft.x=xpos;
			eraseRect.topLeft.y=digitsOffsetY;
			eraseRect.extent.x=digitsWidth+digitsSpace;
			eraseRect.extent.y=digitsHeight;
			
			P48WinDrawRectangle(&eraseRect,0);
			P48WinCopyRectangle(digitsWindow,panelWindow,&copyRect,xpos,digitsOffsetY,winOverlay);
			xpos+=digitsPeriodWidth+digitsSpace;
		}
	}
	WinSetDrawWindow(mainDisplayWindow);

	copyRect.topLeft.x = 0;
	copyRect.topLeft.y = 0;
	copyRect.extent.x = displayExtX-optionPanelX;
	copyRect.extent.y = panelHeight;
	
	if (optionDisplayMonitor)
		P48WinCopyRectangle(panelWindow,mainDisplayWindow,&copyRect,optionPanelX,optionPanelY,winOverlay);
}


/***********************************************************************
 *
 * FUNCTION:    HidePanel
 *
 ***********************************************************************/
void HidePanel(void)
{
	RectangleType copyRect;
	
	if (optionDisplayMonitor)
	{
		copyRect.topLeft.x = optionPanelX;
		copyRect.topLeft.y = optionPanelY;
		copyRect.extent.x = displayExtX-optionPanelX;
		copyRect.extent.y = panelHeight;
		P48WinCopyRectangle(backStoreWindow,mainDisplayWindow,&copyRect,optionPanelX,optionPanelY,winPaint);
	}
	statusPanelHidden=true;
}


/***********************************************************************
 *
 * FUNCTION:    ShowPanel
 *
 ***********************************************************************/
void ShowPanel(void)
{
	RectangleType copyRect;
	
	if (optionDisplayMonitor)
	{
		copyRect.topLeft.x = 0;
		copyRect.topLeft.y = 0;
		copyRect.extent.x = displayExtX-optionPanelX;
		copyRect.extent.y = panelHeight;
		P48WinCopyRectangle(panelWindow,mainDisplayWindow,&copyRect,optionPanelX,optionPanelY,winOverlay);
		statusPanelHidden=false;
	}
	else
	{
		statusPanelHidden=true;
	}
}


/***********************************************************************
 *
 * FUNCTION:    HandlePanelPress
 *
 ***********************************************************************/

void HandlePanelPress(Coord x, Coord y)
{
	Boolean			penDown;
	Int16			snapPointX[MAX_PANELS];
	Int16			i;
	Int16 			refY,newY,oldY;
	Int16 			refX,newX,oldX;
	Boolean			stopPolling=false;
	Int16			deltaX=0;
	Int16			finishX=9999;
	Boolean			continueLoop=true;
	Boolean			firstTouch=true;
	Int16			firstY;
	RectangleType	copyRect;
	Int16 			tmp;
	
		
	if (statusPortraitViaLandscape) { tmp=x; x=(screenHeight-1)-y; y=tmp; }
	
	if (x>(optionPanelX+panelTabWidth))
	{
		// User pressed panel, so switch display
		do { EvtGetPenNative(WinGetDisplayWindow(),&x,&y,&penDown); } while(penDown);
		
		if (statusPortraitViaLandscape)
		{
			tmp=x;
			x=(screenHeight-1)-y;
			y=tmp;
		}
		
		//check to see if user let up while still in panel, if so switch reading
		if ((x >= optionPanelX) &&
			(x <= (optionPanelX+panelWidth)) &&
			(y >= optionPanelY) &&
			(y <= (optionPanelY+panelHeight)))
		{
			if (optionPanelType==MHZ)
				optionPanelType=MIPS;
			else
				optionPanelType=MHZ;
			InitializePanelDisplay(false);
			UpdatePanelDisplay();
		}
	}
	else
	{		
		// User pressed tab
		
		copyRect.topLeft.x = optionPanelX;
		copyRect.topLeft.y = optionPanelY;
		copyRect.extent.x = displayExtX-optionPanelX;
		copyRect.extent.y = panelHeight;
		P48WinCopyRectangle(backStoreWindow,mainDisplayWindow,&copyRect,optionPanelX,optionPanelY,winPaint);
			
		copyRect.topLeft.x = 0;
		copyRect.topLeft.y = 0;
		copyRect.extent.x = 320;
		copyRect.extent.y = 150;  // only resave the LCD screen, the rest hasn't changed
		P48WinCopyRectangle(mainDisplayWindow,backStoreWindow,&copyRect,0,0,winPaint);
		
		// Highlight tab
		WinSetDrawWindow(panelWindow);
		WinPushDrawState();
		WinSetDrawMode(winSwap);
		WinSetBackColorRGB(&grey1RGB,NULL);
		WinSetForeColorRGB(&grey2RGB,NULL);
		copyRect.topLeft.x=0;
		copyRect.topLeft.y=0;
		copyRect.extent.x=panelTabWidth;
		copyRect.extent.y=panelHeight;
		P48WinPaintRectangle(&copyRect,0);
		WinPopDrawState();
		WinSetDrawWindow(mainDisplayWindow);
		
		
		copyRect.topLeft.x = 0;
		copyRect.topLeft.y = 0;
		copyRect.extent.x = displayExtX-optionPanelX;
		copyRect.extent.y = panelHeight;
		P48WinCopyRectangle(panelWindow,mainDisplayWindow,&copyRect,optionPanelX,optionPanelY,winOverlay);

		snapPointX[0]=displayExtX-panelTabWidth;
		snapPointX[1]=displayExtX-panelWidth;

		refX=x;
		refY=y;
		
		oldX=optionPanelX;
		oldY=optionPanelY;
		firstY=optionPanelY;
		
		do 
		{
			if (!stopPolling)
			{
				EvtGetPenNative(WinGetDisplayWindow(),&x,&y,&penDown);
				
				if (statusPortraitViaLandscape) { tmp=x; x=(screenHeight-1)-y; y=tmp; }

				
				// get new position for panel taking into account the relative position of the original pen
				newX = optionPanelX + (x-refX);
				newY = optionPanelY + (y-refY);
			}
			else
			{
				newX+=deltaX;
				SysTaskDelay(1);
			}
			
			// don't let panel stray beyond boundaries
			if (newX < (displayExtX-panelWidth)) newX=(displayExtX-panelWidth);
			if (newX > (displayExtX-panelTabWidth)) newX=(displayExtX-panelTabWidth);
			if (newY < 0) newY=0;
			if (newY > (displayExtY-panelHeight)) newY=(displayExtY-panelHeight);
			
			if (!penDown) 
			{
				// user has raised the pen, so move panel to nearest boundary
				stopPolling=true;  // stop accepting pen input
				penDown=true;      // don't come back into this code on next go 'round
				
				// Unhighlight tab
				WinSetDrawWindow(panelWindow);
				WinPushDrawState();
				WinSetDrawMode(winSwap);
				WinSetBackColorRGB(&grey1RGB,NULL);
				WinSetForeColorRGB(&grey2RGB,NULL);
				copyRect.topLeft.x=0;
				copyRect.topLeft.y=0;
				copyRect.extent.x=panelTabWidth;
				copyRect.extent.y=panelHeight;
				P48WinPaintRectangle(&copyRect,0);
				WinPopDrawState();
				WinSetDrawWindow(mainDisplayWindow);
				
				for (i=0;i<(MAX_PANELS-1);i++)
				{
					if ((newX>=snapPointX[i+1]) && (newX<=snapPointX[i]))
					{
						// we're between two snap points - go to closest one
						if ((newX-snapPointX[i+1])>(snapPointX[i]-newX))
						{
							// closer to snapPoint[i]
							deltaX=1;
							finishX=snapPointX[i];
						}
						else
						{
							// closer to snapPoint[i+1]
							deltaX=-1;
							finishX=snapPointX[i+1];
						}
						break;
					}
				}
				if (deltaX==0) continueLoop=false;
			}
			
			if (!stopPolling)
			{
				
				// Don't start moving in y direction until already 4 pixels into it, then allow full freedom
				if (firstTouch)
				{
					if ((newY>(firstY-4)) && (newY<(firstY+4)))
						newY=firstY;
					else
						firstTouch=false;
				}
				
				// snap to panel boundary if within SNAP_WIDTH pixels
				for (i=0;i<MAX_PANELS;i++)
				{
					if ((newX>(snapPointX[i]-SNAP_WIDTH)) && (newX<(snapPointX[i]+SNAP_WIDTH)))
					{
						newX=snapPointX[i];
						break;
					}
				}
				
				// if no actual position change then don't update
				if ((oldY==newY) && (oldX==newX)) continue;
			}
		
			copyRect.topLeft.x = oldX;
			copyRect.topLeft.y = oldY;
			copyRect.extent.x = displayExtX-oldX;
			copyRect.extent.y = panelHeight;
			P48WinCopyRectangle(backStoreWindow,mainDisplayWindow,&copyRect,oldX,oldY,winPaint);
			
			copyRect.topLeft.x = 0;
			copyRect.topLeft.y = 0;
			copyRect.extent.x = displayExtX-newX;
			copyRect.extent.y = panelHeight;
			P48WinCopyRectangle(panelWindow,mainDisplayWindow,&copyRect,newX,newY,winOverlay);
			
			oldX=newX;
			oldY=newY;
			
			if (newX==finishX) continueLoop=false;
			
		} while(continueLoop);
		
		optionPanelX=newX;
		optionPanelY=newY;
		
		optionPanelOpen=(optionPanelX==(displayExtX-panelWidth));
	}
	

}
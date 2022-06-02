#include <PceNativeCall.h>
#include <endianutils.h>
#include <PalmOS.h>


UInt32 PalmOS_WinGetBitmap(UInt32 **winHandle, const void *emulStateP, Call68KFuncType *call68KFuncP);
UInt32 PalmOS_BmpGetBits(UInt32 *bmpPtr, const void *emulStateP, Call68KFuncType *call68KFuncP);
void PalmOS_BmpGetDimensions(UInt32 *bmpPtr, Int16 *width, Int16 *height, UInt16 *rowBytes, 
								const void *emulStateP, Call68KFuncType *call68KFuncP);


UInt32 ARMlet_Main(const void *emulStateP, UInt8 *userData68KP, Call68KFuncType *call68KFuncP)
{
	UInt32		**fromWin;
	UInt32		**toWin;
	UInt32		**destWin;
	UInt32		offsetX; 
	UInt32		offsetY;
	UInt32		transparent;
	Int32 		i,j,k,stepping;
	UInt32 		fromVal,toVal;
	UInt32		fr,fg,fb,tr,tg,tb;
	
	UInt16		*fromBasePtr,*toBasePtr;
	UInt16		*destBasePtr;
	UInt16		*fromPtr,*toPtr;
	UInt16		*destPtr;
	UInt32		*fromBM,*toBM,*destBM;
	
	Int16		toWidth,toHeight;
	UInt16		toRowBytes;
	Int16		destWidth,destHeight;
	UInt16		destRowInt16s;
	
	UInt32		tc=0x0000ffff;
		
	fromWin 	= (UInt32**)ReadUnaligned32(userData68KP);
	toWin 		= (UInt32**)ReadUnaligned32(userData68KP+4);
	destWin 	= (UInt32**)ReadUnaligned32(userData68KP+8);
	offsetX 	= ReadUnaligned32(userData68KP+12);
	offsetY 	= ReadUnaligned32(userData68KP+16);
	transparent	= ReadUnaligned32(userData68KP+20);
	stepping	= ReadUnaligned32(userData68KP+24);
	
	if (stepping<1)
		stepping=1;
	if (stepping>256)
		stepping=256;
	

	fromWin=(UInt32**)ByteSwap32(fromWin);
	fromBM 		= (UInt32*)PalmOS_WinGetBitmap(fromWin, emulStateP, call68KFuncP);

	fromBM=(UInt32*)ByteSwap32(fromBM);
	fromBasePtr = (UInt16*)PalmOS_BmpGetBits(fromBM, emulStateP, call68KFuncP);

	toWin=(UInt32**)ByteSwap32(toWin);
	toBM 		= (UInt32*)PalmOS_WinGetBitmap(toWin, emulStateP, call68KFuncP);

	toBM=(UInt32*)ByteSwap32(toBM);
	toBasePtr 	= (UInt16*)PalmOS_BmpGetBits(toBM, emulStateP, call68KFuncP);

	destWin=(UInt32**)ByteSwap32(destWin);
	destBM 		= (UInt32*)PalmOS_WinGetBitmap(destWin, emulStateP, call68KFuncP);

	destBM=(UInt32*)ByteSwap32(destBM);
	destBasePtr = (UInt16*)PalmOS_BmpGetBits(destBM, emulStateP, call68KFuncP);

	PalmOS_BmpGetDimensions(toBM, &toWidth, &toHeight, &toRowBytes, emulStateP, call68KFuncP);
	PalmOS_BmpGetDimensions(destBM, &destWidth, &destHeight, &destRowInt16s, emulStateP, call68KFuncP);
	
	destRowInt16s>>=1;
	
	if (transparent)
	{
		for (k=stepping;k<=1024;k+=stepping)
		{
			fromPtr=fromBasePtr;
			toPtr=toBasePtr;
			destPtr=destBasePtr;
			destPtr+=(destRowInt16s*offsetY)+offsetX;
			
			for (i=0;i<toHeight;i++)
			{
				for (j=0;j<toWidth;j++)
				{
					fromVal=*fromPtr++;
						
					fb=(fromVal&0x001F);
					fg=(fromVal&0x07E0); 
					fr=(fromVal&0xF800);
						
					toVal=*toPtr++;
					
					if (toVal!=tc)
					{
						tb=(toVal&0x001F);
						tg=(toVal&0x07E0); 
						tr=(toVal&0xF800);

						fr+=(((tr-fr)*k)>>10);
						fg+=(((tg-fg)*k)>>10);
						fb+=(((tb-fb)*k)>>10);
																
						*destPtr++=(fr&0xF800)|(fg&0x07E0)|(fb&0x001F);
					}
					else
					{
						destPtr++;
					}
				}
				destPtr += (destRowInt16s-toWidth);
			}
		}
	}
	else
	{
		for (k=stepping;k<=1024;k+=stepping)
		{
			fromPtr=fromBasePtr;
			toPtr=toBasePtr;
			destPtr=destBasePtr;
			destPtr+=(destRowInt16s*offsetY)+offsetX;
			
			for (i=0;i<toHeight;i++)
			{
				for (j=0;j<toWidth;j++)
				{
					fromVal=*fromPtr++;
						
					fb=(fromVal&0x001F);
					fg=(fromVal&0x07E0); 
					fr=(fromVal&0xF800);
						
					toVal=*toPtr++;
							
					tb=(toVal&0x001F);
					tg=(toVal&0x07E0); 
					tr=(toVal&0xF800);

					fr+=(((tr-fr)*k)>>10);
					fg+=(((tg-fg)*k)>>10);
					fb+=(((tb-fb)*k)>>10);
															
					*destPtr++=(fr&0xF800)|(fg&0x07E0)|(fb&0x001F);
				}
				destPtr += (destRowInt16s-toWidth);
			}
		}
	}
	
 	return 1;
}





inline UInt32 PalmOS_WinGetBitmap(UInt32 **winHandle, const void *emulStateP, Call68KFuncType *call68KFuncP)
{
	return (call68KFuncP)(
		emulStateP,
		PceNativeTrapNo(sysTrapWinGetBitmap),
		&winHandle,
		4 | kPceNativeWantA0);
}


inline UInt32 PalmOS_BmpGetBits(UInt32 *bmpPtr, const void *emulStateP, Call68KFuncType *call68KFuncP)
{
	return (call68KFuncP)(
		emulStateP,
		PceNativeTrapNo(sysTrapBmpGetBits),
		&bmpPtr,
		4 | kPceNativeWantA0);
}


inline void PalmOS_BmpGetDimensions(UInt32 *bmpPtr, Int16 *width, Int16 *height, UInt16 *rowBytes, 
								const void *emulStateP, Call68KFuncType *call68KFuncP)
{
	UInt32 args[4];
	
	args[0] = (UInt32)bmpPtr;
	args[1] = ByteSwap32(width);
	args[2] = ByteSwap32(height);
	args[3] = ByteSwap32(rowBytes);

	(call68KFuncP)(emulStateP, PceNativeTrapNo(sysTrapBmpGetDimensions), &args, 16);
	
	*width = ByteSwap16(*width);
	*height = ByteSwap16(*height);
	*rowBytes = ByteSwap16(*rowBytes);	
}


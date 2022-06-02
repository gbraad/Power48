#include <PceNativeCall.h>
#include <endianutils.h>
#include <PalmOS.h>


UInt32 PalmOS_WinGetBitmap(UInt32 **winHandle, const void *emulStateP, Call68KFuncType *call68KFuncP);
UInt32 PalmOS_BmpGetBits(UInt32 *bmpPtr, const void *emulStateP, Call68KFuncType *call68KFuncP);
void PalmOS_BmpGetDimensions(UInt32 *bmpPtr, Int16 *width, Int16 *height, UInt16 *rowBytes, 
								const void *emulStateP, Call68KFuncType *call68KFuncP);
UInt32 PalmOS_BmpGetColortable(UInt32 *bmpPtr, const void *emulStateP, Call68KFuncType *call68KFuncP);


UInt32 ARMlet_Main(const void *emulStateP, UInt8 *userData68KP, Call68KFuncType *call68KFuncP)
{
	UInt32		*sourceBitmap;
	Int32		x;
	Int32		y;
		
	UInt8 *data;
	UInt16 *dest;
	UInt16 runCount;
	UInt32 *destBitmap;
	Int16 destWidth,destHeight;
	Int16 srcWidth,srcHeight;
	UInt16 rowCount=0;
	UInt32 rowSkip;
	UInt16 c1;
	UInt16 colorTable[256];
	UInt16 numCTEntries;
	UInt16 i;
	UInt8 index,cr,cg,cb; 
	UInt16 *ctPtr;
	UInt16 rowB;
	UInt32 landscape;
	Int32 numPixels;
	BitmapCompressionType cType;
	
	
	sourceBitmap = (UInt32*)ReadUnaligned32(userData68KP);
	destBitmap = (UInt32*)ReadUnaligned32(userData68KP+4);
	x = (Int32)ReadUnaligned32(userData68KP+8);
	y = (Int32)ReadUnaligned32(userData68KP+12);
	landscape = (UInt32)ReadUnaligned32(userData68KP+16);
	cType = (BitmapCompressionType)ReadUnaligned32(userData68KP+20);

	destBitmap =(UInt32*)ByteSwap32(destBitmap);
	dest = (UInt16*)PalmOS_BmpGetBits(destBitmap, emulStateP, call68KFuncP);
	PalmOS_BmpGetDimensions(destBitmap, &destWidth, &destHeight, &rowB, emulStateP, call68KFuncP);
	
	if (landscape)
		dest += ((Int32)destWidth*(Int32)(destHeight-1-x))+(Int32)y;
	else
		dest += ((Int32)destWidth*(Int32)y)+(Int32)x;
	
	sourceBitmap =(UInt32*)ByteSwap32(sourceBitmap);
	PalmOS_BmpGetDimensions(sourceBitmap, &srcWidth, &srcHeight, &rowB, emulStateP, call68KFuncP);
	
	if (landscape)
		rowSkip = ((Int32)destWidth*(Int32)srcWidth)+1;
	else
		rowSkip = destWidth-srcWidth;
		
	numPixels = (Int32)srcWidth*(Int32)srcHeight;
	
	data = (UInt8*)PalmOS_BmpGetColortable(sourceBitmap, emulStateP, call68KFuncP);

	// data now points to color table
	numCTEntries=(data[0]<<8)|data[1];
	data+=2;
	
	for (i=0;i<numCTEntries;i++)
	{
		index=*data++;
		cr=(*data++)>>3;
		cg=(*data++)>>2;
		cb=(*data++)>>3;
		colorTable[index]=((UInt16)cr<<11)|((UInt16)cg<<5)|cb;
	}
	ctPtr=colorTable;
	
	data = (UInt8*)PalmOS_BmpGetBits(sourceBitmap, emulStateP, call68KFuncP);
	
	if (cType==BitmapCompressionTypePackBits)
	{
		// *data now points to compressed data size indicator so skip it
		data+=4;

		if (landscape)
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

		if (landscape)
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


inline UInt32 PalmOS_BmpGetColortable(UInt32 *bmpPtr, const void *emulStateP, Call68KFuncType *call68KFuncP)
{
	return (call68KFuncP)(
		emulStateP,
		PceNativeTrapNo(sysTrapBmpGetColortable),
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


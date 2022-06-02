#include <PceNativeCall.h>
#include <endianutils.h>
#include <PalmOS.h>


UInt32 PalmOS_WinGetBitmap(UInt32 **winHandle, const void *emulStateP, Call68KFuncType *call68KFuncP);
UInt32 PalmOS_BmpGetBits(UInt32 *bmpPtr, const void *emulStateP, Call68KFuncType *call68KFuncP);
void PalmOS_BmpGetDimensions(UInt32 *bmpPtr, Int16 *width, Int16 *height, UInt16 *rowBytes, 
								const void *emulStateP, Call68KFuncType *call68KFuncP);


#define ReadUnaligned32Swapped(addr)  \
	( ((((unsigned char *)(addr))[3]) << 24) | \
	  ((((unsigned char *)(addr))[2]) << 16) | \
	  ((((unsigned char *)(addr))[1]) <<  8) | \
	  ((((unsigned char *)(addr))[0])) )
#define ReadUnaligned16Swapped(addr)  \
	( ((((unsigned char *)(addr))[1]) <<  8) | \
	  ((((unsigned char *)(addr))[0])) )


UInt32 ARMlet_Main(const void *emulStateP, UInt8 *userData68KP, Call68KFuncType *call68KFuncP)
{
	UInt8	*buf;
	UInt32	bufSize;
	UInt32	**win;
	
	UInt32 fileSize;
	UInt32 picOffset;
	UInt32 width;
	UInt32 height;
	UInt16 depth;
	UInt32 compression;
	UInt8  *imageData,*imageDataBase;
	UInt16 *winData;
	Int16 winHeight,winWidth;
	UInt32 *winBitmap;
	Int32 i,j;
	UInt32 r,g,b;
	UInt32 bytesPerRow;
	UInt32 rowModulus;
	UInt16 dummy;
	
	buf 	= (UInt8*)ReadUnaligned32(userData68KP);
	bufSize = (UInt32)ReadUnaligned32(userData68KP+4);
	win 	= (UInt32**)ReadUnaligned32(userData68KP+8);
	win =(UInt32**)ByteSwap32(win);


	fileSize=ReadUnaligned32Swapped(&buf[2]);
	picOffset=ReadUnaligned32Swapped(&buf[10]);
	width=ReadUnaligned32Swapped(&buf[18]);
	height=ReadUnaligned32Swapped(&buf[22]);
	depth=ReadUnaligned16Swapped(&buf[28]);
	compression=ReadUnaligned32Swapped(&buf[30]);
		
		
	imageDataBase=buf+picOffset;
		
	if ((depth!=24) || (compression!=0)) return 1;
		
	bytesPerRow = width*3;
	rowModulus = bytesPerRow%4;
	if (rowModulus) bytesPerRow += (4-rowModulus);
		
	winBitmap = (UInt32*)PalmOS_WinGetBitmap(win, emulStateP, call68KFuncP);
	winBitmap = (UInt32*)ByteSwap32(winBitmap);

	PalmOS_BmpGetDimensions(winBitmap, &winWidth, &winHeight, &dummy, emulStateP, call68KFuncP);
	
	winData = (UInt16*)PalmOS_BmpGetBits(winBitmap, emulStateP, call68KFuncP);
		
	if ((winWidth!=width)||(winHeight!=height)) return 1;
		
	if (bufSize<((bytesPerRow*height)+picOffset)) return 1;
		
	for (j=height-1;j>=0;j--)
	{
		imageData=imageDataBase+(j*bytesPerRow);
		for (i=0;i<width;i++)
		{
			b=(*imageData++)>>3;
			g=(*imageData++)>>2;
			r=(*imageData++)>>3;
			*winData++ = ((UInt16)r<<11)|((UInt16)g<<5)|b;
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


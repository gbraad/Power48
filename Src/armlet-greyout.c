#include <PceNativeCall.h>
#include <endianutils.h>
#include <PalmOS.h>


UInt32 ARMlet_Main(const void *emulStateP, UInt8 *userData68KP, Call68KFuncType *call68KFuncP)
{
	UInt32		**myWin;
	Int32		x;
	Int32		y;
	Int32		extentX;
	Int32		extentY;
	UInt16		rowInts;

	UInt16 *winBase;
	UInt16 valRed,valGreen,valBlue; 
	UInt16 deltaRed,deltaGreen,deltaBlue;
	UInt16 baseRed,baseGreen,baseBlue;
	UInt32 *tempBitmap;
	int i,j;
	
	winBase = (UInt16*)ReadUnaligned32(userData68KP);
	x 		= (Int32)ReadUnaligned32(userData68KP+4);
	y 		= (Int32)ReadUnaligned32(userData68KP+8);
	extentX = (Int32)ReadUnaligned32(userData68KP+12);
	extentY = (Int32)ReadUnaligned32(userData68KP+16);
	rowInts = (UInt16)ReadUnaligned32(userData68KP+20);

	baseRed = 24;  // 16 bit representation of gray dialog background
	baseGreen=48;
	baseBlue= 24;
	
	winBase += (y*rowInts)+x;
	
	for (i=0;i<extentY;i++)
	{
		for (j=0;j<extentX;j++)
		{
			valRed = *winBase;
			valBlue=valRed&0x1f;
			valRed>>=5;
			valGreen=valRed&0x3f;
			valRed>>=6;
				
			deltaRed   = ((baseRed-valRed)>>1);
			deltaBlue  = ((baseBlue-valBlue)>>1);
			deltaGreen = ((baseGreen-valGreen)>>1);
				 
			valRed=(valRed + deltaRed)&0x1f;
			valBlue=(valBlue + deltaBlue)&0x1f;
			valGreen=(valGreen + deltaGreen)&0x3f;
				
			valRed<<=6;
			valRed|=valGreen;
			valRed<<=5;
			valRed|=valBlue;
			*winBase++ = valRed;
		}
		winBase += rowInts-extentX;
	}
 	return 1;
}



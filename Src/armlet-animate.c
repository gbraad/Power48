#include <PceNativeCall.h>
#include <endianutils.h>
#include <PalmOS.h>
#include "Power48.h"


static Int32 COS[90]=
{
65536, 65376, 64898, 64103, 
62997, 61583, 59870, 57864, 
55577, 53019, 50203, 47142, 
43852, 40347, 36647, 32768, 
28729, 24550, 20251, 15854, 
11380, 6850, 2287, -2287, 
-6850, -11380, -15854, -20251, 
-24550, -28729, -32767, -36647, 
-40347, -43852, -47142, -50203, 
-53019, -55577, -57864, -59870, 
-61583, -62997, -64103, -64898, 
-65376, -65536, -65376, -64898, 
-64103, -62997, -61583, -59870, 
-57864, -55577, -53019, -50203, 
-47142, -43852, -40347, -36647, 
-32768, -28729, -24550, -20251, 
-15854, -11380, -6850, -2287, 
2287, 6850, 11380, 15854, 
20251, 24550, 28729, 32767, 
36647, 40347, 43852, 47142, 
50203, 53019, 55577, 57864, 
59870, 61583, 62997, 64103, 
64898, 65376 
};

UInt32 ARMlet_Main(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
{
	UInt16		*fromPtr,*toPtr,*destPtr;
	Int32 		i,j;
	UInt32 		fromVal,toVal;
	UInt32		fr,fg,fb,tr,tg,tb;
	Int32 		fadeLevel;
	UInt32		position;
	
	register drawInfo *Q;
	
	Q=userData68KP;

				
	fromPtr=Q->fromBasePtr;
	toPtr=Q->toBasePtr; 
	destPtr=Q->destBasePtr;
	destPtr+=(Q->destRowInt16s*Q->offsetY)+Q->offsetX;
	position=ByteSwap32(Q->position);
		
	if (Q->effectType == TYPE_ONE)
	{
		for (i=0;i<Q->toHeight;i++)
		{
			fadeLevel = (COS[position]>>8)+768;
			position++;
			if (position>=90) position=0;

			for (j=0;j<Q->toWidth;j++)
			{
				fromVal=*fromPtr++;
							
				fb=(fromVal&0x001F);
				fg=(fromVal&0x07E0); 
				fr=(fromVal&0xF800);
							
				toVal=*toPtr++;
								
				tb=(toVal&0x001F);
				tg=(toVal&0x07E0); 
				tr=(toVal&0xF800);

				fr+=(((tr-fr)*fadeLevel)>>10);
				fg+=(((tg-fg)*fadeLevel)>>10);
				fb+=(((tb-fb)*fadeLevel)>>10);
																
				*destPtr++=(fr&0xF800)|(fg&0x07E0)|(fb&0x001F);
			}
			destPtr += (Q->destRowInt16s-Q->toWidth);
		}
	}
	else if (Q->effectType == TYPE_TWO)
	{
		Q->fadeOffset=ByteSwap32(Q->fadeOffset);
		for (i=0;i<Q->toHeight;i++)
		{
			fadeLevel = (COS[position>>1]>>7)+Q->fadeOffset;
			if (fadeLevel<0) fadeLevel=0;
			
			position++;
			if (position>=180) position=0;

			for (j=0;j<Q->toWidth;j++)
			{
				fromVal=*fromPtr++;
							
				fb=(fromVal&0x001F);
				fg=(fromVal&0x07E0); 
				fr=(fromVal&0xF800);
							
				toVal=*toPtr++;
								
				tb=(toVal&0x001F);
				tg=(toVal&0x07E0); 
				tr=(toVal&0xF800);

				fr+=(((tr-fr)*fadeLevel)>>10);
				fg+=(((tg-fg)*fadeLevel)>>10);
				fb+=(((tb-fb)*fadeLevel)>>10);
																
				*destPtr++=(fr&0xF800)|(fg&0x07E0)|(fb&0x001F);
			}
			destPtr += (Q->destRowInt16s-Q->toWidth);
		}
	}
	
 	return 1;
}



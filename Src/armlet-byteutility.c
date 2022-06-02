#include <PceNativeCall.h>
#include <endianutils.h>
#include <PalmOS.h>
#include "Power48.h"



UInt32 ARMlet_Main(const void *emulStateP, UInt8 *userData68KP, Call68KFuncType *call68KFuncP)
{
	UInt8		*buf,*a,*b;
	UInt32		tSize;
	compOpType 	operation;
	UInt32		rVal,i;
	UInt32		dVal;
	UInt32		addHighBit;
	
	buf      = (UInt8*)ReadUnaligned32(userData68KP);
	tSize    = (UInt32)ReadUnaligned32(userData68KP+4);
	operation = (compOpType)ReadUnaligned32(userData68KP+8);

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
		return rVal;
	}
 	return 0;
}

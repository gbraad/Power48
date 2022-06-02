/*************************************************************************\
*
*  SATURN.C
*
*  This is the core of the emulator
*
*  (c)1995 Sebastien Carlier
*  (c)2002 Robert Hildinger
*
\*************************************************************************/
#include <PceNativeCall.h>
#include <endianutils.h>
#include <PalmOS.h>
#include "IO.h"
#include "Power48_core.h"
#include "TRACE.H"


#define INTERRUPT(t)  {Q->INTERRUPT_PROCESSING=1;rstkpush(Q->pc,Q);Q->pc=0xf;I=GetRealAddressNoIO_ARM(0xf,Q);}
#define goyes(n) GOJMP = I+n; I+=n; goto o_goyes;
#define PCHANGED      {Q->F_s[0]=Q->P;Q->F_l[1]=Q->P+1;}

#define CYCLE_COUNT 

#ifndef CYCLE_COUNT
#define TICKS(val) 
#else
#define TICKS(val) cycleCount+=val;
#endif





void ErrorPrint_arm( const char* screenText, Boolean newLine, register emulState *Q);
void EmulateBeep_ARM(register emulState *Q);
void display_clear_arm(register emulState *Q);
void adjust_contrast_arm(register emulState *Q);
void toggle_lcd_arm(register emulState *Q);
Boolean ProcessPalmEvent_arm(UInt32 timeout, register emulState *Q);
void SwapAllData_arm(register emulState *Q);
void kbd_handler_arm(UInt16 keyIndex, KHType action, register emulState *Q);
void cycleColor_arm(register emulState *Q);
UInt32 TimGetTicks_arm(register emulState *Q);
void DrawDisplayLine16x2_arm(register emulState *Q);
void DrawDisplayLine16x2_arm_PVL(register emulState *Q);
void DrawDisplayLine16x1_arm(register emulState *Q);
void draw_annunciator_arm(UInt16* lcdMemPtr, UInt16 *annunciatorMemPtr, UInt16 *backgroundMemPtr, UInt32 draw, register emulState *Q);
void change_ann(register emulState *Q) ;
UInt32 write_io_multiple_arm(UInt32 d, UInt8 *a, UInt32 s, register emulState *Q);
UInt32 read_io_multiple_arm(UInt32 d, UInt8 *a, UInt32 s, register emulState *Q);
void RecalculateDisplayParameters_arm(register emulState *Q);

#ifdef TRACE_ENABLED
void TracePrint_arm(register emulState *Q);
#endif



 


static UInt16 crc_table[16] =
{
	0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285, 0x6306, 0x7387,
	0x8408, 0x9489, 0xA50A, 0xB58B, 0xC60C, 0xD68D, 0xE70E, 0xF78F
};

static Boolean pixelMapConst[64] =
{
	0,0,0,0,
	1,0,0,0,
	0,1,0,0,
	1,1,0,0,
	0,0,1,0,
	1,0,1,0,
	0,1,1,0,
	1,1,1,0,
	0,0,0,1,
	1,0,0,1,
	0,1,0,1,
	1,1,0,1,
	0,0,1,1,
	1,0,1,1,
	0,1,1,1,
	1,1,1,1
};	
	


inline void CRC_update(UInt8 nib, register emulState *Q)
{
	Q->crc = (UInt16)((Q->crc>>4)^crc_table[(Q->crc^nib)&0xf]);
}



inline UInt32 Npack(UInt8 *a, UInt32 s) 
{
	UInt32 r = 0;

	while (s--) r = (r<<4)|a[s];
	return r;
}



inline UInt32 Npack_CRC(UInt8 *a, UInt32 s, register emulState *Q) 
{
	UInt32 r = 0;
	UInt32 i;
  
	for (i=0;i<s;i++) CRC_update(a[s],Q);
	
	while (s--) r = (r<<4)|a[s];
	
	return r;
}



inline void Nunpack(UInt8 *a, UInt32 b, UInt32 s) 
{
	UInt32 i;

	for (i=0; i<s; i++) { a[i] = (UInt8)(b&0xf); b>>=4; }
}



inline void Ncopy(UInt8 *a, UInt8 *b, UInt32 s) 
{
	while (s--) { a[s]=b[s]; }
}



inline void Nxchg(UInt8 *a, UInt8 *b, UInt32 s) 
{
	UInt8 t;

	while (s--) { t=a[s]; a[s]=b[s]; b[s]=t; }
}



inline void Ninc_5(UInt8 *a, register emulState *Q) 
{
	UInt32 i;
	UInt8 carry;

	if (Q->MODE==10)
	{
		carry=1;
		for (i=0; i<5; ++i) 
		{
			if (a[i] >= 10)	a[i] &= 0x7;

			a[i] += carry;
			carry = (a[i] >= 10);
			if (carry) a[i] -= 10;
		}
		Q->CARRY = carry;
	}
	else
	{
		for (i=0; i<5; ++i) 
		{
			a[i]++;
			if (a[i]<16) 
			{ 
				Q->CARRY = 0; 
				return; 
			}
			a[i]-=16;
		}
		Q->CARRY = 1;
	}
}



inline void Ndec_5(UInt8 *a, register emulState *Q) 
{
	UInt8 base = Q->MODE;
	UInt32 i;

	for (i=0; i<5; i++) 
	{
		a[i]--; 
		if (a[i]<16) 
		{ 
			Q->CARRY = 0; 
			return; 
		}
		a[i] += base;
	}
	Q->CARRY = 1;
}



inline void Nadd(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	UInt8 carry = 0;
	UInt8 base = Q->MODE;
	UInt32 i;

	for (i=0; i<s; ++i) 
	{
		if ((base==10) && (a[i] >= 10)) a[i] &= 0x7;
			
		a[i] += b[i]+carry;
		
		if (a[i] < base)
			carry = 0;
		else
		{ 
			a[i] -= base; 
			carry = 1; 
		}
	}
	Q->CARRY = carry;
}



inline void NFinc(UInt8 *a, UInt8 field, register emulState *Q) 
{
	UInt32 i;
	UInt8 carry;
	UInt32 offset = Q->F_s[field];
	UInt32 fieldLength = Q->F_l[field];

	if (Q->MODE==10)
	{
		carry=1;
		for (i=offset; i<(fieldLength+offset); ++i) 
		{
			if (a[i&0xf] >= 10)	a[i&0xf] &= 0x7;

			a[i&0xf] += carry;
			carry = (a[i&0xf] >= 10);
			if (carry) a[i&0xf] -= 10;
		}
		Q->CARRY = carry;
	}
	else
	{
		for (i=offset; i<(fieldLength+offset); ++i) 
		{
			a[i&0xf]++;
			if (a[i&0xf]<16) 
			{ 
				Q->CARRY = 0; 
				return; 
			}
			a[i&0xf]-=16;
		}
		Q->CARRY = 1;
	}
}



inline void NFdec(UInt8 *a, UInt8 field, register emulState *Q) 
{
	UInt8 base = Q->MODE;
	UInt32 i;
	UInt32 offset = Q->F_s[field];
	UInt32 fieldLength = Q->F_l[field];

	for (i=offset; i<(fieldLength+offset); ++i) 
	{
		a[i&0xf]--; 
		if (a[i&0xf]<16) 
		{ 
			Q->CARRY = 0; 
			return; 
		}
		a[i&0xf] += base;
	}
	Q->CARRY = 1;
}



inline void NFaddconstant(UInt8 *a, UInt8 n, UInt8 c, register emulState *Q)
{
	UInt32 i,fieldLength,offset;

	fieldLength = Q->F_l[n];   // Saturn bug
	if ((fieldLength==1) && (c>0)) fieldLength=16;
		
	offset=Q->F_s[n];
	
	a[offset] += c;
		
	for (i=offset; i<(fieldLength+offset); i++) 
	{
		a[i&0xf]++;
		if (a[i&0xf]<16) 
		{ 
			Q->CARRY = 0; 
			return; 
		}
		a[i&0xf]-=16;
	}
	Q->CARRY = 1;
}



inline void Nsub(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	UInt8 carry = 0;
	UInt8 base = Q->MODE;
	UInt32 i;

	for (i=0; i<s; ++i) 
	{
		a[i] -= b[i]+carry;
		if (a[i]>=16)
		{ 
			a[i] += base; 
			carry = 1; 
		}
		else
			carry = 0;
	}
	Q->CARRY = carry;
}



inline void Nrsub(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	UInt8 carry = 0;
	UInt8 base = Q->MODE;
	UInt32 i;

	for (i=0; i<s; ++i) 
	{
		a[i]=b[i]-(a[i]+carry);
		if (a[i]>=16)
		{ 
			a[i] += base; 
			carry = 1; 
		}
		else
			carry = 0;
	}
	Q->CARRY = carry;
}



inline void NFsubconstant(UInt8 *a, UInt8 n, UInt8 c, register emulState *Q)
{
	UInt32 i,fieldLength,offset;
		
	fieldLength = Q->F_l[n];   // Saturn bug

	if ((fieldLength==1) && (c>0)) fieldLength=16;
	
	offset=Q->F_s[n];

	a[offset] -= c;

	for (i=offset; i<(fieldLength+offset); i++) 
	{
		a[i&0xf]--; 
		if (a[i&0xf]<16) 
		{ 
			Q->CARRY = 0; 
			return; 
		}
    	a[i&0xf] += 16;
	}
	Q->CARRY = 1;
}



inline void Nand(UInt8 *a, UInt8 *b, UInt32 s) 
{
	while (s--) a[s]&=b[s];
}



inline void Nor(UInt8 *a, UInt8 *b, UInt32 s) 
{
	while (s--) a[s]|=b[s];
}



inline void Nzero(UInt8 *a, UInt32 s) 
{
	while (s--) a[s]=0;
}



inline void Nnot(UInt8 *a, UInt32 s, register emulState *Q) 
{
	UInt8 c = Q->MODE-1;

	while (s--) 
	{
		a[s]=c-a[s];
		if ((a[s]&0xf0) != 0) a[s] &= 0x7;   // handle decimal mode overflow
	}
	
	Q->CARRY = 0;
}



inline void Nneg(UInt8 *a, UInt32 s, register emulState *Q) 
{
	UInt8 c = Q->MODE-1;
	UInt32 i;

	for (i=0; i<s; i++) if (a[i]) break;
	
	if (i==s) 
	{ 
		Q->CARRY=0; 
		return; 
	} /* it was 0 */

	Q->CARRY=1;
	a[i] = Q->MODE - a[i]; /* first non-zero */
	
	if ((a[i]&0xf0) != 0) a[i] &= 0x7;   // handle decimal mode overflow
	
	for (++i; i<s; i++) 
	{
		a[i]=c-a[i];
		if ((a[i]&0xf0) != 0) a[i] &= 0x7;   // handle decimal mode overflow
	}
}



inline void Nsl(UInt8 *a, UInt32 s) 
{
	// if (a[s-1])
	//   Q->HST|=SB;
	while (--s) a[s]=a[s-1];
	(*a)=0;
}



inline void Nsr(UInt8 *a, UInt32 s, register emulState *Q) 
{
	if (*a) Q->HST|=SB;
	
	while (--s) { (*a)=a[1]; a++; }
	(*a)=0;
}



inline void Nbit0(UInt8 *a, UInt8 b) 
{
	a[b>>2]&=~(1<<(b&3));
}



inline void Nbit1(UInt8 *a, UInt8 b) 
{
	a[b>>2]|=1<<(b&3);
}



inline void Nslc(UInt8 *a, UInt32 s) 
{
	UInt8 c;

	c = a[s-1];
	while (--s) a[s] = a[s-1];
	*a = c;
}



inline void Nsrc(UInt8 *a, UInt32 s, register emulState *Q) 
{
	UInt8 c = *a;

	if (c) Q->HST|=SB;
	while (--s) { *a=a[1]; a++; }
	*a = c;
}



inline void Nsrb(UInt8 *a, UInt32 s, register emulState *Q) 
{
	if ((*a)&1) Q->HST|=SB;

	while (--s) 
	{
		(*a)>>=1;
		if (a[1]&1) (*a)|=8;
		a++;
	}
	*a>>=1;      
}



inline void Tbit0(UInt8 *a, UInt8 b, register emulState *Q) 
{
	if (a[b>>2]&(1<<(b&3)))
		Q->CARRY = 0;
	else
    	Q->CARRY = 1;
}



inline void Tbit1(UInt8 *a, UInt8 b, register emulState *Q) 
{
	if (a[b>>2]&(1<<(b&3)))
		Q->CARRY = 1;
	else
		Q->CARRY = 0;
}



inline void Te(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	while (s--) if (a[s]!=b[s]) { Q->CARRY = 0; return; }
	Q->CARRY = 1;
}



inline void Tne(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	while (s--) if (a[s]!=b[s]) { Q->CARRY=1; return; }
	Q->CARRY = 0;
}



inline void Tz(UInt8 *a, UInt32 s, register emulState *Q) 
{
	while (s--) if (a[s]!=0) { Q->CARRY = 0; return; }
	Q->CARRY = 1;
}



inline void Tnz(UInt8 *a, UInt32 s, register emulState *Q) 
{
	while (s--) if (a[s]!=0) { Q->CARRY=1; return; }
	Q->CARRY = 0;
}



inline void Ta(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	while (--s) if (a[s]!=b[s]) break;
	Q->CARRY = (a[s]>b[s]);
}



inline void Tb(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	while (--s) if (a[s]!=b[s]) break;
	Q->CARRY = (a[s]<b[s]);
}



inline void Tae(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	while (--s) if (a[s]!=b[s]) break;
	Q->CARRY = (a[s]>=b[s]);
}



inline void Tbe(UInt8 *a, UInt8 *b, UInt32 s, register emulState *Q) 
{
	while (--s) if (a[s]!=b[s]) break;
	Q->CARRY = (a[s]<=b[s]);
}









/***********************************************************************************
 *
 *  Bus Primitives
 *
 *
 *
 *
 ***********************************************************************************/


/* Memory Controllers

        48SX:            48GX              49G
        -----            ----              ---
HDW     0 IO             0 IO              0 IO
RAM     1 RAM            1 RAM             1 RAM
CE2     2 PORT 1         2 bank switcher   2 bank switcher
CE1     3 PORT 2         3 PORT 1          3 PORT 1
NCE3    4 UNUSED         4 PORT 2          4 PORT 2
ROM     5 ROM            5 ROM             5 ROM

*/




inline void config(register emulState *Q) 
{
	UInt32 d;
	
	d=Npack(&Q->C[3],2)<<12;   // keep to 0x1000 nibble boundary
		
	// Controllers are configured in the following order:
	// HDW, RAM, CE1, CE2, NCE3
	//
	if (!Q->mcIOAddrConfigured) 
	{ 
		Q->mcIOAddrConfigured = true;
		Q->mcIOBase = Npack(Q->C,5)&0xFFFC0;			// keep to 64 nibble boundary
		Q->mcIOCeil = Q->mcIOBase + 0x40;			
		return;
	}
	
	if (!Q->mcRAMAddrConfigured)
	{
		if (!Q->mcRAMSizeConfigured)	
		{
			Q->mcRAMSizeConfigured = true;
			Q->mcRAMSize = 0x100000-d;
			return;
		}
		Q->mcRAMAddrConfigured = true;
		Q->mcRAMBase = d;	
		Q->mcRAMCeil = d+Q->mcRAMSize;
		Q->mcRAMDelta = (UInt32)Q->ram-d;
		return;
	}
	
	if (!Q->mcCE1AddrConfigured)
	{
		if (!Q->mcCE1SizeConfigured)	
		{
			Q->mcCE1SizeConfigured = true;
			Q->mcCE1Size = 0x100000-d;
			return;
		}
		Q->mcCE1AddrConfigured = true;
		Q->mcCE1Base = d;	
		Q->mcCE1Ceil = d+Q->mcCE1Size;
		Q->mcCE1Delta = (UInt32)Q->ce1addr-d;
		return;
	}
	
	if (!Q->mcCE2AddrConfigured)
	{
		if (!Q->mcCE2SizeConfigured)	
		{
			Q->mcCE2SizeConfigured = true;
			Q->mcCE2Size = 0x100000-d;
			return;
		}
		Q->mcCE2AddrConfigured = true;
		Q->mcCE2Base = d;	
		Q->mcCE2Ceil = d+Q->mcCE2Size;
		Q->mcCE2Delta = (UInt32)Q->ce2addr-d;
		return;
	}
	
	if (!Q->mcNCE3AddrConfigured)
	{
		if (!Q->mcNCE3SizeConfigured)	
		{
			Q->mcNCE3SizeConfigured = true;
			Q->mcNCE3Size = 0x100000-d;
			return;
		}
		Q->mcNCE3AddrConfigured = true;
		Q->mcNCE3Base = d;	
		Q->mcNCE3Ceil = d+Q->mcNCE3Size;
		Q->mcNCE3Delta = (UInt32)Q->nce3addr-d;
		return;
	}
}



inline void unconfig(register emulState *Q) 
{
	UInt32 d;

	d=Npack(&Q->C[3],2)<<12;   // keep to 0x1000 nibble boundary
	
	// Controllers are un-configured in the following order:
	// HDW, RAM, CE2, CE1, NCE3
	//

	if ((Npack(Q->C,5)&0xFFFC0)==Q->mcIOBase)
	{
		Q->mcIOAddrConfigured=false;
		Q->mcIOCeil=0;
		return;
	}

	if ((Q->mcRAMAddrConfigured)&&(d==Q->mcRAMBase))
	{
		Q->mcRAMAddrConfigured=false;
		Q->mcRAMSizeConfigured=false;
		Q->mcRAMCeil = 0;	// Setting the ceiling to zero will prevent the address decoders
		return; 		// from selecting this controller. Leave the base untouched
	}
		
	if ((Q->mcCE2AddrConfigured)&&(d==Q->mcCE2Base))
	{
		Q->mcCE2AddrConfigured=false;
		Q->mcCE2SizeConfigured=false;
		Q->mcCE2Ceil = 0;	// Setting the ceiling to zero will prevent the address decoders
		return; 		// from selecting this controller. Leave the base untouched
	}
		
	if ((Q->mcCE1AddrConfigured)&&(d==Q->mcCE1Base))
	{
		Q->mcCE1AddrConfigured=false;
		Q->mcCE1SizeConfigured=false;
		Q->mcCE1Ceil = 0;	// Setting the ceiling to zero will prevent the address decoders
		return; 		// from selecting this controller. Leave the base untouched
	}
		
	if ((Q->mcNCE3AddrConfigured)&&(d==Q->mcNCE3Base))
	{
		Q->mcNCE3AddrConfigured=false;
		Q->mcNCE3SizeConfigured=false;
		Q->mcNCE3Ceil = 0;	// Setting the ceiling to zero will prevent the address decoders
		return; 		// from selecting this controller. Leave the base untouched
	}
}



inline void reset(register emulState *Q) 
{	
	Q->mcIOAddrConfigured=false;
	Q->mcIOBase=0x100000;
	Q->mcIOCeil=0;

	Q->mcRAMSizeConfigured=false;
	Q->mcRAMAddrConfigured=false;
	Q->mcRAMBase=0x100000;
	Q->mcRAMCeil=0;
	Q->mcRAMSize=0;
	Q->mcRAMDelta=0;

	Q->mcCE1SizeConfigured=false;
	Q->mcCE1AddrConfigured=false;
	Q->mcCE1Base=0x100000;
	Q->mcCE1Ceil=0;
	Q->mcCE1Size=0;
	Q->mcCE1Delta=0;

	Q->mcCE2SizeConfigured=false;
	Q->mcCE2AddrConfigured=false;
	Q->mcCE2Base=0x100000;
	Q->mcCE2Ceil=0;
	Q->mcCE2Size=0;
	Q->mcCE2Delta=0;

	Q->mcNCE3SizeConfigured=false;
	Q->mcNCE3AddrConfigured=false;
	Q->mcNCE3Base=0x100000;
	Q->mcNCE3Ceil=0;
	Q->mcNCE3Size=0;
	Q->mcNCE3Delta=0;
}



inline void c_eq_id(register emulState *Q) 
{
	UInt32 val;
	
	// Controllers are configured in the following order:
	// HDW, RAM, CE1, CE2, NCE3
	//

	if (!Q->mcIOAddrConfigured) 
	{
		val=Q->mcIOBase|0x19;
		goto repack;
	}
	
	if (!Q->mcRAMAddrConfigured)  
	{
		if (!Q->mcRAMSizeConfigured)  
		{
			val=(0x100000-Q->mcRAMSize)|0x03;
			goto repack;
		}
		val=Q->mcRAMBase|0xF4;
		goto repack;
	}

	if (!Q->mcCE1AddrConfigured)
	{
		if (!Q->mcCE1SizeConfigured)
		{
			val=(0x100000-Q->mcCE1Size)|0x05;
			goto repack;
		}
		val=Q->mcCE1Base|0xF6;
		goto repack;
	}
	
	
	if (!Q->mcCE2AddrConfigured)  
	{
		if (!Q->mcCE2SizeConfigured)
		{
			val=(0x100000-Q->mcCE2Size)|0x07;
			goto repack;
		}
		val=Q->mcCE2Base|0xF8;
		goto repack;
	}
	
	if (!Q->mcNCE3AddrConfigured) 
	{
		if (!Q->mcNCE3SizeConfigured) 
		{
			val=(0x100000-Q->mcNCE3Size)|0x01;
			goto repack;
		}
		val=Q->mcNCE3Base|0xF2;
		goto repack;
	}
	
	Q->C[0]=0;
	Q->C[1]=0;
	Q->C[2]=0;
	Q->C[4]=0;
	Q->C[5]=0;
	return;
	
repack:
	Nunpack(Q->C,val,5);
}



inline UInt32 rstkpop(register emulState *Q) 
{
	UInt32 r;
	UInt16 t;

	t = Q->rstkp;
	t--;
	t&=7;
	Q->rstkp = t;
	r = Q->rstk[t];
	Q->rstk[t] = 0;
#ifdef TRACE_ENABLED
	Q->poporpush=1;
#endif
	return r;
}



inline void rstkpush(UInt32 d, register emulState *Q) 
{
	UInt16 t;

	t = Q->rstkp;   
	Q->rstk[t] = d;
	t++;
	t &= 7;
	Q->rstkp = t;
#ifdef TRACE_ENABLED
	Q->poporpush=2;
#endif
}








/***********************************************************************************
 *
 *  Memory Primitives
 *
 *
 *
 *
 ***********************************************************************************/


inline UInt8* GetRealAddressNoIO_ARM(UInt32 d, register emulState *Q)
{		
  	if ((d>=Q->mcRAMBase)&&(d<Q->mcRAMCeil))  // RAM
	{
		return d+(UInt8*)Q->mcRAMDelta;
	}
	else if ((d>=Q->mcCE2Base)&&(d<Q->mcCE2Ceil))
	{
		return d+(UInt8*)Q->mcCE2Delta;
	}
	else if ((d>=Q->mcCE1Base)&&(d<Q->mcCE1Ceil))
	{
		return d+(UInt8*)Q->mcCE1Delta;
	}
 	else if ((d>=Q->mcNCE3Base)&&(d<Q->mcNCE3Ceil))
	{
		return d+(UInt8*)Q->mcNCE3Delta;
	}
 	else   //ROM
  	{
  		if (Q->status49GActive)
  		{
  			d&=0x7ffff;   // 49 ROM is mirrored
  			if (d<0x40000)  // lo bank
  				return Q->flashRomLo+d;
  			else			// hi bank
  				return Q->flashRomHi+d;
  		}
  		else
			return Q->rom+d;
	}
}



inline void RomSwitch(UInt32 d,UInt32 s, register emulState *Q)
{
	d=d+s-1;
	d=(d>>1)&0x3f;
	Q->flashBankHi=d&0xf;
	Q->flashBankLo=d>>4;
	Q->flashRomHi=Q->flashAddr[Q->flashBankHi]-0x40000;
	Q->flashRomLo=Q->flashAddr[Q->flashBankLo];
	
	TRACE3(435,Q->flashBankLo,Q->flashBankHi);
}




inline void DisplayMainTouch(Int32 d, register emulState *Q)
{
	Int32 y;
	Int32 width;
	
	width=Q->lcdDisplayAreaWidth;
	if (width<0) width=-width;
	if (width==0) return;
	
	d-=Q->lcdDisplayAddressStart;

	if (d>=0)
	{
		y=0; 
		while (d>=width) { y++; d-=width; }
	} 
	else
	{
		d=-d;
		y=1;
		while (d>=width) { y++; d-=width; }
	}	

	Q->lcdRedrawBuffer[y] = 1;

	TRACE3(433,d,y);
}



inline void DisplayMenuTouch(Int32 d, register emulState *Q)
{
	UInt32 y;
		
	if ((Q->lcdLineCount==0) || (Q->lcdLineCount==63))
		return;

	d -= Q->lcdMenuAddressStart;
	if (d<0) return;
	
	y = Q->lcdLineCount + 1;
	
	while (d>=34) { y++; d-=34; }

	Q->lcdRedrawBuffer[y] = 1;
	
	TRACE3(434,d,y);
}




inline UInt32 NCread(UInt8 *a, UInt32 d, UInt32 s, register emulState *Q) 
{
	UInt8 *n;
	UInt8 val;
	
	if ((d<Q->mcIOCeil)&&(d>=Q->mcIOBase))  //IO
	{
		TRACE4(442,d,d-Q->mcIOBase,s);
		return read_io_multiple_arm(d-Q->mcIOBase,a,s,Q);
	}
	else if ((d>=Q->mcRAMBase)&&(d<Q->mcRAMCeil))  // RAM
	{
		TRACE4(443,d,d+Q->mcRAMDelta,s);
		n=d+(UInt8*)Q->mcRAMDelta;
	}
	else if ((d>=Q->mcCE2Base)&&(d<Q->mcCE2Ceil))
	{
		TRACE4(444,d,d+Q->mcCE2Delta,s);
		n=d+(UInt8*)Q->mcCE2Delta;
	}
	else if ((d>=Q->mcCE1Base)&&(d<Q->mcCE1Ceil))
	{
		TRACE4(445,d,d+Q->mcCE1Delta,s);
		if (Q->status49GActive)
		{
			RomSwitch(d,s,Q);
			return false;
		}
		else
			n=d+(UInt8*)Q->mcCE1Delta;
	}
	else if ((d>=Q->mcNCE3Base)&&(d<Q->mcNCE3Ceil))
	{
		TRACE4(446,d,d+Q->mcNCE3Delta,s);
		n=d+(UInt8*)Q->mcNCE3Delta;
	}
	else   //ROM
	{
		TRACE4(447,d,d+(UInt32)Q->rom,s);
  		if (Q->status49GActive)
  		{
  			d&=0x7ffff;
  			if (d<0x40000)  // lo bank
  				n=Q->flashRomLo+d;
  			else			// hi bank
  				n=Q->flashRomHi+d;
  		}
  		else
			n=Q->rom+d;
	}
	while (s--) { val=*n++; *a++ = val; CRC_update(val,Q); };
	return false;
}



inline UInt32 Nwrite(UInt8 *a, UInt32 d, UInt32 s, register emulState *Q) 
{
	UInt8 *n;
	
	if ((d<Q->mcIOCeil)&&(d>=Q->mcIOBase))  //IO
	{
		TRACE4(448,d,d-Q->mcIOBase,s);
		return write_io_multiple_arm(d-Q->mcIOBase,a,s,Q);
	}
	else if ((d>=Q->mcRAMBase)&&(d<Q->mcRAMCeil))  // RAM
	{
		TRACE4(449,d,d+Q->mcRAMDelta,s);
		n=d+(UInt8*)Q->mcRAMDelta;
		if ((d<Q->lcdDisplayAddressEnd)&&(d>=Q->lcdDisplayAddressStart2))
			DisplayMainTouch(d,Q);
		else if ((d<Q->lcdMenuAddressEnd)&&(d>=Q->lcdMenuAddressStart))
			DisplayMenuTouch(d,Q);
	}
	else if ((d>=Q->mcCE2Base)&&(d<Q->mcCE2Ceil))
	{
		TRACE4(450,d,d+Q->mcCE2Delta,s);
		n=d+(UInt8*)Q->mcCE2Delta;
		if ((d<Q->lcdDisplayAddressEnd)&&(d>=Q->lcdDisplayAddressStart2))
			DisplayMainTouch(d,Q);
		else if ((d<Q->lcdMenuAddressEnd)&&(d>=Q->lcdMenuAddressStart))
			DisplayMenuTouch(d,Q);
	}
	else if ((d>=Q->mcCE1Base)&&(d<Q->mcCE1Ceil))
	{
		TRACE4(451,d,d+Q->mcCE1Delta,s);
		if (Q->status49GActive)
		{
			RomSwitch(d,s,Q);
			return false;
		}
		else
			n=d+(UInt8*)Q->mcCE1Delta;
	}
	else if ((d>=Q->mcNCE3Base)&&(d<Q->mcNCE3Ceil))
	{
		TRACE4(452,d,d+Q->mcNCE3Delta,s);
		n=d+(UInt8*)Q->mcNCE3Delta;
	}
	else   //ROM
	{
		TRACE4(453,d,d+(UInt32)Q->rom,s);
		n=Q->rom+d;
	}
	while (s--) { *n++ = *a++; };
	return false;
}








/***********************************************************************************
 *
 *  Hardware Primitives
 *
 *
 *
 *
 ***********************************************************************************/


inline void update_in(register emulState *Q) 
{ 
	UInt16 testOUT=Q->OUT;
	UInt16 testIN=Q->IR15X;

	if (testOUT & 0x001) testIN|=Q->kbd_row[0];
	if (testOUT & 0x002) testIN|=Q->kbd_row[1];
	if (testOUT & 0x004) testIN|=Q->kbd_row[2];
	if (testOUT & 0x008) testIN|=Q->kbd_row[3];
	if (testOUT & 0x010) testIN|=Q->kbd_row[4];
	if (testOUT & 0x020) testIN|=Q->kbd_row[5];
	if (testOUT & 0x040) testIN|=Q->kbd_row[6];
	if (testOUT & 0x080) testIN|=Q->kbd_row[7];
	if (testOUT & 0x100) testIN|=Q->kbd_row[8];
	Q->IN=testIN;
}





#define NCFread(a,b,f,q)				NCread(a+q->F_s[f],b,q->F_l[f],q)
#define NFwrite(a,b,f,q)				Nwrite(a+q->F_s[f],b,q->F_l[f],q)
#define NFcopy(a,b,f,q)					Ncopy(a+q->F_s[f],b+q->F_s[f],q->F_l[f])
#define NFxchg(a,b,f,q)					Nxchg(a+q->F_s[f],b+q->F_s[f],q->F_l[f])
#define NFadd(a,b,f,q)					Nadd(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)
#define NFsub(a,b,f,q)					Nsub(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)
#define NFrsub(a,b,f,q)					Nrsub(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)
#define NFand(a,b,f,q)					Nand(a+q->F_s[f],b+q->F_s[f],q->F_l[f])
#define NFor(a,b,f,q)					Nor(a+q->F_s[f],b+q->F_s[f],q->F_l[f])
#define NFzero(a,f,q)					Nzero(a+q->F_s[f],q->F_l[f])
#define NFnot(a,f,q) 					Nnot(a+q->F_s[f],q->F_l[f],q)
#define NFneg(a,f,q) 					Nneg(a+q->F_s[f],q->F_l[f],q)
#define NFsr(a,f,q)						Nsr(a+q->F_s[f],q->F_l[f],q)
#define NFsl(a,f,q)						Nsl(a+q->F_s[f],q->F_l[f])
#define NFsrb(a,f,q) 					Nsrb(a+q->F_s[f],q->F_l[f],q)
#define TFe(a,b,f,q) 					Te(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)
#define TFne(a,b,f,q) 					Tne(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)
#define TFz(a,f,q) 						Tz(a+q->F_s[f],q->F_l[f],q)
#define TFnz(a,f,q)						Tnz(a+q->F_s[f],q->F_l[f],q)
#define TFa(a,b,f,q) 					Ta(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)
#define TFae(a,b,f,q) 					Tae(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)
#define TFb(a,b,f,q)					Tb(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)
#define TFbe(a,b,f,q)					Tbe(a+q->F_s[f],b+q->F_s[f],q->F_l[f],q)







UInt32 ARMlet_Main(const void *emulStateP, void *userData68KP, Call68KFuncType *call68KFuncP)
{
	UInt32 			d,e;
	UInt8 			*I, *GOJMP, n;
	Int8 			jmp;
	UInt8 			*a8,*b8;
	UInt32 			currentTick;
	UInt32 			colorCycleCount=10;
	UInt16 			oldContrast;
	UInt32 			index;
	UInt32			tickMark;
	UInt32			pollKeyboard;
	UInt32			slowDown;
	UInt32			keyPressed;
	UInt32			eventTimeout;
	UInt32			instCount;
	UInt32 			prevInstCount;
	UInt32			cycleCount;
	UInt32			prevCycleCount;
	UInt32			t2ICmark;
	UInt32			instPerT2Tick;
	UInt32			flipflop;
	
	register emulState *Q;	
	
	Q=userData68KP;

	if (Q->statusScreenRedraw)
	{
		Q->statusScreenRedraw=false;
		adjust_contrast_arm(Q);
		for (index=0;index<64;index++)
		{
			if (Q->statusDisplayMode2x2)
			{
				if (Q->statusPortraitViaLandscape)
					DrawDisplayLine16x2_arm_PVL(Q);
				else
					DrawDisplayLine16x2_arm(Q);
			}
			else
			{
					DrawDisplayLine16x1_arm(Q);
			}
				
			Q->lcdCurrentLine++;
			
			if (Q->lcdCurrentLine>=0x40) Q->lcdCurrentLine=0;
		}
		Q->lcdPreviousAnnunciatorState=0;
		change_ann(Q);				
		return 1;
	}
	
		
	Q->emulStateP = emulStateP;
	Q->call68KFuncP = call68KFuncP;
	
	SwapAllData_arm(Q);
	
	Q->labelOffset[0]=0;
	Q->labelOffset[1]=skipToKeyB;
	Q->labelOffset[2]=skipToKeyC;
	Q->labelOffset[3]=(27*Q->rowInt16s)-skipToKeyD;
	Q->labelOffset[4]=skipToKeyE;
	Q->labelOffset[5]=skipToKeyF;
		
	Q->colorCyclePhase=0;
	Q->colorCycleRed=0x1F;
	Q->colorCycleGreen=0;
	Q->colorCycleBlue=0;	
	
	d=0;
	
	slowDown=0;
	keyPressed=0;
	flipflop=0;
	
	instCount=0;
	prevInstCount=0;
	cycleCount=0;
	prevCycleCount=Q->maxCyclesPerTick;
	eventTimeout=0;
	tickMark = TimGetTicks_arm(Q)+1;
	pollKeyboard=0;
	
	instPerT2Tick=20;
	t2ICmark = 100;
	Q->prevTickStamp = TimGetTicks_arm(Q);
	
	
	Q->IO_TIMER_RUN=(Q->ioram[0x2f]&1);
	
	adjust_contrast_arm(Q);	
	RecalculateDisplayParameters_arm(Q);
	Q->statusRecalc=false;
	Q->statusChangeAnn=false;
	
	I=GetRealAddressNoIO_ARM(Q->pc,Q);
	
	while (1) 
	{
		//instCount++;

#ifdef TRACE_ENABLED
		Q->traceNumber=999;
		Q->OLD_PC=ByteSwap32(Q->pc);
		
		Q->A_HI = ((UInt32)Q->A[15]<<28) | ((UInt32)Q->A[14]<<24) | 
			      ((UInt32)Q->A[13]<<20) | ((UInt32)Q->A[12]<<16) |
			      ((UInt32)Q->A[11]<<12) | ((UInt32)Q->A[10]<<8)  | 
			      ((UInt32)Q->A[ 9]<<4)  |  (UInt32)Q->A[ 8];
		Q->A_HI = ByteSwap32(Q->A_HI);
		Q->A_LO = ((UInt32)Q->A[ 7]<<28) | ((UInt32)Q->A[ 6]<<24) | 
			      ((UInt32)Q->A[ 5]<<20) | ((UInt32)Q->A[ 4]<<16) |
			      ((UInt32)Q->A[ 3]<<12) | ((UInt32)Q->A[ 2]<<8)  | 
			      ((UInt32)Q->A[ 1]<<4)  |  (UInt32)Q->A[ 0];
		Q->A_LO = ByteSwap32(Q->A_LO);
		
		Q->B_HI = ((UInt32)Q->B[15]<<28) | ((UInt32)Q->B[14]<<24) | 
			      ((UInt32)Q->B[13]<<20) | ((UInt32)Q->B[12]<<16) |
			      ((UInt32)Q->B[11]<<12) | ((UInt32)Q->B[10]<<8)  | 
			      ((UInt32)Q->B[ 9]<<4)  |  (UInt32)Q->B[ 8];
		Q->B_HI = ByteSwap32(Q->B_HI);
		Q->B_LO = ((UInt32)Q->B[ 7]<<28) | ((UInt32)Q->B[ 6]<<24) | 
			      ((UInt32)Q->B[ 5]<<20) | ((UInt32)Q->B[ 4]<<16) |
			      ((UInt32)Q->B[ 3]<<12) | ((UInt32)Q->B[ 2]<<8)  | 
			      ((UInt32)Q->B[ 1]<<4)  |  (UInt32)Q->B[ 0];
		Q->B_LO = ByteSwap32(Q->B_LO);

		Q->C_HI = ((UInt32)Q->C[15]<<28) | ((UInt32)Q->C[14]<<24) | 
			      ((UInt32)Q->C[13]<<20) | ((UInt32)Q->C[12]<<16) |
			      ((UInt32)Q->C[11]<<12) | ((UInt32)Q->C[10]<<8)  | 
			      ((UInt32)Q->C[ 9]<<4)  |  (UInt32)Q->C[ 8];
		Q->C_HI = ByteSwap32(Q->C_HI);
		Q->C_LO = ((UInt32)Q->C[ 7]<<28) | ((UInt32)Q->C[ 6]<<24) | 
			      ((UInt32)Q->C[ 5]<<20) | ((UInt32)Q->C[ 4]<<16) |
			      ((UInt32)Q->C[ 3]<<12) | ((UInt32)Q->C[ 2]<<8)  | 
			      ((UInt32)Q->C[ 1]<<4)  |  (UInt32)Q->C[ 0];
		Q->C_LO = ByteSwap32(Q->C_LO);

		Q->D_HI = ((UInt32)Q->D[15]<<28) | ((UInt32)Q->D[14]<<24) | 
			      ((UInt32)Q->D[13]<<20) | ((UInt32)Q->D[12]<<16) |
			      ((UInt32)Q->D[11]<<12) | ((UInt32)Q->D[10]<<8)  | 
			      ((UInt32)Q->D[ 9]<<4)  |  (UInt32)Q->D[ 8];
		Q->D_HI = ByteSwap32(Q->D_HI);
		Q->D_LO = ((UInt32)Q->D[ 7]<<28) | ((UInt32)Q->D[ 6]<<24) | 
			      ((UInt32)Q->D[ 5]<<20) | ((UInt32)Q->D[ 4]<<16) |
			      ((UInt32)Q->D[ 3]<<12) | ((UInt32)Q->D[ 2]<<8)  | 
			      ((UInt32)Q->D[ 1]<<4)  |  (UInt32)Q->D[ 0];
		Q->D_LO = ByteSwap32(Q->D_LO);
		
		Q->OLD_ST = ((UInt16)Q->ST[3]<<12) | ((UInt16)Q->ST[2]<<8)  | 
					((UInt16)Q->ST[1]<<4)  |  (UInt16)Q->ST[0];

		Q->nibcode = ((UInt32)I[0]<<28) | ((UInt32)I[1]<<24) | 
					 ((UInt32)I[2]<<20) | ((UInt32)I[3]<<16) |
					 ((UInt32)I[4]<<12) | ((UInt32)I[5]<<8)  | 
					 ((UInt32)I[6]<<4)  |  (UInt32)I[7];
		Q->nibcode = ByteSwap32(Q->nibcode);
		
			   
		Q->OLD_P = Q->P;
		Q->OLD_CARRY = Q->CARRY;
		Q->OLD_D0 = ByteSwap32(Q->d0);
		Q->OLD_D1 = ByteSwap32(Q->d1);
		Q->OLD_T1 = Q->t1;
		Q->OLD_T2 = ByteSwap32(Q->t2);
#endif


	
switch((I[0]<<4)|I[1]) {
  case 0x00: goto o00;
  case 0x01: goto o01;
  case 0x02: goto o02;
  case 0x03: goto o03;
  case 0x04: goto o04;
  case 0x05: goto o05;
  case 0x06: goto o06;
  case 0x07: goto o07;
  case 0x08: goto o08;
  case 0x09: goto o09;
  case 0x0A: goto o0A;
  case 0x0B: goto o0B;
  case 0x0C: goto o0C;
  case 0x0D: goto o0D;
  case 0x0E: switch(I[3]) {
   case 0x0: goto o0Ef0;
   case 0x1: goto o0Ef1;
   case 0x2: goto o0Ef2;
   case 0x3: goto o0Ef3;
   case 0x4: goto o0Ef4;
   case 0x5: goto o0Ef5;
   case 0x6: goto o0Ef6;
   case 0x7: goto o0Ef7;
   case 0x8: goto o0Ef8;
   case 0x9: goto o0Ef9;
   case 0xA: goto o0EfA;
   case 0xB: goto o0EfB;
   case 0xC: goto o0EfC;
   case 0xD: goto o0EfD;
   case 0xE: goto o0EfE;
   case 0xF: goto o0EfF; } goto o_invalid;
  case 0x0F: goto o0F;
  case 0x10: switch(I[2]) {
   case 0x0: goto o100;
   case 0x1: goto o101;
   case 0x2: goto o102;
   case 0x3: goto o103;
   case 0x4: goto o104;
   case 0x8: goto o108;
   case 0x9: goto o109;
   case 0xA: goto o10A;
   case 0xB: goto o10B;
   case 0xC: goto o10C; } goto o_invalid3;
  case 0x11: switch(I[2]) {
   case 0x0: goto o110;
   case 0x1: goto o111;
   case 0x2: goto o112;
   case 0x3: goto o113;
   case 0x4: goto o114;
   case 0x8: goto o118;
   case 0x9: goto o119;
   case 0xA: goto o11A;
   case 0xB: goto o11B;
   case 0xC: goto o11C; } goto o_invalid3;
  case 0x12: switch(I[2]) {
   case 0x0: goto o120;
   case 0x1: goto o121;
   case 0x2: goto o122;
   case 0x3: goto o123;
   case 0x4: goto o124;
   case 0x8: goto o128;
   case 0x9: goto o129;
   case 0xA: goto o12A;
   case 0xB: goto o12B;
   case 0xC: goto o12C; } goto o_invalid3;
  case 0x13: switch(I[2]) {
   case 0x0: goto o130;
   case 0x1: goto o131;
   case 0x2: goto o132;
   case 0x3: goto o133;
   case 0x4: goto o134;
   case 0x5: goto o135;
   case 0x6: goto o136;
   case 0x7: goto o137;
   case 0x8: goto o138;
   case 0x9: goto o139;
   case 0xA: goto o13A;
   case 0xB: goto o13B;
   case 0xC: goto o13C;
   case 0xD: goto o13D;
   case 0xE: goto o13E;
   case 0xF: goto o13F; } goto o_invalid;
  case 0x14: switch(I[2]) {
   case 0x0: goto o140;
   case 0x1: goto o141;
   case 0x2: goto o142;
   case 0x3: goto o143;
   case 0x4: goto o144;
   case 0x5: goto o145;
   case 0x6: goto o146;
   case 0x7: goto o147;
   case 0x8: goto o148;
   case 0x9: goto o149;
   case 0xA: goto o14A;
   case 0xB: goto o14B;
   case 0xC: goto o14C;
   case 0xD: goto o14D;
   case 0xE: goto o14E;
   case 0xF: goto o14F; } goto o_invalid;
  case 0x15: switch(I[2]) {
   case 0x0: goto o150a;
   case 0x1: goto o151a;
   case 0x2: goto o152a;
   case 0x3: goto o153a;
   case 0x4: goto o154a;
   case 0x5: goto o155a;
   case 0x6: goto o156a;
   case 0x7: goto o157a;
   case 0x8: goto o158x;
   case 0x9: goto o159x;
   case 0xA: goto o15Ax;
   case 0xB: goto o15Bx;
   case 0xC: goto o15Cx;
   case 0xD: goto o15Dx;
   case 0xE: goto o15Ex;
   case 0xF: goto o15Fx; } goto o_invalid;
  case 0x16: goto o16x;
  case 0x17: goto o17x;
  case 0x18: goto o18x;
  case 0x19: goto o19d2;
  case 0x1A: goto o1Ad4;
  case 0x1B: goto o1Bd5;
  case 0x1C: goto o1Cx;
  case 0x1D: goto o1Dd2;
  case 0x1E: goto o1Ed4;
  case 0x1F: goto o1Fd5;
  
  case 0x20: goto o2n;
  case 0x21: goto o2n;
  case 0x22: goto o2n;
  case 0x23: goto o2n;
  case 0x24: goto o2n;
  case 0x25: goto o2n;
  case 0x26: goto o2n;
  case 0x27: goto o2n;
  case 0x28: goto o2n;
  case 0x29: goto o2n;
  case 0x2A: goto o2n;
  case 0x2B: goto o2n;
  case 0x2C: goto o2n;
  case 0x2D: goto o2n;
  case 0x2E: goto o2n;
  case 0x2F: goto o2n;

  case 0x30: goto o3X;
  case 0x31: goto o3X;
  case 0x32: goto o3X;
  case 0x33: goto o3X;
  case 0x34: goto o3X;
  case 0x35: goto o3X;
  case 0x36: goto o3X;
  case 0x37: goto o3X;
  case 0x38: goto o3X;
  case 0x39: goto o3X;
  case 0x3A: goto o3X;
  case 0x3B: goto o3X;
  case 0x3C: goto o3X;
  case 0x3D: goto o3X;
  case 0x3E: goto o3X;
  case 0x3F: goto o3X;

  case 0x40: goto o4d2;
  case 0x41: goto o4d2;
  case 0x42: goto o4d2;
  case 0x43: goto o4d2;
  case 0x44: goto o4d2;
  case 0x45: goto o4d2;
  case 0x46: goto o4d2;
  case 0x47: goto o4d2;
  case 0x48: goto o4d2;
  case 0x49: goto o4d2;
  case 0x4A: goto o4d2;
  case 0x4B: goto o4d2;
  case 0x4C: goto o4d2;
  case 0x4D: goto o4d2;
  case 0x4E: goto o4d2;
  case 0x4F: goto o4d2;

  case 0x50: goto o5d2;
  case 0x51: goto o5d2;
  case 0x52: goto o5d2;
  case 0x53: goto o5d2;
  case 0x54: goto o5d2;
  case 0x55: goto o5d2;
  case 0x56: goto o5d2;
  case 0x57: goto o5d2;
  case 0x58: goto o5d2;
  case 0x59: goto o5d2;
  case 0x5A: goto o5d2;
  case 0x5B: goto o5d2;
  case 0x5C: goto o5d2;
  case 0x5D: goto o5d2;
  case 0x5E: goto o5d2;
  case 0x5F: goto o5d2;

  case 0x60: goto o6d3;
  case 0x61: goto o6d3;
  case 0x62: goto o6d3;
  case 0x63: goto o6d3;
  case 0x64: goto o6d3;
  case 0x65: goto o6d3;
  case 0x66: goto o6d3;
  case 0x67: goto o6d3;
  case 0x68: goto o6d3;
  case 0x69: goto o6d3;
  case 0x6A: goto o6d3;
  case 0x6B: goto o6d3;
  case 0x6C: goto o6d3;
  case 0x6D: goto o6d3;
  case 0x6E: goto o6d3;
  case 0x6F: goto o6d3;

  case 0x70: goto o7d3;
  case 0x71: goto o7d3;
  case 0x72: goto o7d3;
  case 0x73: goto o7d3;
  case 0x74: goto o7d3;
  case 0x75: goto o7d3;
  case 0x76: goto o7d3;
  case 0x77: goto o7d3;
  case 0x78: goto o7d3;
  case 0x79: goto o7d3;
  case 0x7A: goto o7d3;
  case 0x7B: goto o7d3;
  case 0x7C: goto o7d3;
  case 0x7D: goto o7d3;
  case 0x7E: goto o7d3;
  case 0x7F: goto o7d3;
  
  case 0x80: switch(I[2]) {
   case 0x0: goto o800;
   case 0x1: goto o801;
   case 0x2: goto o802;
   case 0x3: goto o803;
   case 0x4: goto o804;
   case 0x5: goto o805;
   case 0x6: goto o806;
   case 0x7: goto o807;
   case 0x8: switch(I[3]) {
    case 0x0: goto o8080;
    case 0x1: if (I[4]) goto o_invalid5; goto o80810;
    case 0x2: goto o8082X;
    case 0x3: goto o8083;
    case 0x4: goto o8084n;
    case 0x5: goto o8085n;
    case 0x6: goto o8086n;
    case 0x7: goto o8087n;
    case 0x8: goto o8088n;
    case 0x9: goto o8089n;
    case 0xA: goto o808An;
    case 0xB: goto o808Bn;
    case 0xC: goto o808C;
    case 0xD: goto o808D;
    case 0xE: goto o808E;
    case 0xF: goto o808F; } goto o_invalid;
   case 0x9: goto o809;
   case 0xA: goto o80A;
   case 0xB: goto o80B;
   case 0xC: goto o80Cn;
   case 0xD: goto o80Dn;
   case 0xE: goto o80E;
   case 0xF: goto o80Fn; } goto o_invalid;
  case 0x81: switch(I[2]) {
   case 0x0: goto o810;
   case 0x1: goto o811;
   case 0x2: goto o812;
   case 0x3: goto o813;
   case 0x4: goto o814;
   case 0x5: goto o815;
   case 0x6: goto o816;
   case 0x7: goto o817;
   case 0x8: switch(I[4]) {
    case 0x0: goto o818f0x;
    case 0x1: goto o818f1x;
    case 0x2: goto o818f2x;
    case 0x3: goto o818f3x;
    case 0x8: goto o818f8x;
    case 0x9: goto o818f9x;
    case 0xA: goto o818fAx;
    case 0xB: goto o818fBx; } goto o_invalid6;
   case 0x9: switch(I[4]) {
    case 0x0: goto o819f0;
    case 0x1: goto o819f1;
    case 0x2: goto o819f2;
    case 0x3: goto o819f3; } goto o_invalid5;
   case 0xA: switch(I[4]) {
    case 0x0: switch(I[5]) {
     case 0x0: goto o81Af00;
     case 0x1: goto o81Af01;
     case 0x2: goto o81Af02;
     case 0x3: goto o81Af03;
     case 0x4: goto o81Af04;
     case 0x8: goto o81Af08;
     case 0x9: goto o81Af09;
     case 0xA: goto o81Af0A;
     case 0xB: goto o81Af0B;
     case 0xC: goto o81Af0C; } goto o_invalid6;
    case 0x1: switch(I[5]) {
     case 0x0: goto o81Af10;
     case 0x1: goto o81Af11;
     case 0x2: goto o81Af12;
     case 0x3: goto o81Af13;
     case 0x4: goto o81Af14;
     case 0x8: goto o81Af18;
     case 0x9: goto o81Af19;
     case 0xA: goto o81Af1A;
     case 0xB: goto o81Af1B;
     case 0xC: goto o81Af1C; } goto o_invalid6;
    case 0x2: switch(I[5]) {
     case 0x0: goto o81Af20;
     case 0x1: goto o81Af21;
     case 0x2: goto o81Af22;
     case 0x3: goto o81Af23;
     case 0x4: goto o81Af24;
     case 0x8: goto o81Af28;
     case 0x9: goto o81Af29;
     case 0xA: goto o81Af2A;
     case 0xB: goto o81Af2B;
     case 0xC: goto o81Af2C; } goto o_invalid6; } goto o_invalid5;
   case 0xB: switch(I[3]) {
    case 0x1: goto o81B1;
    case 0x2: goto o81B2;
    case 0x3: goto o81B3;
    case 0x4: goto o81B4;
    case 0x5: goto o81B5;
    case 0x6: goto o81B6;
    case 0x7: goto o81B7; } goto o_invalid4;
   case 0xC: goto o81C;
   case 0xD: goto o81D;
   case 0xE: goto o81E;
   case 0xF: goto o81F; } goto o_invalid;
  case 0x82: goto o82n;
  case 0x83: goto o83n;
  case 0x84: goto o84n;
  case 0x85: goto o85n;
  case 0x86: goto o86n;
  case 0x87: goto o87n;
  case 0x88: goto o88n;
  case 0x89: goto o89n;
  case 0x8A: switch(I[2]) {
   case 0x0: goto o8A0;
   case 0x1: goto o8A1;
   case 0x2: goto o8A2;
   case 0x3: goto o8A3;
   case 0x4: goto o8A4;
   case 0x5: goto o8A5;
   case 0x6: goto o8A6;
   case 0x7: goto o8A7;
   case 0x8: goto o8A8;
   case 0x9: goto o8A9;
   case 0xA: goto o8AA;
   case 0xB: goto o8AB;
   case 0xC: goto o8AC;
   case 0xD: goto o8AD;
   case 0xE: goto o8AE;
   case 0xF: goto o8AF;} goto o_invalid;
  case 0x8B: switch(I[2]) {
   case 0x0: goto o8B0;
   case 0x1: goto o8B1;
   case 0x2: goto o8B2;
   case 0x3: goto o8B3;
   case 0x4: goto o8B4;
   case 0x5: goto o8B5;
   case 0x6: goto o8B6;
   case 0x7: goto o8B7;
   case 0x8: goto o8B8;
   case 0x9: goto o8B9;
   case 0xA: goto o8BA;
   case 0xB: goto o8BB;
   case 0xC: goto o8BC;
   case 0xD: goto o8BD;
   case 0xE: goto o8BE;
   case 0xF: goto o8BF; } goto o_invalid;
  case 0x8C: goto o8Cd4;
  case 0x8D: goto o8Dd5;
  case 0x8E: goto o8Ed4;
  case 0x8F: goto o8Fd5;
  
  case 0x90: 
  case 0x91: 
  case 0x92: 
  case 0x93: 
  case 0x94: 
  case 0x95: 
  case 0x96: 
  case 0x97:switch(I[2]) {
   case 0x0: goto o9a0;
   case 0x1: goto o9a1;
   case 0x2: goto o9a2;
   case 0x3: goto o9a3;
   case 0x4: goto o9a4;
   case 0x5: goto o9a5;
   case 0x6: goto o9a6;
   case 0x7: goto o9a7;
   case 0x8: goto o9a8;
   case 0x9: goto o9a9;
   case 0xA: goto o9aA;
   case 0xB: goto o9aB;
   case 0xC: goto o9aC;
   case 0xD: goto o9aD;
   case 0xE: goto o9aE;
   case 0xF: goto o9aF; } goto o_invalid;       
  case 0x98: 
  case 0x99: 
  case 0x9A: 
  case 0x9B: 
  case 0x9C: 
  case 0x9D: 
  case 0x9E: 
  case 0x9F: switch(I[2]) {
   case 0x0: goto o9b0;
   case 0x1: goto o9b1;
   case 0x2: goto o9b2;
   case 0x3: goto o9b3;
   case 0x4: goto o9b4;
   case 0x5: goto o9b5;
   case 0x6: goto o9b6;
   case 0x7: goto o9b7;
   case 0x8: goto o9b8;
   case 0x9: goto o9b9;
   case 0xA: goto o9bA;
   case 0xB: goto o9bB;
   case 0xC: goto o9bC;
   case 0xD: goto o9bD;
   case 0xE: goto o9bE;
   case 0xF: goto o9bF; } goto o_invalid;
   
  case 0xA0: 
  case 0xA1: 
  case 0xA2: 
  case 0xA3: 
  case 0xA4: 
  case 0xA5: 
  case 0xA6: 
  case 0xA7:switch(I[2]) {
   case 0x0: goto oAa0;
   case 0x1: goto oAa1;
   case 0x2: goto oAa2;
   case 0x3: goto oAa3;
   case 0x4: goto oAa4;
   case 0x5: goto oAa5;
   case 0x6: goto oAa6;
   case 0x7: goto oAa7;
   case 0x8: goto oAa8;
   case 0x9: goto oAa9;
   case 0xA: goto oAaA;
   case 0xB: goto oAaB;
   case 0xC: goto oAaC;
   case 0xD: goto oAaD;
   case 0xE: goto oAaE;
   case 0xF: goto oAaF; } goto o_invalid;
  case 0xA8: 
  case 0xA9: 
  case 0xAA: 
  case 0xAB: 
  case 0xAC: 
  case 0xAD: 
  case 0xAE: 
  case 0xAF: switch(I[2]) {
   case 0x0: goto oAb0;
   case 0x1: goto oAb1;
   case 0x2: goto oAb2;
   case 0x3: goto oAb3;
   case 0x4: goto oAb4;
   case 0x5: goto oAb5;
   case 0x6: goto oAb6;
   case 0x7: goto oAb7;
   case 0x8: goto oAb8;
   case 0x9: goto oAb9;
   case 0xA: goto oAbA;
   case 0xB: goto oAbB;
   case 0xC: goto oAbC;
   case 0xD: goto oAbD;
   case 0xE: goto oAbE;
   case 0xF: goto oAbF; } goto o_invalid;
   
  case 0xB0: 
  case 0xB1: 
  case 0xB2: 
  case 0xB3: 
  case 0xB4: 
  case 0xB5: 
  case 0xB6: 
  case 0xB7:switch(I[2]) {
   case 0x0: goto oBa0;
   case 0x1: goto oBa1;
   case 0x2: goto oBa2;
   case 0x3: goto oBa3;
   case 0x4: goto oBa4;
   case 0x5: goto oBa5;
   case 0x6: goto oBa6;
   case 0x7: goto oBa7;
   case 0x8: goto oBa8;
   case 0x9: goto oBa9;
   case 0xA: goto oBaA;
   case 0xB: goto oBaB;
   case 0xC: goto oBaC;
   case 0xD: goto oBaD;
   case 0xE: goto oBaE;
   case 0xF: goto oBaF; } goto o_invalid;
  case 0xB8: 
  case 0xB9: 
  case 0xBA: 
  case 0xBB: 
  case 0xBC: 
  case 0xBD: 
  case 0xBE: 
  case 0xBF: switch(I[2]) {
   case 0x0: goto oBb0;
   case 0x1: goto oBb1;
   case 0x2: goto oBb2;
   case 0x3: goto oBb3;
   case 0x4: goto oBb4;
   case 0x5: goto oBb5;
   case 0x6: goto oBb6;
   case 0x7: goto oBb7;
   case 0x8: goto oBb8;
   case 0x9: goto oBb9;
   case 0xA: goto oBbA;
   case 0xB: goto oBbB;
   case 0xC: goto oBbC;
   case 0xD: goto oBbD;
   case 0xE: goto oBbE;
   case 0xF: goto oBbF; } goto o_invalid;
  
  case 0xC0: goto oC0;
  case 0xC1: goto oC1;
  case 0xC2: goto oC2;
  case 0xC3: goto oC3;
  case 0xC4: goto oC4;
  case 0xC5: goto oC5;
  case 0xC6: goto oC6;
  case 0xC7: goto oC7;
  case 0xC8: goto oC8;
  case 0xC9: goto oC9;
  case 0xCA: goto oCA;
  case 0xCB: goto oCB;
  case 0xCC: goto oCC;
  case 0xCD: goto oCD;
  case 0xCE: goto oCE;
  case 0xCF: goto oCF;
  case 0xD0: goto oD0;
  case 0xD1: goto oD1;
  case 0xD2: goto oD2;
  case 0xD3: goto oD3;
  case 0xD4: goto oD4;
  case 0xD5: goto oD5;
  case 0xD6: goto oD6;
  case 0xD7: goto oD7;
  case 0xD8: goto oD8;
  case 0xD9: goto oD9;
  case 0xDA: goto oDA;
  case 0xDB: goto oDB;
  case 0xDC: goto oDC;
  case 0xDD: goto oDD;
  case 0xDE: goto oDE;
  case 0xDF: goto oDF; 
  case 0xE0: goto oE0;
  case 0xE1: goto oE1;
  case 0xE2: goto oE2;
  case 0xE3: goto oE3;
  case 0xE4: goto oE4;
  case 0xE5: goto oE5;
  case 0xE6: goto oE6;
  case 0xE7: goto oE7;
  case 0xE8: goto oE8;
  case 0xE9: goto oE9;
  case 0xEA: goto oEA;
  case 0xEB: goto oEB;
  case 0xEC: goto oEC;
  case 0xED: goto oED;
  case 0xEE: goto oEE;
  case 0xEF: goto oEF; 
  case 0xF0: goto oF0;
  case 0xF1: goto oF1;
  case 0xF2: goto oF2;
  case 0xF3: goto oF3;
  case 0xF4: goto oF4;
  case 0xF5: goto oF5;
  case 0xF6: goto oF6;
  case 0xF7: goto oF7;
  case 0xF8: goto oF8;
  case 0xF9: goto oF9;
  case 0xFA: goto oFA;
  case 0xFB: goto oFB;
  case 0xFC: goto oFC;
  case 0xFD: goto oFD;
  case 0xFE: goto oFE;
  case 0xFF: goto oFF; } goto o_invalid;



o00: TICKS(9) TRACE1(0)  Q->HST|=XM; Q->pc = rstkpop(Q); goto o_done_resetI;
o01: TICKS(9) TRACE1(1)  Q->pc = rstkpop(Q); goto o_done_resetI;
o02: TICKS(9) TRACE1(2)  Q->CARRY=1; Q->pc = rstkpop(Q); goto o_done_resetI;
o03: TICKS(9) TRACE1(3)  Q->CARRY=0; Q->pc = rstkpop(Q); goto o_done_resetI;
o04: TICKS(3) TRACE1(4)  Q->pc+=2; Q->MODE=16; I+=2; goto o_done;
o05: TICKS(3) TRACE1(5)  Q->pc+=2; Q->MODE=10; I+=2; goto o_done;
o06: TICKS(8) TRACE1(6)  Q->pc+=2; rstkpush(Npack(Q->C,5),Q); I+=2; NOT_FUNCTION_CALL goto o_done;
o07: TICKS(8) TRACE1(7)  Q->pc+=2; Nunpack(Q->C, rstkpop(Q), 5); I+=2; NOT_FUNCTION_CALL goto o_done;
o08: TICKS(5) TRACE1(8)  Q->pc+=2; Nzero(Q->ST,3); I+=2; goto o_done;
o09: TICKS(5) TRACE1(9)  Q->pc+=2; Ncopy(Q->C, Q->ST, 3); I+=2; goto o_done;
o0A: TICKS(5) TRACE1(10)  Q->pc+=2; Ncopy(Q->ST, Q->C, 3); I+=2; goto o_done;
o0B: TICKS(5) TRACE1(11)  Q->pc+=2; Nxchg(Q->C, Q->ST, 3); I+=2; goto o_done;
o0C: TICKS(3) TRACE1(12)  if (Q->P<15) { Q->P++; Q->CARRY=0; } else { Q->P=0; Q->CARRY=1; } PCHANGED I+=2; Q->pc+=2; goto o_done;
o0D: TICKS(3) TRACE1(13)  if (Q->P>0) { Q->P--; Q->CARRY=0; } else { Q->P=15; Q->CARRY=1; } PCHANGED I+=2; Q->pc+=2; goto o_done;

o0Ef0: a8=Q->A; b8=Q->B; TRACE2(14,I[2])  goto nfand_finish;
o0Ef1: a8=Q->B; b8=Q->C; TRACE2(15,I[2])  goto nfand_finish;
o0Ef2: a8=Q->C; b8=Q->A; TRACE2(16,I[2])  goto nfand_finish;
o0Ef3: a8=Q->D; b8=Q->C; TRACE2(17,I[2])  goto nfand_finish;
o0Ef4: a8=Q->B; b8=Q->A; TRACE2(18,I[2])  goto nfand_finish;
o0Ef5: a8=Q->C; b8=Q->B; TRACE2(19,I[2])  goto nfand_finish;
o0Ef6: a8=Q->A; b8=Q->C; TRACE2(20,I[2])  goto nfand_finish;
o0Ef7: a8=Q->C; b8=Q->D; TRACE2(21,I[2])  goto nfand_finish;
		nfand_finish: TICKS(4+Q->F_l[I[2]]) Q->pc+=4; NFand(a8,b8,I[2],Q); I+=4; goto o_done;
o0Ef8: a8=Q->A; b8=Q->B; TRACE2(22,I[2])  goto nfor_finish;
o0Ef9: a8=Q->B; b8=Q->C; TRACE2(23,I[2])  goto nfor_finish;
o0EfA: a8=Q->C; b8=Q->A; TRACE2(24,I[2])  goto nfor_finish;
o0EfB: a8=Q->D; b8=Q->C; TRACE2(25,I[2])  goto nfor_finish;
o0EfC: a8=Q->B; b8=Q->A; TRACE2(26,I[2])  goto nfor_finish;
o0EfD: a8=Q->C; b8=Q->B; TRACE2(27,I[2])  goto nfor_finish;
o0EfE: a8=Q->A; b8=Q->C; TRACE2(28,I[2])  goto nfor_finish;
o0EfF: a8=Q->C; b8=Q->D; TRACE2(29,I[2])  goto nfor_finish;
		nfor_finish: TICKS(4+Q->F_l[I[2]]) Q->pc+=4; NFor(a8,b8,I[2],Q); I+=4; goto o_done;

o0F: TICKS(9) TRACE1(30)  if (Q->INTERRUPT_PENDING && Q->INTERRUPT_ENABLED) {Q->INTERRUPT_PENDING=0;Q->pc=0xf;} else {Q->INTERRUPT_PROCESSING=0;Q->pc=rstkpop(Q);} goto o_done_resetI;

o100: a8=Q->R0; b8=Q->A; TRACE1(31)  goto ncopy1_finish;
o101: a8=Q->R1; b8=Q->A; TRACE1(32)  goto ncopy1_finish;
o102: a8=Q->R2; b8=Q->A; TRACE1(33)  goto ncopy1_finish;
o103: a8=Q->R3; b8=Q->A; TRACE1(34)  goto ncopy1_finish;
o104: a8=Q->R4; b8=Q->A; TRACE1(35)  goto ncopy1_finish;
o108: a8=Q->R0; b8=Q->C; TRACE1(36)  goto ncopy1_finish;
o109: a8=Q->R1; b8=Q->C; TRACE1(37)  goto ncopy1_finish;
o10A: a8=Q->R2; b8=Q->C; TRACE1(38)  goto ncopy1_finish;
o10B: a8=Q->R3; b8=Q->C; TRACE1(39)  goto ncopy1_finish;
o10C: a8=Q->R4; b8=Q->C; TRACE1(40)  goto ncopy1_finish;
o110: a8=Q->A; b8=Q->R0; TRACE1(41)  goto ncopy1_finish;
o111: a8=Q->A; b8=Q->R1; TRACE1(42)  goto ncopy1_finish;
o112: a8=Q->A; b8=Q->R2; TRACE1(43)  goto ncopy1_finish;
o113: a8=Q->A; b8=Q->R3; TRACE1(44)  goto ncopy1_finish;
o114: a8=Q->A; b8=Q->R4; TRACE1(45)  goto ncopy1_finish;
o118: a8=Q->C; b8=Q->R0; TRACE1(46)  goto ncopy1_finish;
o119: a8=Q->C; b8=Q->R1; TRACE1(47)  goto ncopy1_finish;
o11A: a8=Q->C; b8=Q->R2; TRACE1(48)  goto ncopy1_finish;
o11B: a8=Q->C; b8=Q->R3; TRACE1(49)  goto ncopy1_finish;
o11C: a8=Q->C; b8=Q->R4; TRACE1(50)  goto ncopy1_finish;
		ncopy1_finish: TICKS(19) Q->pc+=3; Ncopy(a8, b8, 16); I+=3; goto o_done;
o120: a8=Q->A; b8=Q->R0; TRACE1(51)  goto nxchg1_finish;
o121: a8=Q->A; b8=Q->R1; TRACE1(52)  goto nxchg1_finish;
o122: a8=Q->A; b8=Q->R2; TRACE1(53)  goto nxchg1_finish;
o123: a8=Q->A; b8=Q->R3; TRACE1(54)  goto nxchg1_finish;
o124: a8=Q->A; b8=Q->R4; TRACE1(55)  goto nxchg1_finish;
o128: a8=Q->C; b8=Q->R0; TRACE1(56)  goto nxchg1_finish;
o129: a8=Q->C; b8=Q->R1; TRACE1(57)  goto nxchg1_finish;
o12A: a8=Q->C; b8=Q->R2; TRACE1(58)  goto nxchg1_finish;
o12B: a8=Q->C; b8=Q->R3; TRACE1(59)  goto nxchg1_finish;
o12C: a8=Q->C; b8=Q->R4; TRACE1(60)  goto nxchg1_finish;
		nxchg1_finish: TICKS(19) Q->pc+=3; Nxchg(a8, b8, 16); I+=3; goto o_done;
		
o130: TICKS(8) TRACE1(61)  Q->pc+=3; Q->d0 = Npack(Q->A, 5); I+=3; goto o_done;
o131: TICKS(8) TRACE1(62)  Q->pc+=3; Q->d1 = Npack(Q->A, 5); I+=3; goto o_done;
o132: TICKS(8) TRACE1(63)  Q->pc+=3; d = Q->d0; Q->d0=Npack(Q->A, 5); Nunpack(Q->A, d, 5); I+=3; goto o_done;
o133: TICKS(8) TRACE1(64)  Q->pc+=3; d = Q->d1; Q->d1=Npack(Q->A, 5); Nunpack(Q->A, d, 5); I+=3; goto o_done;
o134: TICKS(8) TRACE1(65)  Q->pc+=3; Q->d0 = Npack(Q->C, 5); I+=3; goto o_done;
o135: TICKS(8) TRACE1(66)  Q->pc+=3; Q->d1 = Npack(Q->C, 5); I+=3; goto o_done;
o136: TICKS(8) TRACE1(67)  Q->pc+=3; d = Q->d0; Q->d0=Npack(Q->C, 5); Nunpack(Q->C, d, 5); I+=3; goto o_done;
o137: TICKS(8) TRACE1(68)  Q->pc+=3; d = Q->d1; Q->d1=Npack(Q->C, 5); Nunpack(Q->C, d, 5); I+=3; goto o_done;
o138: TICKS(7) TRACE1(69)  Q->pc+=3; Q->d0&=0xf0000; Q->d0|=Npack(Q->A, 4); I+=3; goto o_done;
o139: TICKS(7) TRACE1(70)  Q->pc+=3; Q->d1&=0xf0000; Q->d1|=Npack(Q->A, 4); I+=3; goto o_done;
o13A: TICKS(7) TRACE1(71)  Q->pc+=3; d = Q->d0; Q->d0&=0xf0000; Q->d0|=Npack(Q->A, 4); Nunpack(Q->A, d, 4); I+=3; goto o_done;
o13B: TICKS(7) TRACE1(72)  Q->pc+=3; d = Q->d1; Q->d1&=0xf0000; Q->d1|=Npack(Q->A, 4); Nunpack(Q->A, d, 4); I+=3; goto o_done;
o13C: TICKS(7) TRACE1(73)  Q->pc+=3; Q->d0&=0xf0000; Q->d0|=Npack(Q->C, 4); I+=3; goto o_done;
o13D: TICKS(7) TRACE1(74)  Q->pc+=3; Q->d1&=0xf0000; Q->d1|=Npack(Q->C, 4); I+=3; goto o_done;
o13E: TICKS(7) TRACE1(75)  Q->pc+=3; d = Q->d0; Q->d0&=0xf0000; Q->d0|=Npack(Q->C, 4); Nunpack(Q->C, d, 4); I+=3; goto o_done;
o13F: TICKS(7) TRACE1(76)  Q->pc+=3; d = Q->d1; Q->d1&=0xf0000; Q->d1|=Npack(Q->C, 4); Nunpack(Q->C, d, 4); I+=3; goto o_done;

o140: a8=Q->A; d=Q->d0; TRACE1(77)  goto nwrite1_finish;
o141: a8=Q->A; d=Q->d1; TRACE1(78)  goto nwrite1_finish;
o144: a8=Q->C; d=Q->d0; TRACE1(79)  goto nwrite1_finish;
o145: a8=Q->C; d=Q->d1; TRACE1(80)  goto nwrite1_finish;
		nwrite1_finish: TICKS(17) Q->pc+=3; I+=3; if (Nwrite(a8, d, 5,Q)) INTERRUPT(); goto o_done;
o142: a8=Q->A; d=Q->d0; TRACE1(81)  goto ncread1_finish;
o143: a8=Q->A; d=Q->d1; TRACE1(82)  goto ncread1_finish;
o146: a8=Q->C; d=Q->d0; TRACE1(83)  goto ncread1_finish;
o147: a8=Q->C; d=Q->d1; TRACE1(84)  goto ncread1_finish;
		ncread1_finish: TICKS(18) Q->pc+=3; I+=3; if (NCread(a8, d, 5,Q)) INTERRUPT(); goto o_done;
o148: a8=Q->A; d=Q->d0; TRACE1(85)  goto nwrite2_finish;
o149: a8=Q->A; d=Q->d1; TRACE1(86)  goto nwrite2_finish;
o14C: a8=Q->C; d=Q->d0; TRACE1(87)  goto nwrite2_finish;
o14D: a8=Q->C; d=Q->d1; TRACE1(88)  goto nwrite2_finish;
		nwrite2_finish: TICKS(14) Q->pc+=3; I+=3; if (Nwrite(a8, d, 2,Q)) INTERRUPT(); goto o_done;
o14A: a8=Q->A; d=Q->d0; TRACE1(89)  goto ncread2_finish;
o14B: a8=Q->A; d=Q->d1; TRACE1(90)  goto ncread2_finish;
o14E: a8=Q->C; d=Q->d0; TRACE1(91)  goto ncread2_finish;
o14F: a8=Q->C; d=Q->d1; TRACE1(92)  goto ncread2_finish;
		ncread2_finish: TICKS(15) Q->pc+=3; I+=3; if (NCread(a8, d, 2,Q)) INTERRUPT(); goto o_done;
o150a: a8=Q->A; d=Q->d0; TRACE2(93,I[3])  goto nfwrite_finish;
o151a: a8=Q->A; d=Q->d1; TRACE2(94,I[3])  goto nfwrite_finish;
o154a: a8=Q->C; d=Q->d0; TRACE2(95,I[3])  goto nfwrite_finish;
o155a: a8=Q->C; d=Q->d1; TRACE2(96,I[3])  goto nfwrite_finish;
		nfwrite_finish: TICKS(16+Q->F_l[I[3]]) Q->pc+=4; if (NFwrite(a8, d, I[3],Q)) INTERRUPT() else {I+=4;} goto o_done;
o152a: a8=Q->A; d=Q->d0; TRACE2(97,I[3])  goto ncfread_finish;
o153a: a8=Q->A; d=Q->d1; TRACE2(98,I[3])  goto ncfread_finish;
o156a: a8=Q->C; d=Q->d0; TRACE2(99,I[3])  goto ncfread_finish;
o157a: a8=Q->C; d=Q->d1; TRACE2(100,I[3])  goto ncfread_finish;
		ncfread_finish: TICKS(17+Q->F_l[I[3]]) Q->pc+=4; if (NCFread(a8, d, I[3],Q)) INTERRUPT() else {I+=4;} goto o_done;
o158x: a8=Q->A; d=Q->d0; TRACE2(101,I[3]+1)  goto nwrite3_finish;
o159x: a8=Q->A; d=Q->d1; TRACE2(102,I[3]+1)  goto nwrite3_finish;
o15Cx: a8=Q->C; d=Q->d0; TRACE2(103,I[3]+1)  goto nwrite3_finish;
o15Dx: a8=Q->C; d=Q->d1; TRACE2(104,I[3]+1)  goto nwrite3_finish;
		nwrite3_finish: TICKS(16+I[3]) Q->pc+=4; if (Nwrite(a8, d, I[3]+1,Q)) INTERRUPT() else {I+=4;} goto o_done;
o15Ax: a8=Q->A; d=Q->d0; TRACE2(105,I[3]+1)  goto ncread3_finish;
o15Bx: a8=Q->A; d=Q->d1; TRACE2(106,I[3]+1)  goto ncread3_finish;
o15Ex: a8=Q->C; d=Q->d0; TRACE2(107,I[3]+1)  goto ncread3_finish;
o15Fx: a8=Q->C; d=Q->d1; TRACE2(108,I[3]+1)  goto ncread3_finish;
		ncread3_finish: TICKS(17+I[3]) Q->pc+=4; if (NCread(a8, d, I[3]+1,Q)) INTERRUPT() else {I+=4;} goto o_done;
		
o16x: TICKS(7) TRACE2(109,I[2]+1)  Q->pc+=3; Q->d0+=I[2]+1; if (Q->d0>0xfffff) { Q->d0&=0xfffff; Q->CARRY=1; } else Q->CARRY=0; I+=3; goto o_done;
o17x: TICKS(7) TRACE2(110,I[2]+1)  Q->pc+=3; Q->d1+=I[2]+1; if (Q->d1>0xfffff) { Q->d1&=0xfffff; Q->CARRY=1; } else Q->CARRY=0; I+=3; goto o_done;
o18x: TICKS(7) TRACE2(111,I[2]+1)  Q->pc+=3; Q->d0-=I[2]+1; if (Q->d0>0xfffff) { Q->d0&=0xfffff; Q->CARRY=1; } else Q->CARRY=0; I+=3; goto o_done;
o19d2: TICKS(4) TRACE2(112,Npack(I+2, 2))  Q->pc+=4; Q->d0&=0xfff00; Q->d0|=Npack(I+2, 2); I+=4; goto o_done;
o1Ad4: TICKS(6) TRACE2(113,Npack(I+2, 4))  Q->pc+=6; Q->d0&=0xf0000; Q->d0|=Npack(I+2, 4); I+=6; goto o_done;
o1Bd5: TICKS(7) TRACE2(114,Npack(I+2, 5))  Q->pc+=7; Q->d0=Npack(I+2, 5); I+=7; goto o_done;
o1Cx: TICKS(7) TRACE2(115,I[2]+1)  Q->pc+=3; Q->d1-=I[2]+1; if (Q->d1>0xfffff) { Q->d1&=0xfffff; Q->CARRY=1; } else Q->CARRY=0; I+=3; goto o_done;
o1Dd2: TICKS(4) TRACE2(116,Npack(I+2, 2))  Q->pc+=4; Q->d1&=0xfff00; Q->d1|=Npack(I+2, 2); I+=4; goto o_done;
o1Ed4: TICKS(6) TRACE2(117,Npack(I+2, 4))  Q->pc+=6; Q->d1&=0xf0000; Q->d1|=Npack(I+2, 4); I+=6; goto o_done;
o1Fd5: TICKS(7) TRACE2(118,Npack(I+2, 5))  Q->pc+=7; Q->d1=Npack(I+2, 5); I+=7; goto o_done;
o2n: TICKS(2) TRACE2(119,I[1])  Q->pc+=2; Q->P=I[1]; PCHANGED I+=2; goto o_done;

o3X:  TICKS(4+I[1]) TRACE1(120)  n=I[1]+1; if (Q->P+n<=16) Ncopy(Q->C+Q->P, I+2, n); else { Ncopy(Q->C+Q->P, I+2, 16-Q->P); Ncopy(Q->C, I+2+(16-Q->P), n-(16-Q->P)); } e=2+n; Q->pc+=e; I+=e; goto o_done;
o4d2: TICKS(3) TRACE2(121,(I[2]<<4)+I[1])  if (!Q->CARRY) { TICKS(3) Q->pc+=3; I+=3; goto o_done; } else { TICKS(10) jmp=(I[2]<<4)+I[1]; if (jmp) { Q->pc+=jmp+1; } else { Q->pc=rstkpop(Q); } Q->pc&=0xfffff; goto o_done_resetI; }
o5d2: TICKS(3) TRACE2(122,(I[2]<<4)+I[1])  if ( Q->CARRY) { TICKS(3) Q->pc+=3; I+=3; goto o_done; } else { TICKS(10) jmp=(I[2]<<4)+I[1]; if (jmp) { Q->pc+=jmp+1; } else { Q->pc=rstkpop(Q); } Q->pc&=0xfffff; goto o_done_resetI; }
o6d3: TICKS(11) d=Npack(I+1,3); TRACE2(123,d)  if (d&0x800) Q->pc-=(0xfff-d); else Q->pc+=d+1; Q->pc&=0xfffff; goto o_done_resetI;
o7d3: TICKS(12) rstkpush(Q->pc+4,Q); d=Npack(I+1,3); TRACE2(124,d)  if (d&0x800) Q->pc-=(0xffc-d); else Q->pc+=d+4; Q->pc&=0xfffff; goto o_done_resetI;

o800: TICKS(4) TRACE1(125)  Q->pc+=3; Q->OUT&=0xff0; Q->OUT|=Q->C[0]; I+=3; goto o_done;
o801: TICKS(6) TRACE1(126)  Q->pc+=3; Q->OUT = Npack(Q->C, 3); I+=3; goto o_done;
o802: TICKS(7) TRACE1(127)  Q->pc+=3; update_in(Q); Nunpack(Q->A, Q->IN, 4); I+=3; goto o_done;
o803: TICKS(7) TRACE1(128)  Q->pc+=3; update_in(Q); Nunpack(Q->C, Q->IN, 4); I+=3; goto o_done;
o804: TICKS(12) TRACE1(129)  Q->pc+=3; unconfig(Q); TRACE1(441)  I+=3; goto o_done;
o805: TICKS(11) TRACE1(130)  Q->pc+=3; config(Q); TRACE1(441)  I+=3; goto o_done;
o806: TICKS(11) TRACE1(131)  Q->pc+=3; c_eq_id(Q); TRACE1(441)  I+=3; goto o_done;
o807: TICKS(5) TRACE1(132)  if (Q->SHUTDN) { if (Q->WAKEUP) { Q->SHUTDN=0; Q->pc+=3; I+=3; Q->WAKEUP=0; } } else { Q->SHUTDN=1; Q->WAKEUP=0; } goto o_done;

o8080: TICKS(5) TRACE1(133)  Q->pc+=4; I+=4; Q->INTERRUPT_ENABLED=1; if (Q->INTERRUPT_PENDING && (!Q->INTERRUPT_PROCESSING)) {Q->INTERRUPT_PENDING=0;INTERRUPT("DL");} goto o_done;
o80810: TICKS(6) TRACE1(134)  Q->pc+=5; I+=5; if (Q->IN) {if (!Q->INTERRUPT_PROCESSING) INTERRUPT("RS") else Q->INTERRUPT_PENDING=1;} goto o_done;
o8082X: TICKS(7+I[4]) n=I[4]+1; TRACE1(135)  if (Q->P+n<=16) Ncopy(Q->A+Q->P, I+5, n); else { Ncopy(Q->A+Q->P, I+5, 16-Q->P); Ncopy(Q->A, I+5+(16-Q->P), n-(16-Q->P)); } e=5+n; Q->pc+=e; I+=e; goto o_done;
o8083: TICKS(7) Q->pc+=4; I+=4; TRACE1(136)  goto o_done;
o8084n: TICKS(6) Q->pc+=5; TRACE2(137,I[4])  Nbit0(Q->A, I[4]); I+=5; goto o_done;
o8085n: TICKS(6) Q->pc+=5; TRACE2(138,I[4])  Nbit1(Q->A, I[4]); I+=5; goto o_done;
o8086n: TICKS(9) Q->pc+=5; TRACE2(139,I[4])  Tbit0(Q->A, I[4],Q); goyes(5); 
o8087n: TICKS(9) Q->pc+=5; TRACE2(140,I[4])  Tbit1(Q->A, I[4],Q); goyes(5);
o8088n: TICKS(6) Q->pc+=5; TRACE2(141,I[4])  Nbit0(Q->C, I[4]); I+=5; goto o_done;
o8089n: TICKS(6) Q->pc+=5; TRACE2(142,I[4])  Nbit1(Q->C, I[4]); I+=5; goto o_done;
o808An: TICKS(9) Q->pc+=5; TRACE2(143,I[4])  Tbit0(Q->C, I[4],Q); goyes(5); 
o808Bn: TICKS(9) Q->pc+=5; TRACE2(144,I[4])  Tbit1(Q->C, I[4],Q); goyes(5); 
o808C: TICKS(23) TRACE1(145)  Q->pc = Npack_CRC(GetRealAddressNoIO_ARM(Npack(Q->A,5),Q), 5, Q); goto o_done_resetI;
o808D: TICKS(7) TRACE1(146)  Q->pc+=4; I+=4; goto o_done;
o808E: TICKS(23) TRACE1(147)  Q->pc = Npack_CRC(GetRealAddressNoIO_ARM(Npack(Q->C,5),Q), 5, Q); goto o_done_resetI;
o808F: TICKS(5) TRACE1(148)  Q->pc+=4; Q->INTERRUPT_ENABLED = 0; I+=4; goto o_done;

o809: TICKS(8) TRACE1(149)  Q->pc+=3; d=Npack(Q->C, 5); d+=Q->P+1; if (d>0xfffff) { d&=0xfffff; Q->CARRY=1; } else Q->CARRY=0; Nunpack(Q->C, d, 5); I+=3; goto o_done;
o80A: TICKS(6) TRACE1(150)  Q->pc+=3; reset(Q); TRACE1(441)  I+=3; goto o_done;
o80B: TICKS(6) TRACE1(151)  Q->pc+=3; I+=3; goto o_done;
o80Cn: TICKS(6) TRACE2(152,I[3])  Q->pc+=4; Q->C[I[3]] = Q->P; I+=4; goto o_done;
o80Dn: TICKS(6) TRACE2(153,I[3])  Q->pc+=4; Q->P = Q->C[I[3]]; PCHANGED I+=4; goto o_done;
o80E: TICKS(7) TRACE1(154)  Q->pc+=3; Q->C[0]=0; I+=3; goto o_done;
o80Fn: TICKS(6) TRACE2(155,I[3])  Q->pc+=4; n = Q->P; Q->P = Q->C[I[3]]; Q->C[I[3]] = n; PCHANGED I+=4; goto o_done;

o810: a8=Q->A; TRACE1(156)  goto nslc_finish;
o811: a8=Q->B; TRACE1(157)  goto nslc_finish;
o812: a8=Q->C; TRACE1(158)  goto nslc_finish;
o813: a8=Q->D; TRACE1(159)  goto nslc_finish;
		nslc_finish: TICKS(16) Q->pc+=3; Nslc(a8, 16); I+=3; goto o_done;
o814: a8=Q->A; TRACE1(160)  goto nsrc_finish;
o815: a8=Q->B; TRACE1(161)  goto nsrc_finish;
o816: a8=Q->C; TRACE1(162)  goto nsrc_finish;
o817: a8=Q->D; TRACE1(163)  goto nsrc_finish;
		nsrc_finish: TICKS(16) Q->pc+=3; Nsrc(a8, 16, Q); I+=3; goto o_done;
o818f0x: a8=Q->A; n=I[3]; TRACE3(164,I[5],n)  goto nfunpack_nfadd_finish;
o818f1x: a8=Q->B; n=I[3]; TRACE3(165,I[5],n)  goto nfunpack_nfadd_finish;
o818f2x: a8=Q->C; n=I[3]; TRACE3(166,I[5],n)  goto nfunpack_nfadd_finish;
o818f3x: a8=Q->D; n=I[3]; TRACE3(167,I[5],n)  goto nfunpack_nfadd_finish;
		nfunpack_nfadd_finish: TICKS(5+Q->F_l[n]) Q->pc+=6; NFaddconstant(a8, n, I[5], Q); I+=6; goto o_done;
o818f8x: a8=Q->A; n=I[3]; TRACE3(168,I[5],n)  goto nfunpack_nfsub_finish;
o818f9x: a8=Q->B; n=I[3]; TRACE3(169,I[5],n)  goto nfunpack_nfsub_finish;
o818fAx: a8=Q->C; n=I[3]; TRACE3(170,I[5],n)  goto nfunpack_nfsub_finish;
o818fBx: a8=Q->D; n=I[3]; TRACE3(171,I[5],n)  goto nfunpack_nfsub_finish;
		nfunpack_nfsub_finish: TICKS(5+Q->F_l[n]) Q->pc+=6; NFsubconstant(a8, n, I[5], Q); I+=6; goto o_done;
o819f0: a8=Q->A; TRACE2(172,I[3])  goto nfsrb_finish;
o819f1: a8=Q->B; TRACE2(173,I[3])  goto nfsrb_finish;
o819f2: a8=Q->C; TRACE2(174,I[3])  goto nfsrb_finish;
o819f3: a8=Q->D; TRACE2(175,I[3])  goto nfsrb_finish;
		nfsrb_finish: TICKS(20) Q->pc+=5; NFsrb(a8, I[3],Q); I+=5; goto o_done;
o81Af00: a8=Q->R0; b8=Q->A; TRACE2(176,I[3])  goto nfcopy1_finish;
o81Af01: a8=Q->R1; b8=Q->A; TRACE2(177,I[3])  goto nfcopy1_finish;
o81Af02: a8=Q->R2; b8=Q->A; TRACE2(178,I[3])  goto nfcopy1_finish;
o81Af03: a8=Q->R3; b8=Q->A; TRACE2(179,I[3])  goto nfcopy1_finish;
o81Af04: a8=Q->R4; b8=Q->A; TRACE2(180,I[3])  goto nfcopy1_finish;
o81Af08: a8=Q->R0; b8=Q->C; TRACE2(181,I[3])  goto nfcopy1_finish;
o81Af09: a8=Q->R1; b8=Q->C; TRACE2(182,I[3])  goto nfcopy1_finish;
o81Af0A: a8=Q->R2; b8=Q->C; TRACE2(183,I[3])  goto nfcopy1_finish;
o81Af0B: a8=Q->R3; b8=Q->C; TRACE2(184,I[3])  goto nfcopy1_finish;
o81Af0C: a8=Q->R4; b8=Q->C; TRACE2(185,I[3])  goto nfcopy1_finish;
o81Af10: a8=Q->A; b8=Q->R0; TRACE2(186,I[3])  goto nfcopy1_finish;
o81Af11: a8=Q->A; b8=Q->R1; TRACE2(187,I[3])  goto nfcopy1_finish;
o81Af12: a8=Q->A; b8=Q->R2; TRACE2(188,I[3])  goto nfcopy1_finish;
o81Af13: a8=Q->A; b8=Q->R3; TRACE2(189,I[3])  goto nfcopy1_finish;
o81Af14: a8=Q->A; b8=Q->R4; TRACE2(190,I[3])  goto nfcopy1_finish;
o81Af18: a8=Q->C; b8=Q->R0; TRACE2(191,I[3])  goto nfcopy1_finish;
o81Af19: a8=Q->C; b8=Q->R1; TRACE2(192,I[3])  goto nfcopy1_finish;
o81Af1A: a8=Q->C; b8=Q->R2; TRACE2(193,I[3])  goto nfcopy1_finish;
o81Af1B: a8=Q->C; b8=Q->R3; TRACE2(194,I[3])  goto nfcopy1_finish;
o81Af1C: a8=Q->C; b8=Q->R4; TRACE2(195,I[3])  goto nfcopy1_finish;
		nfcopy1_finish: TICKS(6+Q->F_l[I[3]]) Q->pc+=6; NFcopy(a8, b8, I[3],Q); I+=6; goto o_done;
o81Af20: a8=Q->A; b8=Q->R0; TRACE2(196,I[3])  goto nfxchg1_finish;
o81Af21: a8=Q->A; b8=Q->R1; TRACE2(197,I[3])  goto nfxchg1_finish;
o81Af22: a8=Q->A; b8=Q->R2; TRACE2(198,I[3])  goto nfxchg1_finish;
o81Af23: a8=Q->A; b8=Q->R3; TRACE2(199,I[3])  goto nfxchg1_finish;
o81Af24: a8=Q->A; b8=Q->R4; TRACE2(200,I[3])  goto nfxchg1_finish;
o81Af28: a8=Q->C; b8=Q->R0; TRACE2(201,I[3])  goto nfxchg1_finish;
o81Af29: a8=Q->C; b8=Q->R1; TRACE2(202,I[3])  goto nfxchg1_finish;
o81Af2A: a8=Q->C; b8=Q->R2; TRACE2(203,I[3])  goto nfxchg1_finish;
o81Af2B: a8=Q->C; b8=Q->R3; TRACE2(204,I[3])  goto nfxchg1_finish;
o81Af2C: a8=Q->C; b8=Q->R4; TRACE2(205,I[3])  goto nfxchg1_finish;
		nfxchg1_finish: TICKS(6+Q->F_l[I[3]]) Q->pc+=6; NFxchg(a8, b8, I[3],Q); I+=6; goto o_done;
o81B1: TRACE1(206)  EmulateBeep_ARM(Q); PCHANGED Q->pc=rstkpop(Q); goto o_done_resetI;
o81B2: TICKS(16) TRACE1(207)  Q->pc = Npack(Q->A, 5); goto o_done_resetI;
o81B3: TICKS(16) TRACE1(208)  Q->pc = Npack(Q->C, 5); goto o_done_resetI;
o81B4: TICKS(9) TRACE1(209)  Q->pc+=4; Nunpack(Q->A, Q->pc, 5); I+=4; goto o_done;
o81B5: TICKS(9) TRACE1(210)  Q->pc+=4; Nunpack(Q->C, Q->pc, 5); I+=4; goto o_done;
o81B6: TICKS(16) TRACE1(211)  d = Q->pc+4; Q->pc = Npack(Q->A, 5); Nunpack(Q->A, d, 5); goto o_done_resetI;
o81B7: TICKS(16) TRACE1(212)  d = Q->pc+4; Q->pc = Npack(Q->C, 5); Nunpack(Q->C, d, 5); goto o_done_resetI;


o81C: a8=Q->A; TRACE1(213)  goto nsrb_finish;
o81D: a8=Q->B; TRACE1(214)  goto nsrb_finish;
o81E: a8=Q->C; TRACE1(215)  goto nsrb_finish;
o81F: a8=Q->D; TRACE1(216)  goto nsrb_finish;
		nsrb_finish: TICKS(20) Q->pc+=3; Nsrb(a8, 16,Q); I+=3; goto o_done;
o82n: TICKS(3) TRACE2(217,I[2])  Q->pc+=3; Q->HST&=~I[2]; I+=3; goto o_done;
o83n: TICKS(6) TRACE2(218,I[2])  Q->pc+=3; Q->CARRY=(Q->HST&I[2])?0:1; goyes(3);
o84n: TICKS(4) TRACE2(219,I[2])  Q->pc+=3; Nbit0(Q->ST, I[2]); I+=3; goto o_done;
o85n: TICKS(4) TRACE2(220,I[2])  Q->pc+=3; Nbit1(Q->ST, I[2]); I+=3; goto o_done;
o86n: TICKS(7) TRACE2(221,I[2])  Q->pc+=3; Tbit0(Q->ST, I[2],Q); goyes(3); 
o87n: TICKS(7) TRACE2(222,I[2])  Q->pc+=3; Tbit1(Q->ST, I[2],Q); goyes(3); 
o88n: TICKS(6) TRACE2(223,I[2])  Q->pc+=3; if (Q->P!=I[2]) Q->CARRY=1; else Q->CARRY=0; goyes(3); 
o89n: TICKS(6) TRACE2(224,I[2])  Q->pc+=3; if (Q->P==I[2]) Q->CARRY=1; else Q->CARRY=0; goyes(3); 
o8A0: a8=Q->A; b8=Q->B; TRACE1(225)  goto te_finish;
o8A1: a8=Q->B; b8=Q->C; TRACE1(226)  goto te_finish;
o8A2: a8=Q->C; b8=Q->A; TRACE1(227)  goto te_finish;
o8A3: a8=Q->D; b8=Q->C; TRACE1(228)  goto te_finish;
		te_finish: TICKS(11) Q->pc+=3; Te(a8, b8, 5,Q); goyes(3); 
o8A4: a8=Q->A; b8=Q->B; TRACE1(229)  goto tne_finish;
o8A5: a8=Q->B; b8=Q->C; TRACE1(230)  goto tne_finish;
o8A6: a8=Q->C; b8=Q->A; TRACE1(231)  goto tne_finish;
o8A7: a8=Q->D; b8=Q->C; TRACE1(232)  goto tne_finish;
		tne_finish: TICKS(11) Q->pc+=3; Tne(a8, b8, 5,Q); goyes(3); 
o8A8: a8=Q->A; TRACE1(233)  goto tz_finish;
o8A9: a8=Q->B; TRACE1(234)  goto tz_finish;
o8AA: a8=Q->C; TRACE1(235)  goto tz_finish;
o8AB: a8=Q->D; TRACE1(236)  goto tz_finish;
		tz_finish: TICKS(11) Q->pc+=3; Tz(a8, 5,Q); goyes(3); 
o8AC: a8=Q->A; TRACE1(237)  goto tnz_finish;
o8AD: a8=Q->B; TRACE1(238)  goto tnz_finish;
o8AE: a8=Q->C; TRACE1(239)  goto tnz_finish;
o8AF: a8=Q->D; TRACE1(240)  goto tnz_finish;
		tnz_finish: TICKS(11) Q->pc+=3; Tnz(a8, 5,Q); goyes(3); 
o8B0: a8=Q->A; b8=Q->B; TRACE1(241)  goto ta_finish;
o8B1: a8=Q->B; b8=Q->C; TRACE1(242)  goto ta_finish;
o8B2: a8=Q->C; b8=Q->A; TRACE1(243)  goto ta_finish;
o8B3: a8=Q->D; b8=Q->C; TRACE1(244)  goto ta_finish;
		ta_finish: TICKS(11) Q->pc+=3; Ta(a8, b8, 5,Q); goyes(3); 
o8B4: a8=Q->A; b8=Q->B; TRACE1(245)  goto tb_finish;
o8B5: a8=Q->B; b8=Q->C; TRACE1(246)  goto tb_finish;
o8B6: a8=Q->C; b8=Q->A; TRACE1(247)  goto tb_finish;
o8B7: a8=Q->D; b8=Q->C; TRACE1(248)  goto tb_finish;
		tb_finish: TICKS(11) Q->pc+=3; Tb(a8, b8, 5,Q); goyes(3); 
o8B8: a8=Q->A; b8=Q->B; TRACE1(249)  goto tae_finish;
o8B9: a8=Q->B; b8=Q->C; TRACE1(250)  goto tae_finish;
o8BA: a8=Q->C; b8=Q->A; TRACE1(251)  goto tae_finish;
o8BB: a8=Q->D; b8=Q->C; TRACE1(252)  goto tae_finish;
		tae_finish: TICKS(11) Q->pc+=3; Tae(a8, b8, 5,Q); goyes(3); 
o8BC: a8=Q->A; b8=Q->B; TRACE1(253)  goto tbe_finish;
o8BD: a8=Q->B; b8=Q->C; TRACE1(254)  goto tbe_finish;
o8BE: a8=Q->C; b8=Q->A; TRACE1(255)  goto tbe_finish;
o8BF: a8=Q->D; b8=Q->C; TRACE1(256)  goto tbe_finish;
		tbe_finish: TICKS(11) Q->pc+=3; Tbe(a8, b8, 5,Q); goyes(3); 
		
o8Cd4: TICKS(14) d=Npack(I+2, 4); TRACE2(257,d)  if (d&0x8000) Q->pc -= (0xfffe - d); else Q->pc += d+2; Q->pc&=0xfffff; goto o_done_resetI;
o8Dd5: TICKS(14) TRACE2(258,Npack(I+2, 5))  Q->pc=Npack(I+2, 5); goto o_done_resetI;
o8Ed4: TICKS(14) rstkpush(Q->pc+6,Q); d=Npack(I+2,4); TRACE2(259,d)  if (d&0x8000) Q->pc -= (0xfffa-d); else Q->pc += d+6; Q->pc&=0xfffff; goto o_done_resetI;
o8Fd5: TICKS(15) TRACE2(260,Npack(I+2, 5))  rstkpush(Q->pc+7,Q); Q->pc=Npack(I+2, 5); goto o_done_resetI;

o9a0: a8=Q->A; b8=Q->B; TRACE2(261,I[1])  goto tfe_finish;
o9a1: a8=Q->B; b8=Q->C; TRACE2(262,I[1])  goto tfe_finish;
o9a2: a8=Q->C; b8=Q->A; TRACE2(263,I[1])  goto tfe_finish;
o9a3: a8=Q->D; b8=Q->C; TRACE2(264,I[1])  goto tfe_finish;
		tfe_finish: TICKS(6+Q->F_l[I[1]]) Q->pc+=3; TFe(a8, b8, I[1],Q); goyes(3); 
o9a4: a8=Q->A; b8=Q->B; TRACE2(265,I[1])  goto tfne_finish;
o9a5: a8=Q->B; b8=Q->C; TRACE2(266,I[1])  goto tfne_finish;
o9a6: a8=Q->C; b8=Q->A; TRACE2(267,I[1])  goto tfne_finish;
o9a7: a8=Q->D; b8=Q->C; TRACE2(268,I[1])  goto tfne_finish;
		tfne_finish: TICKS(6+Q->F_l[I[1]]) Q->pc+=3; TFne(a8, b8, I[1],Q); goyes(3); 
o9a8: a8=Q->A; TRACE2(269,I[1])  goto tfz_finish;
o9a9: a8=Q->B; TRACE2(270,I[1])  goto tfz_finish;
o9aA: a8=Q->C; TRACE2(271,I[1])  goto tfz_finish;
o9aB: a8=Q->D; TRACE2(272,I[1])  goto tfz_finish;
		tfz_finish: TICKS(6+Q->F_l[I[1]]) Q->pc+=3; TFz(a8, I[1],Q); goyes(3); 
o9aC: a8=Q->A; TRACE2(273,I[1])  goto tfnz_finish;
o9aD: a8=Q->B; TRACE2(274,I[1])  goto tfnz_finish;
o9aE: a8=Q->C; TRACE2(275,I[1])  goto tfnz_finish;
o9aF: a8=Q->D; TRACE2(276,I[1])  goto tfnz_finish;
		tfnz_finish: TICKS(6+Q->F_l[I[1]]) Q->pc+=3; TFnz(a8, I[1],Q); goyes(3); 
o9b0: a8=Q->A; b8=Q->B; TRACE2(277,I[1]&7)  goto tfa_finish;
o9b1: a8=Q->B; b8=Q->C; TRACE2(278,I[1]&7)  goto tfa_finish;
o9b2: a8=Q->C; b8=Q->A; TRACE2(279,I[1]&7)  goto tfa_finish;
o9b3: a8=Q->D; b8=Q->C; TRACE2(280,I[1]&7)  goto tfa_finish;
		tfa_finish: TICKS(6+Q->F_l[I[1]&7]) Q->pc+=3; TFa(a8, b8, I[1]&7,Q); goyes(3); 
o9b4: a8=Q->A; b8=Q->B; TRACE2(281,I[1]&7)  goto tfb_finish;
o9b5: a8=Q->B; b8=Q->C; TRACE2(282,I[1]&7)  goto tfb_finish;
o9b6: a8=Q->C; b8=Q->A; TRACE2(283,I[1]&7)  goto tfb_finish;
o9b7: a8=Q->D; b8=Q->C; TRACE2(284,I[1]&7)  goto tfb_finish;
		tfb_finish: TICKS(6+Q->F_l[I[1]&7]) Q->pc+=3; TFb(a8, b8, I[1]&7,Q); goyes(3); 
o9b8: a8=Q->A; b8=Q->B; TRACE2(285,I[1]&7)  goto tfae_finish;
o9b9: a8=Q->B; b8=Q->C; TRACE2(286,I[1]&7)  goto tfae_finish;
o9bA: a8=Q->C; b8=Q->A; TRACE2(287,I[1]&7)  goto tfae_finish;
o9bB: a8=Q->D; b8=Q->C; TRACE2(288,I[1]&7)  goto tfae_finish;
		tfae_finish: TICKS(6+Q->F_l[I[1]&7]) Q->pc+=3; TFae(a8, b8, I[1]&7,Q); goyes(3); 
o9bC: a8=Q->A; b8=Q->B; TRACE2(289,I[1]&7)  goto tfbe_finish;
o9bD: a8=Q->B; b8=Q->C; TRACE2(290,I[1]&7)  goto tfbe_finish;
o9bE: a8=Q->C; b8=Q->A; TRACE2(291,I[1]&7)  goto tfbe_finish;
o9bF: a8=Q->D; b8=Q->C; TRACE2(292,I[1]&7)  goto tfbe_finish;
		tfbe_finish: TICKS(6+Q->F_l[I[1]&7]) Q->pc+=3; TFbe(a8, b8, I[1]&7,Q); goyes(3); 
oAa0: a8=Q->A; b8=Q->B; TRACE2(293,I[1])  goto nfadd_finish;
oAa1: a8=Q->B; b8=Q->C; TRACE2(294,I[1])  goto nfadd_finish;
oAa2: a8=Q->C; b8=Q->A; TRACE2(295,I[1])  goto nfadd_finish;
oAa3: a8=Q->D; b8=Q->C; TRACE2(296,I[1])  goto nfadd_finish;
oAa4: a8=Q->A; b8=Q->A; TRACE2(297,I[1])  goto nfadd_finish;
oAa5: a8=Q->B; b8=Q->B; TRACE2(298,I[1])  goto nfadd_finish;
oAa6: a8=Q->C; b8=Q->C; TRACE2(299,I[1])  goto nfadd_finish;
oAa7: a8=Q->D; b8=Q->D; TRACE2(300,I[1])  goto nfadd_finish;
oAa8: a8=Q->B; b8=Q->A; TRACE2(301,I[1])  goto nfadd_finish;
oAa9: a8=Q->C; b8=Q->B; TRACE2(302,I[1])  goto nfadd_finish;
oAaA: a8=Q->A; b8=Q->C; TRACE2(303,I[1])  goto nfadd_finish;
oAaB: a8=Q->C; b8=Q->D; TRACE2(304,I[1])  goto nfadd_finish;
		nfadd_finish: TICKS(3+Q->F_l[I[1]]) Q->pc+=3; NFadd(a8, b8, I[1],Q); I+=3; goto o_done;
oAaC: a8=Q->A; TRACE2(305,I[1])  goto nfdec_finish;
oAaD: a8=Q->B; TRACE2(306,I[1])  goto nfdec_finish;
oAaE: a8=Q->C; TRACE2(307,I[1])  goto nfdec_finish;
oAaF: a8=Q->D; TRACE2(308,I[1])  goto nfdec_finish;
		nfdec_finish: TICKS(3+Q->F_l[I[1]]) Q->pc+=3; NFdec(a8, I[1],Q); I+=3; goto o_done;
oAb0: a8=Q->A; TRACE2(309,I[1]&7)  goto nfzero_finish;
oAb1: a8=Q->B; TRACE2(310,I[1]&7)  goto nfzero_finish;
oAb2: a8=Q->C; TRACE2(311,I[1]&7)  goto nfzero_finish;
oAb3: a8=Q->D; TRACE2(312,I[1]&7)  goto nfzero_finish;
		nfzero_finish: TICKS(3+Q->F_l[I[1]&7]) Q->pc+=3; NFzero(a8, I[1]&7,Q); I+=3; goto o_done;
oAb4: a8=Q->A; b8=Q->B; TRACE2(313,I[1]&7)  goto nfcopy2_finish;
oAb5: a8=Q->B; b8=Q->C; TRACE2(314,I[1]&7)  goto nfcopy2_finish;
oAb6: a8=Q->C; b8=Q->A; TRACE2(315,I[1]&7)  goto nfcopy2_finish;
oAb7: a8=Q->D; b8=Q->C; TRACE2(316,I[1]&7)  goto nfcopy2_finish;
oAb8: a8=Q->B; b8=Q->A; TRACE2(317,I[1]&7)  goto nfcopy2_finish;
oAb9: a8=Q->C; b8=Q->B; TRACE2(318,I[1]&7)  goto nfcopy2_finish;
oAbA: a8=Q->A; b8=Q->C; TRACE2(319,I[1]&7)  goto nfcopy2_finish;
oAbB: a8=Q->C; b8=Q->D; TRACE2(320,I[1]&7)  goto nfcopy2_finish;
		nfcopy2_finish: TICKS(3+Q->F_l[I[1]&7]) Q->pc+=3; NFcopy(a8, b8, I[1]&7,Q); I+=3; goto o_done;
oAbC: a8=Q->A; b8=Q->B; TRACE2(321,I[1]&7)  goto nfxchg2_finish;
oAbD: a8=Q->B; b8=Q->C; TRACE2(322,I[1]&7)  goto nfxchg2_finish;
oAbE: a8=Q->C; b8=Q->A; TRACE2(323,I[1]&7)  goto nfxchg2_finish;
oAbF: a8=Q->D; b8=Q->C; TRACE2(324,I[1]&7)  goto nfxchg2_finish;
		nfxchg2_finish: TICKS(3+Q->F_l[I[1]&7]) Q->pc+=3; NFxchg(a8, b8, I[1]&7,Q); I+=3; goto o_done;
oBa0: a8=Q->A; b8=Q->B; TRACE2(325,I[1])  goto nfsub_finish;
oBa1: a8=Q->B; b8=Q->C; TRACE2(326,I[1])  goto nfsub_finish;
oBa2: a8=Q->C; b8=Q->A; TRACE2(327,I[1])  goto nfsub_finish;
oBa3: a8=Q->D; b8=Q->C; TRACE2(328,I[1])  goto nfsub_finish;
oBa8: a8=Q->B; b8=Q->A; TRACE2(329,I[1])  goto nfsub_finish;
oBa9: a8=Q->C; b8=Q->B; TRACE2(330,I[1])  goto nfsub_finish;
oBaA: a8=Q->A; b8=Q->C; TRACE2(331,I[1])  goto nfsub_finish;
oBaB: a8=Q->C; b8=Q->D; TRACE2(332,I[1])  goto nfsub_finish;
		nfsub_finish: TICKS(3+Q->F_l[I[1]]) Q->pc+=3; NFsub(a8, b8, I[1],Q); I+=3; goto o_done;
oBa4: a8=Q->A; TRACE2(333,I[1])  goto nfinc_finish;
oBa5: a8=Q->B; TRACE2(334,I[1])  goto nfinc_finish;
oBa6: a8=Q->C; TRACE2(335,I[1])  goto nfinc_finish;
oBa7: a8=Q->D; TRACE2(336,I[1])  goto nfinc_finish;
		nfinc_finish: TICKS(3+Q->F_l[I[1]]) Q->pc+=3; NFinc(a8, I[1],Q); I+=3; goto o_done;
oBaC: a8=Q->A; b8=Q->B; TRACE2(337,I[1])  goto nfrsub_finish;
oBaD: a8=Q->B; b8=Q->C; TRACE2(338,I[1])  goto nfrsub_finish;
oBaE: a8=Q->C; b8=Q->A; TRACE2(339,I[1])  goto nfrsub_finish;
oBaF: a8=Q->D; b8=Q->C; TRACE2(340,I[1])  goto nfrsub_finish;
		nfrsub_finish: TICKS(3+Q->F_l[I[1]]) Q->pc+=3; NFrsub(a8, b8, I[1],Q); I+=3; goto o_done;
oBb0: a8=Q->A; TRACE2(341,I[1]&7)  goto nfsl_finish;
oBb1: a8=Q->B; TRACE2(342,I[1]&7)  goto nfsl_finish;
oBb2: a8=Q->C; TRACE2(343,I[1]&7)  goto nfsl_finish;
oBb3: a8=Q->D; TRACE2(344,I[1]&7)  goto nfsl_finish;
		nfsl_finish: TICKS(3+Q->F_l[I[1]&7]) Q->pc+=3; NFsl(a8, I[1]&7,Q); I+=3; goto o_done;
oBb4: a8=Q->A; TRACE2(345,I[1]&7)  goto nfsr_finish;
oBb5: a8=Q->B; TRACE2(346,I[1]&7)  goto nfsr_finish;
oBb6: a8=Q->C; TRACE2(347,I[1]&7)  goto nfsr_finish;
oBb7: a8=Q->D; TRACE2(348,I[1]&7)  goto nfsr_finish;
		nfsr_finish: TICKS(3+Q->F_l[I[1]&7]) Q->pc+=3; NFsr(a8, I[1]&7,Q); I+=3; goto o_done;
oBb8: a8=Q->A; TRACE2(349,I[1]&7)  goto nfneg_finish;
oBb9: a8=Q->B; TRACE2(350,I[1]&7)  goto nfneg_finish;
oBbA: a8=Q->C; TRACE2(351,I[1]&7)  goto nfneg_finish;
oBbB: a8=Q->D; TRACE2(352,I[1]&7)  goto nfneg_finish;
		nfneg_finish: TICKS(3+Q->F_l[I[1]&7]) Q->pc+=3; NFneg(a8, I[1]&7,Q); I+=3; goto o_done;
oBbC: a8=Q->A; TRACE2(353,I[1]&7)  goto nfnot_finish;
oBbD: a8=Q->B; TRACE2(354,I[1]&7)  goto nfnot_finish;
oBbE: a8=Q->C; TRACE2(355,I[1]&7)  goto nfnot_finish;
oBbF: a8=Q->D; TRACE2(356,I[1]&7)  goto nfnot_finish;
		nfnot_finish: TICKS(3+Q->F_l[I[1]&7]) Q->pc+=3; NFnot(a8, I[1]&7,Q); I+=3; goto o_done;
oC0: a8=Q->A; b8=Q->B; TRACE1(357)  goto nadd_finish;
oC1: a8=Q->B; b8=Q->C; TRACE1(358)  goto nadd_finish;
oC2: a8=Q->C; b8=Q->A; TRACE1(359)  goto nadd_finish;
oC3: a8=Q->D; b8=Q->C; TRACE1(360)  goto nadd_finish;
oC4: a8=Q->A; b8=Q->A; TRACE1(361)  goto nadd_finish;
oC5: a8=Q->B; b8=Q->B; TRACE1(362)  goto nadd_finish;
oC6: a8=Q->C; b8=Q->C; TRACE1(363)  goto nadd_finish;
oC7: a8=Q->D; b8=Q->D; TRACE1(364)  goto nadd_finish;
oC8: a8=Q->B; b8=Q->A; TRACE1(365)  goto nadd_finish;
oC9: a8=Q->C; b8=Q->B; TRACE1(366)  goto nadd_finish;
oCA: a8=Q->A; b8=Q->C; TRACE1(367)  goto nadd_finish;
oCB: a8=Q->C; b8=Q->D; TRACE1(368)  goto nadd_finish;
		nadd_finish: TICKS(7) Q->pc+=2; Nadd(a8, b8, 5,Q); I+=2; goto o_done;
oCC: a8=Q->A; TRACE1(369)  goto ndec_finish;
oCD: a8=Q->B; TRACE1(370)  goto ndec_finish;
oCE: a8=Q->C; TRACE1(371)  goto ndec_finish;
oCF: a8=Q->D; TRACE1(372)  goto ndec_finish;
		ndec_finish: TICKS(7) Q->pc+=2; Ndec_5(a8, Q); I+=2; goto o_done;
oD0: a8=Q->A; TRACE1(373)  goto nzero_finish;
oD1: a8=Q->B; TRACE1(374)  goto nzero_finish;
oD2: a8=Q->C; TRACE1(375)  goto nzero_finish;
oD3: a8=Q->D; TRACE1(376)  goto nzero_finish;
		nzero_finish: TICKS(7) Q->pc+=2; Nzero(a8, 5); I+=2; goto o_done;
oD4: a8=Q->A; b8=Q->B; TRACE1(377)  goto ncopy2_finish;
oD5: a8=Q->B; b8=Q->C; TRACE1(378)  goto ncopy2_finish;
oD6: a8=Q->C; b8=Q->A; TRACE1(379)  goto ncopy2_finish;
oD7: a8=Q->D; b8=Q->C; TRACE1(380)  goto ncopy2_finish;
oD8: a8=Q->B; b8=Q->A; TRACE1(381)  goto ncopy2_finish;
oD9: a8=Q->C; b8=Q->B; TRACE1(382)  goto ncopy2_finish;
oDA: a8=Q->A; b8=Q->C; TRACE1(383)  goto ncopy2_finish;
oDB: a8=Q->C; b8=Q->D; TRACE1(384)  goto ncopy2_finish;
		ncopy2_finish: TICKS(7) Q->pc+=2; Ncopy(a8, b8, 5); I+=2; goto o_done;
oDC: a8=Q->A; b8=Q->B; TRACE1(385)  goto nxchg2_finish;
oDD: a8=Q->B; b8=Q->C; TRACE1(386)  goto nxchg2_finish;
oDE: a8=Q->C; b8=Q->A; TRACE1(387)  goto nxchg2_finish;
oDF: a8=Q->D; b8=Q->C; TRACE1(388)  goto nxchg2_finish;
		nxchg2_finish: TICKS(7) Q->pc+=2; Nxchg(a8, b8, 5); I+=2; goto o_done;
oE0: a8=Q->A; b8=Q->B; TRACE1(389)  goto nsub_finish;
oE1: a8=Q->B; b8=Q->C; TRACE1(390)  goto nsub_finish;
oE2: a8=Q->C; b8=Q->A; TRACE1(391)  goto nsub_finish;
oE3: a8=Q->D; b8=Q->C; TRACE1(392)  goto nsub_finish;
oE8: a8=Q->B; b8=Q->A; TRACE1(393)  goto nsub_finish;
oE9: a8=Q->C; b8=Q->B; TRACE1(394)  goto nsub_finish;
oEA: a8=Q->A; b8=Q->C; TRACE1(395)  goto nsub_finish;
oEB: a8=Q->C; b8=Q->D; TRACE1(396)  goto nsub_finish;
		nsub_finish: TICKS(7) Q->pc+=2; Nsub(a8, b8, 5,Q); I+=2; goto o_done;
oE4: a8=Q->A; TRACE1(397)  goto ninc_finish;
oE5: a8=Q->B; TRACE1(398)  goto ninc_finish;
oE6: a8=Q->C; TRACE1(399)  goto ninc_finish;
oE7: a8=Q->D; TRACE1(400)  goto ninc_finish;
		ninc_finish: TICKS(7) Q->pc+=2; Ninc_5(a8, Q); I+=2; goto o_done;
oEC: a8=Q->A; b8=Q->B; TRACE1(401)  goto nrsub_finish;
oED: a8=Q->B; b8=Q->C; TRACE1(402)  goto nrsub_finish;
oEE: a8=Q->C; b8=Q->A; TRACE1(403)  goto nrsub_finish;
oEF: a8=Q->D; b8=Q->C; TRACE1(404)  goto nrsub_finish;
		nrsub_finish: TICKS(7) Q->pc+=2; Nrsub(a8, b8, 5,Q); I+=2; goto o_done;
oF0: a8=Q->A; TRACE1(405)  goto nsl_finish;
oF1: a8=Q->B; TRACE1(406)  goto nsl_finish;
oF2: a8=Q->C; TRACE1(407)  goto nsl_finish;
oF3: a8=Q->D; TRACE1(408)  goto nsl_finish;
		nsl_finish: TICKS(7) Q->pc+=2; Nsl(a8, 5); I+=2; goto o_done;
oF4: a8=Q->A; TRACE1(409)  goto nsr_finish;
oF5: a8=Q->B; TRACE1(410)  goto nsr_finish;
oF6: a8=Q->C; TRACE1(411)  goto nsr_finish;
oF7: a8=Q->D; TRACE1(412)  goto nsr_finish;
		nsr_finish: TICKS(7) Q->pc+=2; Nsr(a8, 5,Q); I+=2; goto o_done;
oF8: a8=Q->A; TRACE1(413)  goto nneg_finish;
oF9: a8=Q->B; TRACE1(414)  goto nneg_finish;
oFA: a8=Q->C; TRACE1(415)  goto nneg_finish;
oFB: a8=Q->D; TRACE1(416)  goto nneg_finish;
		nneg_finish: TICKS(7) Q->pc+=2; Nneg(a8, 5,Q); I+=2; goto o_done;
oFC: a8=Q->A; TRACE1(417)  goto nnot_finish;
oFD: a8=Q->B; TRACE1(418)  goto nnot_finish;
oFE: a8=Q->C; TRACE1(419)  goto nnot_finish;
oFF: a8=Q->D; TRACE1(420)  goto nnot_finish;
		nnot_finish: TICKS(7) Q->pc+=2; Nnot(a8, 5,Q); I+=2; goto o_done;



o_invalid:
	TRACE1(421)
	Q->statusQuit=1;
	SwapAllData_arm(Q);
	return 0;

o_invalid3: TRACE2(422,((UInt32)I[0]<<8 )|((UInt32)I[1]<<4) | (UInt32)I[2])  Q->pc+=3; I+=3; goto o_done;
o_invalid4: TRACE2(423,((UInt32)I[0]<<12)|((UInt32)I[1]<<8) |((UInt32)I[2]<<4) | (UInt32)I[3])  Q->pc+=4; I+=4; goto o_done;
o_invalid5: TRACE2(424,((UInt32)I[0]<<16)|((UInt32)I[1]<<12)|((UInt32)I[2]<<8) |((UInt32)I[3]<<4)| (UInt32)I[4])  Q->pc+=5; I+=5; goto o_done;
o_invalid6: TRACE2(425,((UInt32)I[0]<<20)|((UInt32)I[1]<<16)|((UInt32)I[2]<<12)|((UInt32)I[3]<<8)|((UInt32)I[4]<<4)|(UInt32)I[5])  Q->pc+=6; I+=6; goto o_done;

o_goyes:
	    if (!Q->CARRY) { Q->pc+=2; I+=2; goto o_done; }
	    TICKS(7)
	    jmp = GOJMP[0] | (GOJMP[1]<<4);
	    if (jmp) 
	    {
	    	Q->pc+=jmp;
	    	Q->pc&=0xfffff;
	    }
	    else 
	    {
	    	Q->pc=rstkpop(Q);
	    }
	    
o_done_resetI:
    	I=GetRealAddressNoIO_ARM(Q->pc,Q);

o_done:
		if (++instCount<t2ICmark) continue;		
		t2ICmark += instPerT2Tick;
		
		// From this point on, this code path is hit 8192 times per second
		
		if (Q->IO_TIMER_RUN) 
		{
			// Handle T2 timer updating
			Q->t2--;
			if (Q->t2==0)
			{
				TRACE1(437);
	
				if (Q->ioram[0x2f]&4) { Q->ioram[0x2f]|=8; Q->WAKEUP=1; }
				if (Q->ioram[0x2f]&2) { Q->ioram[0x2f]|=8; if (!Q->INTERRUPT_PROCESSING) INTERRUPT("T2") }
			}
		}	

		if (flipflop++&1)
		{
			// Draw video scan line every other interval
			if (Q->lcdRedrawBuffer[Q->lcdCurrentLine])
			{
				TRACE2(436,Q->lcdCurrentLine);
		
				if (Q->statusDisplayMode2x2)
				{
					if (Q->statusPortraitViaLandscape)
						DrawDisplayLine16x2_arm_PVL(Q);
					else
						DrawDisplayLine16x2_arm(Q);
				}
				else
				{
					DrawDisplayLine16x1_arm(Q);
				}
			}
			Q->lcdCurrentLine++;
			if (Q->lcdCurrentLine>=0x40) { Q->lcdCurrentLine=0; }
		}
		
		
		currentTick = TimGetTicks_arm(Q);
				
		// recompute number of instruction per T2 tick
		if (slowDown)
		{
			if (cycleCount>=prevCycleCount)
			{
				// Q->maxCyclesPerTick cycles have elapsed... 
				// wait for system tick by setting eventTimeout = 1
				prevCycleCount=cycleCount+Q->maxCyclesPerTick;
				instPerT2Tick = ((instCount-prevInstCount)*100L)>>13;
				prevInstCount = instCount;
				eventTimeout=1;
				pollKeyboard=1;
			}	
		}
		else
		{
			if (currentTick>=tickMark)
			{
				// one tick of the system timer has elapsed
				instPerT2Tick = ((instCount-prevInstCount)*100L)>>13;
				prevInstCount = instCount;
				tickMark=currentTick+1;
				eventTimeout=0;
				pollKeyboard=1;
			}
		}
		



		//Timer control section
		if (Q->IO_TIMER_RUN) 
		{
			// Handle T1 timer updating
			if ((currentTick-Q->prevTickStamp)>=6)
			{
				Q->prevTickStamp=currentTick;
				if (Q->t1)
					Q->t1--;
				else
				{
					TRACE1(438);
	
					Q->t1 = 0xf;
					if (Q->ioram[0x2e]&4) { Q->ioram[0x2e]|=8; Q->WAKEUP=1; }
					if (Q->ioram[0x2e]&2) { Q->ioram[0x2e]|=8; if (!Q->INTERRUPT_PROCESSING) INTERRUPT("T1") }
				}
			}
		}




		// Keyboard polling section
		if (pollKeyboard)
		{
			pollKeyboard=0;
			
			if (Q->statusColorCycle)
			{	
				if (!--colorCycleCount)
				{
					cycleColor_arm(Q);
					colorCycleCount=10;
					for (d=0;d<64;d++) Q->lcdRedrawBuffer[d] = 1;  // signal complete redraw
					Q->lcdPreviousAnnunciatorState=0;
					change_ann(Q);				
				}
			}
			
			tickMark = TimGetTicks_arm(Q)+1;
						
			if (Q->ioram[0x10]&8)   // clear serial buffer byte.
			{
				if (Q->ioram[0x12]&1) 
				{
					// Dispose of byte in buffer at 0x16, 0x17
					Q->ioram[0x12] = 0;
					if (Q->ioram[0x10]&4) INTERRUPT("IO")
				}
			}
		    
			if (Q->delayEvent)
			{
				Q->delayEvent--;
				if (Q->delayEvent==0)
				{
					//perform event;
					if (Q->delayKey==virtKeyVideoSwap)
					{
						toggle_lcd_arm(Q);
					}
					else
					{
						kbd_handler_arm(noKey,KEY_RELEASE_ALL,Q);
						if (Q->delayEvent2)
						{
							kbd_handler_arm(Q->delayKey2,KEY_PRESS_SINGLE,Q);
							if (Q->INTERRUPT_ENABLED) { if (!Q->INTERRUPT_PROCESSING) INTERRUPT("KP") else { Q->INTERRUPT_PENDING=1; } }
							Q->delayEvent=Q->delayEvent2;
							Q->delayKey=Q->delayKey2;
							Q->delayKey2=noKey;
							Q->delayEvent2=0;
						}
					}
				}
			}
			
			oldContrast = Q->lcdContrastLevel;
			
			
						
			TRACE1(439);
			
			Q->instCount=instCount;
			Q->cycleCount=cycleCount;
			if ((ProcessPalmEvent_arm(eventTimeout,Q)) && (!Q->IR15X))
			{
				Q->WAKEUP=1;
				if (Q->INTERRUPT_ENABLED)
				{
					if (!Q->INTERRUPT_PROCESSING)
						INTERRUPT("KP")
					else
						Q->INTERRUPT_PENDING=1;
				}
			}
			if (Q->IR15X) 
			{
				Q->WAKEUP=1;
				if (!Q->INTERRUPT_PROCESSING) INTERRUPT("ON")
			}
						
			TRACE1(440);
			
			
			
			
			if (Q->optionTrueSpeed)
			{
				slowDown = true;
				prevCycleCount=cycleCount+Q->maxCyclesPerTick;
			}
			else
			{
				keyPressed=0;
				for (index=0;index<9;index++)
					if (Q->kbd_row[index]!=0)
					{
						keyPressed=1;
						break;
					}
				if (keyPressed) 
				{
					slowDown=true;
					Q->maxCyclesPerTick = 21000;
					prevCycleCount=cycleCount+Q->maxCyclesPerTick;
				}
				else
				{
					slowDown=false;
					Q->maxCyclesPerTick = 0;
				}
			}
			
			if (Q->statusPerformReset)
			{
				Q->pc=0;
				I=GetRealAddressNoIO_ARM(Q->pc,Q);
				Q->statusPerformReset=false;
			}
			if (oldContrast!=Q->lcdContrastLevel)
				adjust_contrast_arm(Q);
			if (Q->statusQuit) break;
	    }
	}
	
	// Normally we only make it to this point by the statusQuit test.
	SwapAllData_arm(Q);
	return 1;
}

















/*************************************************************************\
*
*  FUNCTION:  RecalculateDisplayParameters()
*
\*************************************************************************/
void RecalculateDisplayParameters_arm(register emulState *Q) 
{
	Int16 nibblesPerLine;
	Int16 skipOneNibble;
	Int16 i,j;
	UInt8* realAddress;
	Int16 numLines;	
	
	
	Q->lcdTouched = 0;
	
	// Calculate grob end addresses
	if (Q->lcdPixelOffset&4)
	{
		nibblesPerLine = 36;
		skipOneNibble = 1;
	}
	else
	{
		nibblesPerLine = 34;
		skipOneNibble = 0;
	}
	
	if (Q->lcdLineCount==0)
		numLines=64;
	else
		numLines=Q->lcdLineCount+1;
		
	Q->lcdDisplayAreaWidth = (Q->lcdByteOffset+nibblesPerLine)&0xfffe;
	Q->lcdDisplayAddressEnd = Q->lcdDisplayAddressStart + (numLines*Q->lcdDisplayAreaWidth);
	if (Q->lcdDisplayAddressEnd < Q->lcdDisplayAddressStart)
	{
		Q->lcdDisplayAddressStart2 = Q->lcdDisplayAddressEnd - Q->lcdDisplayAreaWidth;
		Q->lcdDisplayAddressEnd = Q->lcdDisplayAddressStart - Q->lcdDisplayAreaWidth;
	}
	else
	{
		Q->lcdDisplayAddressStart2 = Q->lcdDisplayAddressStart;
	}
	Q->lcdMenuAddressEnd = Q->lcdMenuAddressStart + (64-numLines)*34;
	
	// Precalculate line start addresses
	realAddress = GetRealAddressNoIO_ARM(Q->lcdDisplayAddressStart,Q);
	for (i=0;i<numLines;i++)
	{
		Q->lcdLineAddress[i] = realAddress + (Q->lcdDisplayAreaWidth*i) + skipOneNibble;
		Q->lcdRedrawBuffer[i] = 1;  // mark line for redraw
	}
	if (i<64)
	{
		realAddress = GetRealAddressNoIO_ARM(Q->lcdMenuAddressStart,Q);
		for (j=i;j<64;j++)
		{
			Q->lcdLineAddress[j] = realAddress + (34*(j-i));
			Q->lcdRedrawBuffer[j] = 1;  // mark line for redraw
		}
	}
	
	TRACE1(454);
}




/*************************************************************************\
*
*  FUNCTION:  DrawDisplayLine16x2_arm()
*
*  This function draws one line of the display at 16 bit color depth and
*  2x2 pixel size.
*
\*************************************************************************/
void DrawDisplayLine16x2_arm(register emulState *Q) 
{
	UInt32 	*lcd1MemPtr;			// (UInt32) = copy 32 bits at a time
	UInt32 	*lcd2MemPtr;			// (UInt32) = copy 32 bits at a time
	UInt32  offset;
	UInt32  switchValue;
	UInt8	*valPtr;
	UInt32 	count,count2;
	UInt32	*background1ImagePtr;
	UInt32	*background2ImagePtr;
	UInt32	pixelSetValue;
	UInt32	backSetValue;
	Boolean	*pixelMap;
	Boolean *localPixelMap;
	Boolean	useBackdrop;
	
	if (!Q->lcdOn) return;  
	
	
	useBackdrop = Q->statusBackdrop;
    offset = (Q->rowInt16s*Q->lcdCurrentLine);
    lcd1MemPtr = Q->lcd2x2MemPtrBase + offset;
	lcd2MemPtr = lcd1MemPtr + (Q->rowInt16s>>1);
	valPtr = (UInt8*)Q->lcdLineAddress[Q->lcdCurrentLine];
	switchValue = Q->lcdPixelOffset&3;
	//localPixelMap=Q->pixelMap;
	localPixelMap=(Boolean*)pixelMapConst;
	if (Q->statusColorCycle)
	{
		pixelSetValue=Q->colorPixelVal;
		backSetValue=Q->colorBackVal;
	}
	else
	{
		pixelSetValue=Q->contrastPixelVal;
		backSetValue=Q->contrastBackVal;
	}
	
	
    offset = (262*Q->lcdCurrentLine);
	background1ImagePtr = Q->backgroundImagePtr2x2+1834+offset;
	background2ImagePtr = background1ImagePtr+131;

	switch (switchValue) 
	{
		case 0:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
						background1ImagePtr++;background2ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 1:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap++;  //skip forward one pixel
				count2=3;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
						background1ImagePtr++;background2ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 2:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap+=2;   //skip forward two pixels
				count2=2;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
						background1ImagePtr++;background2ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 3:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap+=3;   //skip forward three pixels
				if (*pixelMap++) { 
					*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
					background1ImagePtr++;background2ImagePtr++;
				} else if (useBackdrop) {
					*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
				} else {
					*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
				}
				break;
	}
	

	count=31;
	do {
		pixelMap = &localPixelMap[(*valPtr++)<<2];
		count2=4;
		do {
			if (*pixelMap++) { 
				*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
				background1ImagePtr++;background2ImagePtr++;
			} else if (useBackdrop) {
				*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
			} else {
				*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
			}
		} while (--count2);
	} while (--count);



	switch (switchValue) 
	{
		case 0: pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=3;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
						background1ImagePtr++;background2ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 1:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
						background1ImagePtr++;background2ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 2:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
						background1ImagePtr++;background2ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
					}
				} while (--count2);

				pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 33
				if (*pixelMap++) { 
					*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
					background1ImagePtr++;background2ImagePtr++;
				} else if (useBackdrop) {
					*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
				} else {
					*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
				}
				break;
		case 3:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
						background1ImagePtr++;background2ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
					}
				} while (--count2);

				pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 33
				count2=2;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;*lcd2MemPtr++ = pixelSetValue;
						background1ImagePtr++;background2ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;*lcd2MemPtr++ = *background2ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;*lcd2MemPtr++ = backSetValue;
					}
				} while (--count2);
	}
	
	Q->lcdRedrawBuffer[Q->lcdCurrentLine] = 0;
}



/*************************************************************************\
*
*  FUNCTION:  DrawDisplayLine16x2_arm_PVL()
*
*  This function draws one line of the display at 16 bit color depth and
*  2x2 pixel size, using the Portrait via Landscape orientation
*
\*************************************************************************/
void DrawDisplayLine16x2_arm_PVL(register emulState *Q) 
{
	UInt32 	*lcd1MemPtr;			// (UInt32) = copy 32 bits at a time
	UInt32  lcdnewline,offset;
	UInt32  switchValue;
	UInt8	*valPtr;
	UInt32 	count,count2;
	UInt16	*background1ImagePtr;
	UInt16	*background2ImagePtr;
	UInt32	pixelSetValue;
	UInt32	backSetValue;
	Boolean	*pixelMap;
	Boolean *localPixelMap;
	Boolean	useBackdrop;
	
	if (!Q->lcdOn) return;  
	
	
	useBackdrop = Q->statusBackdrop;
    lcdnewline=Q->rowInt16s>>1;
    lcd1MemPtr = Q->lcd2x2MemPtrBasePVL + Q->lcdCurrentLine;
	valPtr = (UInt8*)Q->lcdLineAddress[Q->lcdCurrentLine];
	switchValue = Q->lcdPixelOffset&3;
	//localPixelMap=Q->pixelMap;
	localPixelMap=(Boolean*)pixelMapConst;
	if (Q->statusColorCycle)
	{
		pixelSetValue=Q->colorPixelVal;
		backSetValue=Q->colorBackVal;
	}
	else
	{
		pixelSetValue=Q->contrastPixelVal;
		backSetValue=Q->contrastBackVal;
	}
	
    offset = (262*Q->lcdCurrentLine);
	background1ImagePtr = (UInt16*)Q->backgroundImagePtr2x2;
	background1ImagePtr += 3406+(2*offset);
	background2ImagePtr = background1ImagePtr+262;

	switch (switchValue) 
	{
		case 0:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						background1ImagePtr+=2;
						background2ImagePtr+=2;
					} else if (useBackdrop) {
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
					} else {
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
					}
				} while (--count2);
				break;
		case 1:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap++;  //skip forward one pixel
				count2=3;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						background1ImagePtr+=2;
						background2ImagePtr+=2;
					} else if (useBackdrop) {
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
					} else {
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
					}
				} while (--count2);
				break;
		case 2:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap+=2;   //skip forward two pixels
				count2=2;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						background1ImagePtr+=2;
						background2ImagePtr+=2;
					} else if (useBackdrop) {
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
					} else {
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
					}
				} while (--count2);
				break;
		case 3:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap+=3;   //skip forward three pixels
				if (*pixelMap++) { 
					*lcd1MemPtr = pixelSetValue;
					lcd1MemPtr-=lcdnewline;
					*lcd1MemPtr = pixelSetValue;
					lcd1MemPtr-=lcdnewline;
					background1ImagePtr+=2;
					background2ImagePtr+=2;
				} else if (useBackdrop) {
					*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
					lcd1MemPtr-=lcdnewline;
					*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
					lcd1MemPtr-=lcdnewline;
				} else {
					*lcd1MemPtr = backSetValue;
					lcd1MemPtr-=lcdnewline;
					*lcd1MemPtr = backSetValue;
					lcd1MemPtr-=lcdnewline;
				}
				break;
	}
	

	count=31;
	do {
		pixelMap = &localPixelMap[(*valPtr++)<<2];
		count2=4;
		do {
			if (*pixelMap++) { 
				*lcd1MemPtr = pixelSetValue;
				lcd1MemPtr-=lcdnewline;
				*lcd1MemPtr = pixelSetValue;
				lcd1MemPtr-=lcdnewline;
				background1ImagePtr+=2;
				background2ImagePtr+=2;
			} else if (useBackdrop) {
				*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
				lcd1MemPtr-=lcdnewline;
				*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
				lcd1MemPtr-=lcdnewline;
			} else {
				*lcd1MemPtr = backSetValue;
				lcd1MemPtr-=lcdnewline;
				*lcd1MemPtr = backSetValue;
				lcd1MemPtr-=lcdnewline;
			}
		} while (--count2);
	} while (--count);


	switch (switchValue) 
	{
		case 0: pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=3;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						background1ImagePtr+=2;
						background2ImagePtr+=2;
					} else if (useBackdrop) {
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
					} else {
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
					}
				} while (--count2);
				break;
		case 1:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						background1ImagePtr+=2;
						background2ImagePtr+=2;
					} else if (useBackdrop) {
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
					} else {
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
					}
				} while (--count2);
				break;
		case 2:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						background1ImagePtr+=2;
						background2ImagePtr+=2;
					} else if (useBackdrop) {
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
					} else {
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
					}
				} while (--count2);

				pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 33
				if (*pixelMap++) { 
					*lcd1MemPtr = pixelSetValue;
					lcd1MemPtr-=lcdnewline;
					*lcd1MemPtr = pixelSetValue;
					lcd1MemPtr-=lcdnewline;
					background1ImagePtr+=2;
					background2ImagePtr+=2;
				} else if (useBackdrop) {
					*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
					lcd1MemPtr-=lcdnewline;
					*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
					lcd1MemPtr-=lcdnewline;
				} else {
					*lcd1MemPtr = backSetValue;
					lcd1MemPtr-=lcdnewline;
					*lcd1MemPtr = backSetValue;
					lcd1MemPtr-=lcdnewline;
				}
				break;
		case 3:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						background1ImagePtr+=2;
						background2ImagePtr+=2;
					} else if (useBackdrop) {
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
					} else {
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
					}
				} while (--count2);

				pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 33
				count2=2;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = pixelSetValue;
						lcd1MemPtr-=lcdnewline;
						background1ImagePtr+=2;
						background2ImagePtr+=2;
					} else if (useBackdrop) {
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = ((*background2ImagePtr++)<<16)|(*background1ImagePtr++);
						lcd1MemPtr-=lcdnewline;
					} else {
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
						*lcd1MemPtr = backSetValue;
						lcd1MemPtr-=lcdnewline;
					}
				} while (--count2);
	}

	Q->lcdRedrawBuffer[Q->lcdCurrentLine] = 0;
}









void DrawDisplayLine16x1_arm(register emulState *Q) 
{
	UInt16 	*lcd1MemPtr;
	UInt16	*labelMemPtr;
	UInt16  offset,offset2;
	UInt32  switchValue;
	UInt8  	*valPtr;
	UInt16	white=0xffff;
	UInt32	count,count2;
	UInt16	*background1ImagePtr;
	UInt16	pixelSetValue;
	UInt16	backSetValue;
	Boolean	*pixelMap;
	Boolean *localPixelMap;
	Boolean	useBackdrop;
		
	if (!Q->lcdOn) return;  
	
	
	useBackdrop = Q->statusBackdrop;
	offset = (Q->rowInt16s*Q->lcdCurrentLine); 
    lcd1MemPtr = Q->lcd1x1MemPtrBase + offset;
	valPtr = (UInt8*)Q->lcdLineAddress[Q->lcdCurrentLine];
	switchValue = Q->lcdPixelOffset&3;
	//localPixelMap=Q->pixelMap;
	localPixelMap=(Boolean*)pixelMapConst;
	if (Q->statusColorCycle)
	{
		pixelSetValue=(UInt16)(Q->colorPixelVal&0x0000FFFF);
		backSetValue=(UInt16)(Q->colorBackVal&0x0000FFFF);
	}
	else
	{
		pixelSetValue=(UInt16)(Q->contrastPixelVal&0x0000FFFF);
		backSetValue=(UInt16)(Q->contrastBackVal&0x0000FFFF);
	}
	
    offset = (148*Q->lcdCurrentLine);
	background1ImagePtr = Q->backgroundImagePtr1x1+16+offset;

		
	switch (switchValue) 
	{
		case 0:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 1:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap++;  //skip forward one pixel
				count2=3;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 2:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap+=2;   //skip forward two pixels
				count2=2;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 3:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 0
				pixelMap+=3;   //skip forward three pixels
				if (*pixelMap++) { 
					*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
				} else if (useBackdrop) {
					*lcd1MemPtr++ = *background1ImagePtr++;
				} else {
					*lcd1MemPtr++ = backSetValue;
				}
				break;
	}
	
	
	count=31;
	do {
		pixelMap = &localPixelMap[(*valPtr++)<<2];
		count2=4;
		do {
			if (*pixelMap++) { 
				*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
			} else if (useBackdrop) {
				*lcd1MemPtr++ = *background1ImagePtr++;
			} else {
				*lcd1MemPtr++ = backSetValue;
			}
		} while (--count2);
	} while (--count);

	
	switch (switchValue) 
	{
		case 0: pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=3;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 1:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;
					}
				} while (--count2);
				break;
		case 2:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;
					}
				} while (--count2);

				pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 33
				if (*pixelMap++) { 
					*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
				} else if (useBackdrop) {
					*lcd1MemPtr++ = *background1ImagePtr++;
				} else {
					*lcd1MemPtr++ = backSetValue;
				}
				break;
		case 3:	pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 32
				count2=4;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;
					}
				} while (--count2);

				pixelMap = &localPixelMap[(*valPtr++)<<2];  // nibble 33
				count2=2;
				do {
					if (*pixelMap++) { 
						*lcd1MemPtr++ = pixelSetValue;background1ImagePtr++;
					} else if (useBackdrop) {
						*lcd1MemPtr++ = *background1ImagePtr++;
					} else {
						*lcd1MemPtr++ = backSetValue;
					}
				} while (--count2);
	}

	if (Q->lcdCurrentLine>=softLabelStart)   //  are we in the menu area
	{
		if (Q->lcdLineCount==55)  // Is menu active?
		{
			offset2 = (Q->rowInt16s*(Q->lcdCurrentLine-softLabelStart));
			lcd1MemPtr = Q->softKeyMemPtrBase + offset2;
			if (Q->status49GActive) lcd1MemPtr -= (Q->rowInt16s*3);  //  back up three line for 49G skin
			labelMemPtr = Q->labelMemPtrBase + offset2;
			
			for (count=0;count<6;count++)
			{
				lcd1MemPtr+= Q->labelOffset[count];
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				*lcd1MemPtr++ = *labelMemPtr++;
				labelMemPtr++;
			}
		}
		else
		{
			lcd1MemPtr = Q->softKeyMemPtrBase + (Q->rowInt16s*(Q->lcdCurrentLine-softLabelStart));
			if (Q->status49GActive) lcd1MemPtr -= (Q->rowInt16s*3);  //  back up three line for 49G skin
			
			for (count=0;count<6;count++)
			{
				lcd1MemPtr+= Q->labelOffset[count];
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
				*lcd1MemPtr++ = white;
			}
		}
	}
	
	Q->lcdRedrawBuffer[Q->lcdCurrentLine] = 0;
}



void draw_annunciator_arm(UInt16* lcdMemPtrBase, UInt16 *annunciatorMemPtr, UInt16 *backgroundMemPtr, UInt32 draw, register emulState *Q)
{
	UInt32 i,count;
	UInt16	pixelSetValue;
	UInt16	backSetValue;
	Boolean useBackdrop;
	UInt16 *lcdMemPtr;
	
	useBackdrop = Q->statusBackdrop;
	if (Q->statusColorCycle)
	{
		pixelSetValue=(UInt16)(Q->colorPixelVal&0x0000FFFF);
		backSetValue=(UInt16)(Q->colorBackVal&0x0000FFFF);
	}
	else
	{
		pixelSetValue=(UInt16)(Q->contrastPixelVal&0x0000FFFF);
		backSetValue=(UInt16)(Q->contrastBackVal&0x0000FFFF);
	}

	if (Q->statusPortraitViaLandscape)
	{
		for (i=0;i<annHeight;i++)
		{
			lcdMemPtr=lcdMemPtrBase+i;
			count=10;
			do
			{
				if ((*annunciatorMemPtr++ == 0xFFFF) || (draw==0))
				{
					if (useBackdrop)
					{
						*lcdMemPtr = *backgroundMemPtr++;
						lcdMemPtr-=Q->rowInt16s;
					}
					else
					{
						*lcdMemPtr = backSetValue;
						lcdMemPtr-=Q->rowInt16s;
					}
				} 
				else 
				{
					*lcdMemPtr = pixelSetValue;
					lcdMemPtr-=Q->rowInt16s;
					backgroundMemPtr++;
				}
			} while (--count);
					
			annunciatorMemPtr+=annunciatorBitmapWidth-10;   // skip to next line in offscreen bitmap;
			backgroundMemPtr += ((Q->statusDisplayMode2x2)?252:138);
		}
	}
	else
	{
		lcdMemPtr=lcdMemPtrBase;
		for (i=0;i<annHeight;i++)
		{
			count=10;
			do
			{
				if ((*annunciatorMemPtr++ == 0xFFFF) || (draw==0))
				{
					if (useBackdrop)
						*lcdMemPtr++ = *backgroundMemPtr++;
					else
						*lcdMemPtr++ = backSetValue;
				} 
				else 
				{
					*lcdMemPtr++ = pixelSetValue;
					backgroundMemPtr++;
				}
			} while (--count);
			lcdMemPtr+=(Q->rowInt16s-10);   // skip to the next line: Q->rowInt16s - number of ptr copy ops
		
			annunciatorMemPtr+=annunciatorBitmapWidth-10;   // skip to next line in offscreen bitmap;
			backgroundMemPtr += ((Q->statusDisplayMode2x2)?252:138);
		}
	}
}


#define annSLBGoffset2x2		ann2x2ShiftLeftPositionX+262
#define annSRBGoffset2x2		ann2x2ShiftRightPositionX+262
#define annAlphaBGoffset2x2		ann2x2AlphaPositionX+262
#define annAlarmBGoffset2x2		ann2x2AlarmPositionX+262
#define annBusyBGoffset2x2		ann2x2BusyPositionX+262
#define annIOBGoffset2x2		ann2x2IOPositionX+262

#define annSLBGoffset1x1		ann1x1ShiftLeftPositionY*148+1
#define annSRBGoffset1x1		ann1x1ShiftRightPositionY*148+1
#define annAlphaBGoffset1x1		ann1x1AlphaPositionY*148+1
#define annAlarmBGoffset1x1		ann1x1AlarmPositionY*148+1
#define annBusyBGoffset1x1		ann1x1BusyPositionY*148+1
#define annIOBGoffset1x1		ann1x1IOPositionY*148+1


/*************************************************************************\
*
*  FUNCTION:  change_ann()
*
\*************************************************************************/
void change_ann(register emulState *Q) 
{
	UInt16 c;
	UInt16 *lcdMemPtr;
	UInt16 *annunciatorMemPtr;
	UInt16 *backgroundMemPtr;	
	
		
	c = Q->lcdAnnunciatorState;
	if ((!(c&AON)) || (!Q->IO_TIMER_RUN)) c=0;
	if ((c&LA1) != (Q->lcdPreviousAnnunciatorState&LA1))
	{
		if (Q->statusDisplayMode2x2)
		{
			if (Q->statusPortraitViaLandscape)
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBasePVL - (ann2x2ShiftLeftPositionX*Q->rowInt16s);
			else
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBase + ann2x2ShiftLeftPositionX;
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr2x2 + annSLBGoffset2x2;
		}
		else
		{
			lcdMemPtr = (UInt16*)Q->lcd1x1AnnAreaPtrBase + (ann1x1ShiftLeftPositionY*Q->rowInt16s); 
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr1x1 + annSLBGoffset1x1;
		}
			
		annunciatorMemPtr = Q->annunciatorMemPtrBase + annShiftLeftOffset;

		draw_annunciator_arm(lcdMemPtr,annunciatorMemPtr,backgroundMemPtr,c&LA1,Q);
	}
	if ((c&LA2) != (Q->lcdPreviousAnnunciatorState&LA2))
	{
		if (Q->statusDisplayMode2x2)
		{
			if (Q->statusPortraitViaLandscape)
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBasePVL - (ann2x2ShiftRightPositionX*Q->rowInt16s);
			else
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBase + ann2x2ShiftRightPositionX;
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr2x2 + annSRBGoffset2x2;
		}
		else
		{
			lcdMemPtr = (UInt16*)Q->lcd1x1AnnAreaPtrBase + (ann1x1ShiftRightPositionY*Q->rowInt16s);
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr1x1 + annSRBGoffset1x1;
		}
			
		annunciatorMemPtr = Q->annunciatorMemPtrBase + annShiftRightOffset;
		
		draw_annunciator_arm(lcdMemPtr,annunciatorMemPtr,backgroundMemPtr,c&LA2,Q);
	}
	if ((c&LA3) != (Q->lcdPreviousAnnunciatorState&LA3))
	{
		if (Q->statusDisplayMode2x2)
		{
			if (Q->statusPortraitViaLandscape)
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBasePVL - (ann2x2AlphaPositionX*Q->rowInt16s);
			else
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBase + ann2x2AlphaPositionX;
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr2x2 + annAlphaBGoffset2x2;
		}
		else
		{
			lcdMemPtr = (UInt16*)Q->lcd1x1AnnAreaPtrBase + (ann1x1AlphaPositionY*Q->rowInt16s);
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr1x1 + annAlphaBGoffset1x1;
		}

		annunciatorMemPtr = Q->annunciatorMemPtrBase + annAlphaOffset;
		
		draw_annunciator_arm(lcdMemPtr,annunciatorMemPtr,backgroundMemPtr,c&LA3,Q);
	}
	if ((c&LA4) != (Q->lcdPreviousAnnunciatorState&LA4))
	{
		if (Q->statusDisplayMode2x2)
		{
			if (Q->statusPortraitViaLandscape)
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBasePVL - (ann2x2AlarmPositionX*Q->rowInt16s);
			else
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBase + ann2x2AlarmPositionX;
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr2x2 + annAlarmBGoffset2x2;
		}
		else
		{
			lcdMemPtr = (UInt16*)Q->lcd1x1AnnAreaPtrBase + (ann1x1AlarmPositionY*Q->rowInt16s); 
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr1x1 + annAlarmBGoffset1x1;
		}
			
		annunciatorMemPtr = Q->annunciatorMemPtrBase + annAlarmOffset;
		
		draw_annunciator_arm(lcdMemPtr,annunciatorMemPtr,backgroundMemPtr,c&LA4,Q);
	}
	if ((c&LA5) != (Q->lcdPreviousAnnunciatorState&LA5))
	{
		if (Q->statusDisplayMode2x2)
		{
			if (Q->statusPortraitViaLandscape)
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBasePVL - (ann2x2BusyPositionX*Q->rowInt16s);
			else
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBase + ann2x2BusyPositionX;
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr2x2 + annBusyBGoffset2x2;
		}
		else
		{
			lcdMemPtr = (UInt16*)Q->lcd1x1AnnAreaPtrBase + (ann1x1BusyPositionY*Q->rowInt16s);
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr1x1 + annBusyBGoffset1x1;
		}
			
		annunciatorMemPtr = Q->annunciatorMemPtrBase + annBusyOffset;
		
		draw_annunciator_arm(lcdMemPtr,annunciatorMemPtr,backgroundMemPtr,c&LA5,Q);
	}
	if ((c&LA6) != (Q->lcdPreviousAnnunciatorState&LA6))
	{
		if (Q->statusDisplayMode2x2)
		{
			if (Q->statusPortraitViaLandscape)
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBasePVL - (ann2x2IOPositionX*Q->rowInt16s);
			else
				lcdMemPtr = (UInt16*)Q->lcd2x2AnnAreaPtrBase + ann2x2IOPositionX;
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr2x2 + annIOBGoffset2x2;
		}
		else
		{
			lcdMemPtr = (UInt16*)Q->lcd1x1AnnAreaPtrBase + (ann1x1IOPositionY*Q->rowInt16s); 
			backgroundMemPtr = (UInt16*)Q->backgroundImagePtr1x1 + annIOBGoffset1x1;
		}
			
		annunciatorMemPtr = Q->annunciatorMemPtrBase + annIOOffset;
		
		draw_annunciator_arm(lcdMemPtr,annunciatorMemPtr,backgroundMemPtr,c&LA6,Q);
	}
	Q->lcdPreviousAnnunciatorState = c;
}










inline void IOBit(UInt32 d, UInt8 b, Boolean s, register emulState *Q)	// set/clear bit in I/O section
{
	if (s)
		Q->ioram[d] |= b;			// set bit
	else
		Q->ioram[d] &= ~b;			// clear bit
}



UInt32 read_io_multiple_arm(UInt32 d, UInt8 *a, UInt32 s, register emulState *Q)
{
	UInt8 c;
	UInt32 GENIOINTERRUPT = 0;	

	do
	{
		switch (d) 
		{
			case 0x04: c = (UInt8)(Q->crc); break;
			case 0x05: c = (UInt8)(Q->crc>>4); break;
			case 0x06: c = (UInt8)(Q->crc>>8); break;
			case 0x07: c = (UInt8)(Q->crc>>12); break;
			case 0x0F: c = Q->CARDSTATUS; break;
			case 0x15: Q->ioram[0x11]&=0xe; c=Q->ioram[d]; break;
			case 0x16: /**/
			case 0x17: /**/ c=3;break;
			case 0x18: /**/
			case 0x19: /* ScreenPrint("Nibbles #118/#119 read.",true); */ c=3;break;
			case 0x1B: 
			case 0x1C:                // LED CONTROL
			case 0x1D: c = 0; break;  // LED BUFFER
			case 0x20: /**/
			case 0x21: /**/
			case 0x22: /**/
			case 0x23: /**/
			case 0x24: /**/
			case 0x25: /**/
			case 0x26: /**/
			case 0x27: /**/ c=3;break;
			case 0x28: c = (UInt8)Q->lcdCurrentLine; break;
			case 0x29: c = (UInt8)(Q->lcdCurrentLine>>4); break;
			case 0x30: /**/
			case 0x31: /**/
			case 0x32: /**/
			case 0x33: /**/
			case 0x34: /**/ c = 3; break;      
			case 0x37: c = (UInt8)(Q->t1); break;
			case 0x38: c = (UInt8)(Q->t2); break;
			case 0x39: c = (UInt8)(Q->t2>>4); break;
			case 0x3A: c = (UInt8)(Q->t2>>8); break;
			case 0x3B: c = (UInt8)(Q->t2>>12); break;
			case 0x3C: c = (UInt8)(Q->t2>>16); break;
			case 0x3D: c = (UInt8)(Q->t2>>20); break;
			case 0x3E: c = (UInt8)(Q->t2>>24); break;
			case 0x3F: c = (UInt8)(Q->t2>>28); CRC_update(c,Q); break;
			default: c=Q->ioram[d];
		}
		*a++ = (c&0xf);
		d++;
	} while (--s);
	return GENIOINTERRUPT;
}



UInt32 write_io_multiple_arm(UInt32 d, UInt8 *a, UInt32 s, register emulState *Q) 
{
	UInt8 c;
	UInt16 i;
	Int16 tempInt;
	UInt32 tempLong;
	UInt32 contrastTouched    = 0;
	UInt32 annunciatorTouched = 0;
	UInt32 dispAddressTouched = 0;
	UInt32 lineOffsetTouched  = 0;
	UInt32 lineCountTouched   = 0;
	UInt32 menuAddressTouched = 0;
	UInt32 GENIOINTERRUPT = 0;	
	
	do
	{
		c=*a++;
		switch (d) 
		{
// 00100 =  NS:DISPIO
// 00100 @  Display bit offset and DON [DON OFF2 OFF1 OFF0]
// 00100 @  3 nibs for display offset (scrolling), DON=Display ON
			case 0x00:
				if (Q->lcdOn!=(c>>3)) 
				{ 
					Q->lcdOn = c>>3;

					TRACE1(426);
				
					if (!Q->lcdOn) 
					{
						display_clear_arm(Q);
					}
					Q->lcdTouched = 1;  
				}
				if (Q->lcdPixelOffset!=(c&7)) // OFF2, OFF1, or OFF0 changed
				{ 
					Q->lcdTouched = 1; 
					Q->lcdPixelOffset = c&7; 
				}
				break;
				
// 00101 =  NS:CONTRLSB
// 00101 @  Contrast Control [CON3 CON2 CON1 CON0]
// 00101 @  Higher value = darker screen

// 00102 =  NS:DISPTEST
// 00102 @  Display test [VDIG LID TRIM CON4] [LRT LRTD LRTC BIN]
// 00102 @  Normally zeros
			case 0x01:
			case 0x02: contrastTouched=true; break;
			case 0x03: break;
			
// 00104 =  HP:CRC
// 00104 @  16 bit hardware CRC (104-107) (X^16+X^12+X^5+1)
// 00104 @  crc = ( crc >> 4 ) ^ ( ( ( crc ^ nib ) & 0x000F ) * 0x1081 );
	 		case 0x04: Q->crc=Q->crc&0xfff0;Q->crc|=(c); break;		// CRC
	  		case 0x05: Q->crc=Q->crc&0xff0f;Q->crc|=(c<<4); break;	// CRC
	 		case 0x06: Q->crc=Q->crc&0xf0ff;Q->crc|=(c<<8); break;	// CRC
	  		case 0x07: Q->crc=Q->crc&0x0fff;Q->crc|=(c<<12); break;	// CRC

// 00108 =  NS:POWERSTATUS
// 00108 @  Low power registers (108-109)
// 00108 @  [LB2 LB1 LB0 VLBI] (read only)
// 00108 @  LowBat(2) LowBat(1) LowBat(S) VeryLowBat
			case 0x08: break;  // read-only
			
// 00109 =  NS:POWERCTRL
// 00109 @  [ELBI EVLBI GRST RST] (read/write)
			case 0x09: break; 

// 0010A =  NS:MODE
// 0010A @  Mode Register (read-only)
			case 0x0A: break;
			
// 0010B =  HP:ANNCTRL
// 0010B @  Annunciator control [LA4 LA3 LA2 LA1] = [ alarm alpha -> <- ]
			case 0x0B:
			case 0x0C: 
				if (c!=Q->ioram[d])
				{
					annunciatorTouched=1; 
				}
				break;
			
// 0010D =  NS:BAUD
// 0010D @  Serial baud rate [UCK BD2 BD1 BD0] (bit 3 is read-only)
// 0010D @  3 bits = {1200 1920 2400 3840 4800 7680 9600 15360}
			case 0x0D: c = (Q->ioram[d]&8)|(c&7);  // bit 3 is read-only
			// Call procedure to set baud rate
			break;

// 0010E =  NS:CARDCTL
// 0010E @  [ECDT RCDT SMP SWINT] (read/write)
// 0010E @  Enable Card Det., Run Card Det., Set Module Pulled, Software interrupt
	  		case 0x0E:  									// NS:CARDCTRL
	    		if (c&1) GENIOINTERRUPT = 1;
	    		if (c&2) { Q->ioram[0x19] = 2; Q->HST|=MP; GENIOINTERRUPT = 1; }
	    		if (c&8)
	      			if (c&4) Q->ioram[0x0F] = 0;
	    		break;
	    		
// 0010F =  NS:CARDSTATUS
// 0010F @  [P2W P1W P2C P1C] (read-only) Port 2 writable .. Port 1 inserted
	  		case 0x0F: break;  // read-only

// 00110 =  HP:IOC
// 00110 @  Serial I/O Control [SON ETBE ERBF ERBZ]
// 00110 @  Serial On, Interrupt On Recv.Buf.Empty, Full, Buzy
	 		case 0x10: break;  	
	 									
// 00111 =  HP:RCS
// 00111  Serial Receive Control/Status [RX RER RBZ RBF] (bit 3 is read-only)
	  		case 0x11: c = (Q->ioram[d]&8)|(c&7);break;
	  		
// 00112 =  HP:TCS
// 00112 @  Serial Transmit Control/Status [BRK LPB TBZ TBF]
	 		case 0x12: break;
	 		
// 00113 =  HP:CRER
// 00113 @  Serial Clear RER (writing anything clears RER bit)
	  		case 0x13: IOBit(RCS,RER,false,Q); break;
	  		
// 00114 =  HP:RBR
// 00114 @  Serial Receive Buffer Register (Reading clears RBF bit)
	  		case 0x14:
	  		case 0x15: break;

// 00116 =  HP:TBR
// 00116 @  Serial Transmit Buffer Register (Writing sets TBF bit)
	  		case 0x16: break;
	  		case 0x17: Q->ioram[TCS] |= 1; break;

// 00118 =  NS:SRR
// 00118 @  Service Request Register (read-only)
// 00118 @  [ISRQ TSRQ USRQ VSRQ] [KDN NINT2 NINT LSRQ]
	  		case 0x18: 
	  		case 0x19: break;

// 0011A =  HP:IRC
// 0011A @  IR Control Register [IRI EIRU EIRI IRE] (bit 3 is read-only)
// 0011A @  IR Input, Enable IR UART mode, Enable IR Interrupt, IR Event
	  		case 0x1A: c = (Q->ioram[d]&8)|(c&7);break;

// 0011B =  NS:BASENIBOFF
// 0011B @  Used as addressto get BASENIB from 11F to the 5th nibble
	  		case 0x1B: break;

// 0011C =  NS:LCR
// 0011C @  Led Control Register [LED ELBE LBZ LBF] (Setting LED is draining)
	  		case 0x1C: 
			// HP49G new mapping on LED bit change
			if ((Q->status49GActive) && ((c^Q->ioram[d])&LED))
			{
				Q->ioram[d]=c;			// save new value for mapping
				// Adjust ROM mapping???
				//StrPrintF(tmp,"LED change, val = %d",((c&0x8)>>3));
				//ErrorPrint(tmp,true);
				//SysTaskDelay(100);
			}
	  		break;

// 0011D =  NS:LBR
// 0011D @  Led Buffer Register [0 0 0 LBO] (bits 1-3 read zero)
	  		case 0x1D: c &= 1; break;

// 0011E =  NS:SCRATCHPAD
// 0011E @  Scratch pad
	  		case 0x1E: break;

// 0011F =  NS:BASENIB
// 0011F @  7 or 8 for base memory
	  		case 0x1F: break;

// 00120 =  NS:DISPADDR
// 00120 @  Display Start Address (write only)
// 00120 @  bit 0 is ignored (display must start on byte boundary)
	  		case 0x20:
	  		case 0x21:
	  		case 0x22:
	  		case 0x23:
	  		case 0x24: dispAddressTouched=1; break;

// 00125 =  NS:LINEOFFS
// 00125 @  Display Line offset (write only) (no of bytes skipped after each line)
// 00125 @  MSG sign extended
			case 0x25: 
			case 0x26:
			case 0x27: lineOffsetTouched=1; break;

// 00128 =  NS:LINECOUNT
// 00128 @  Display Line Counter and miscellaneous (28-29)
// 00128 @  [LC3 LC2 LC1 LC0] [DA19 M32 LC5 LC4]
// 00128 @  Line counter 6 bits -> max = 2^6-1 = 63 = disp height
// 00128 @  Normally has 55 -> Menu starts at display row 56
			case 0x28: 
			case 0x29: 
				lineCountTouched=1; 
				if ((c^Q->ioram[d])&DA19)	// DA19 changed
				{
					Q->ioram[d]^=DA19;		// save new DA19
					// Adjust ROM mapping???
				}
				break;

			case 0x2A:
			case 0x2B:
			case 0x2C:
			case 0x2D: break;
			

// 0012E =  NS:TIMER1CTRL
// 0012E @  TIMER1 Control [SRQ WKE INT XTRA]
			case 0x2E: c &= 0xE; break;

// 0012F =  NS:TIMER2CTRL
// 0012F @  TIMER2 Control [SRQ WKE INT RUN]
			case 0x2F: 
			Q->IO_TIMER_RUN=(c&RUN); 
			annunciatorTouched=1;
			break;

// 00130 =  NS:MENUADDR
// 00130 @  Display Secondary Start Address (write only) (30-34)
// 00130 @  Menu Display Address, no line offsets
	  		case 0x30:
			case 0x31:
			case 0x32:
			case 0x33:
			case 0x34: menuAddressTouched=1; break;
			
			case 0x35:
			case 0x36: break;
			

// 00137 =  HP:TIMER1
// 00137 @  Decremented 16 times/s
			case 0x37: 
				Q->t1 = c;
				Q->prevTickStamp = TimGetTicks_arm(Q);
				break;
					   
// 00138 =  HP:TIMER2
// 00138 @  hardware timer (38-3F), decremented 8192 times/s
			case 0x38: Q->t2=(Q->t2&0xFFFFFFF0)|(UInt32)c; break;
			case 0x39: Q->t2=(Q->t2&0xFFFFFF0F)|((UInt32)c<<4); break;
			case 0x3A: Q->t2=(Q->t2&0xFFFFF0FF)|((UInt32)c<<8); break;
			case 0x3B: Q->t2=(Q->t2&0xFFFF0FFF)|((UInt32)c<<12); break;
			case 0x3C: Q->t2=(Q->t2&0xFFF0FFFF)|((UInt32)c<<16); break;
			case 0x3D: Q->t2=(Q->t2&0xFF0FFFFF)|((UInt32)c<<20); break;
			case 0x3E: Q->t2=(Q->t2&0xF0FFFFFF)|((UInt32)c<<24); break;
			case 0x3F: Q->t2=(Q->t2&0x0FFFFFFF)|((UInt32)c<<28); break;
			default: break;
		}
		Q->ioram[d] = c;
		d++;
	} while (--s);

	if (contrastTouched)
	{
		tempInt = ((Q->ioram[0x02]&0x1)<<4)|Q->ioram[0x01];
		if (tempInt != Q->lcdContrastLevel)
		{
			Q->lcdContrastLevel = tempInt;
			adjust_contrast_arm(Q);
			
			for (i=0;i<64;i++) Q->lcdRedrawBuffer[i] = 1;  // signal complete redraw
		}
		
		TRACE1(427);
	}
	
	if (annunciatorTouched)
	{
		Q->lcdAnnunciatorState = (Q->ioram[0x0c]<<4)|Q->ioram[0x0b];
		change_ann(Q);
			
		TRACE1(428);
	}
	
	if (dispAddressTouched)
	{
		tempLong = ((UInt32)Q->ioram[0x24]<<16)|((UInt32)Q->ioram[0x23]<<12)|(Q->ioram[0x22]<<8)|(Q->ioram[0x21]<<4)|(Q->ioram[0x20]&0xe);
		if (tempLong != Q->lcdDisplayAddressStart)
		{
			Q->lcdDisplayAddressStart = tempLong;
			Q->lcdTouched = 1;
		}

		TRACE1(429);
	}
	
	if (lineOffsetTouched)
	{
		tempInt = (Q->ioram[0x27]<<8)|(Q->ioram[0x26]<<4)|Q->ioram[0x25];
		if (tempInt&0x800) tempInt-=0x1000;
		if (tempInt != Q->lcdByteOffset)
		{
			Q->lcdByteOffset = tempInt;
			Q->lcdTouched = 1;
		}
			
		TRACE1(430);
	}
	
	if (lineCountTouched)
	{
		tempInt =  ((Q->ioram[0x29]&0x3)<<4)|Q->ioram[0x28];
		if (tempInt != Q->lcdLineCount)
		{
			Q->lcdLineCount = tempInt;
			Q->lcdTouched = 1;
		}
			
		TRACE1(431);
	}
	
	if (menuAddressTouched)
	{
		tempLong = ((UInt32)Q->ioram[0x34]<<16)|((UInt32)Q->ioram[0x33]<<12)|(Q->ioram[0x32]<<8)|(Q->ioram[0x31]<<4)|(Q->ioram[0x30]&0xe);
		if (tempLong != Q->lcdMenuAddressStart)
		{
			Q->lcdMenuAddressStart = tempLong;
			Q->lcdTouched = 1;
		}
			
		TRACE1(432);
	}
	
	if (Q->lcdTouched)
	{
		RecalculateDisplayParameters_arm(Q);
	}
	return GENIOINTERRUPT;
}



UInt32 TimGetTicks_arm(register emulState *Q)
{
	return (Q->call68KFuncP)(
		Q->emulStateP,
		PceNativeTrapNo(sysTrapTimGetTicks),
		NULL,
		0);
}



Boolean ProcessPalmEvent_arm(UInt32 timeout, register emulState *Q)
{
	Boolean val;
	UInt32 arg;
	
	arg = ByteSwap32(timeout);
	
	
	val = (Q->call68KFuncP)(
		Q->emulStateP,
		(UInt32)Q->ProcessPalmEvent,
		&arg,
		4);
			
	if (Q->statusRecalc) 
	{
		RecalculateDisplayParameters_arm(Q);
		Q->statusRecalc=false;
	}
	if (Q->statusChangeAnn) 
	{
		change_ann(Q);
		Q->statusChangeAnn=false;
	}
		
	return val;
}



void ErrorPrint_arm( const char* screenText, Boolean newLine, register emulState *Q)
{
	UInt8 args[6];
	
	*((UInt32*)(&(args[0]))) = ByteSwap32(screenText);
	*((UInt16*)(&(args[4]))) = (UInt16)newLine;


	(Q->call68KFuncP)(
		Q->emulStateP,
		(UInt32)Q->ErrorPrint,
		&args,
		6);
	return;
}



void EmulateBeep_ARM(register emulState *Q)
{
	UInt32 addr;
	UInt8 *checkAddr;
	
	if (Q->optionTarget==HP48SX)
		addr=0x706D2;
	else if (Q->optionTarget==HP48GX)
		addr=0x80850;
	else
		addr=0x80F0F;
		
	checkAddr=GetRealAddressNoIO_ARM(addr,Q);

	checkAddr=(UInt8*)ByteSwap32(checkAddr);
	
	(Q->call68KFuncP)(
		Q->emulStateP,
		(UInt32)Q->EmulateBeep,
		&checkAddr,
		4);
	return;
}


void display_clear_arm(register emulState *Q)
{
	(Q->call68KFuncP)(
		Q->emulStateP,
		(UInt32)Q->display_clear,
		NULL,
		0);
	return;
}


void adjust_contrast_arm(register emulState *Q)
{
	Int16 			contrast;
	Int16			level;
	Int32			fr,fg,fb,br,bg,bb;

	contrast = Q->lcdContrastLevel;

	if (Q->optionTarget==HP48SX)
		contrast-=2;
	else
		contrast-=8;
	if (contrast<0) contrast=0;
	if (contrast>18) contrast=18;
	
						
	fb=(Q->colorPixelVal&0x0000001F);
	fg=(Q->colorPixelVal&0x000007E0); 
	fr=(Q->colorPixelVal&0x0000F800);
			
	bb=(Q->colorBackVal&0x0000001F);
	bg=(Q->colorBackVal&0x000007E0); 
	br=(Q->colorBackVal&0x0000F800);
				
	if (Q->optionTarget==HP48SX)
	{
		if (contrast==9)   // set to default
		{
			Q->contrastBackVal =Q->colorBackVal; 
			Q->contrastPixelVal=Q->colorPixelVal;
		}
		else if (contrast<9)   // fade pixel to background (0-8)
		{
			Q->contrastBackVal =Q->colorBackVal; 
			level = contrast*28;  // adjust to 0-8;
			
			fr = br + (((fr - br)*level)>>8);
			fg = bg + (((fg - bg)*level)>>8);
			fb = bb + (((fb - bb)*level)>>8);
			
			Q->contrastPixelVal = (fr&0x0000F800)|(fg&0x000007E0)|(fb&0x0000001F);
			Q->contrastPixelVal = (Q->contrastPixelVal<<16)|Q->contrastPixelVal;
		}
		else   // fade background to pixel (10-18)
		{
			Q->contrastPixelVal=Q->colorPixelVal;
			level = (contrast-9)*28;  // adjust to 1-9;
						
			br = br + (((fr - br)*level)>>8);
			bg = bg + (((fg - bg)*level)>>8);
			bb = bb + (((fb - bb)*level)>>8);
			
			Q->contrastBackVal = (br&0x0000F800)|(bg&0x000007E0)|(bb&0x0000001F);
			Q->contrastBackVal = (Q->contrastBackVal<<16)|Q->contrastBackVal;
		}
	}
	else //48GX or 49G
	{
		if (contrast==6)   // set to default
		{
			Q->contrastBackVal =Q->colorBackVal; 
			Q->contrastPixelVal=Q->colorPixelVal;
		}
		else if (contrast<6)   // fade pixel to background (0-5)
		{
			Q->contrastBackVal =Q->colorBackVal; 
			level = contrast*42;  // adjust to 0-5;
			
			fr = br + (((fr - br)*level)>>8);
			fg = bg + (((fg - bg)*level)>>8);
			fb = bb + (((fb - bb)*level)>>8);
			
			Q->contrastPixelVal = (fr&0x0000F800)|(fg&0x000007E0)|(fb&0x0000001F);
			Q->contrastPixelVal = (Q->contrastPixelVal<<16)|Q->contrastPixelVal;
		}
		else   // fade background to pixel (7-17)
		{
			Q->contrastPixelVal=Q->colorPixelVal;
			level = (contrast-6)*23;  // adjust to 1-11;
			
			br = br + (((fr - br)*level)>>8);
			bg = bg + (((fg - bg)*level)>>8);
			bb = bb + (((fb - bb)*level)>>8);
			
			Q->contrastBackVal = (br&0x0000F800)|(bg&0x000007E0)|(bb&0x0000001F);
			Q->contrastBackVal = (Q->contrastBackVal<<16)|Q->contrastBackVal;
		}
	}
}


void toggle_lcd_arm(register emulState *Q)
{	
	(Q->call68KFuncP)(
		Q->emulStateP,
		(UInt32)Q->toggle_lcd,
		NULL,
		0);
			
	return;
}


void SwapAllData_arm(register emulState *Q)
{
	Int32 i,num16swap,num32swap;
	UInt16 *d16swap,*d16swapend;
	UInt32 *d32swap,*d32swapend;
		
	d16swap = (UInt16*)&Q->INT16_variable_start;
	d16swapend = (UInt16*)&Q->INT16_variable_end;
	d32swap = (UInt32*)&Q->INT32_variable_start;
	d32swapend = (UInt32*)&Q->INT32_variable_end;
	
	// Pointer size better be 4 bytes for this to work
	num16swap = (((Int32)d16swapend - (Int32)d16swap)>>1)+1;
	num32swap = (((Int32)d32swapend - (Int32)d32swap)>>2)+1;

	for (i=0;i<num16swap;i++) d16swap[i]=ByteSwap16(d16swap[i]); 
	for (i=0;i<num32swap;i++) d32swap[i]=ByteSwap32(d32swap[i]);
}



void kbd_handler_arm(UInt16 keyIndex, KHType action, register emulState *Q)
{
	UInt8 args[4];
	
	*((UInt16*)(&(args[0]))) = ByteSwap16(keyIndex);
	*((UInt16*)(&(args[2]))) = (UInt16)action;


	(Q->call68KFuncP)(
		Q->emulStateP,
		(UInt32)Q->kbd_handler,
		&args,
		4);
	return;
}



void cycleColor_arm(register emulState *Q)
{
	UInt16 color; 
	
	color=((UInt16)Q->colorCycleRed<<11)|((UInt16)Q->colorCycleGreen<<5)|Q->colorCycleBlue;
	Q->colorPixelVal=((UInt32)color<<16)|color;

	switch (Q->colorCyclePhase)
	{
		case 0:
			Q->colorCycleRed=0x1F;
			Q->colorCycleGreen++;
			Q->colorCycleBlue=0;
			if (Q->colorCycleGreen==0x3F) 
				Q->colorCyclePhase=1;
			break;
		case 1:
			Q->colorCycleRed--;
			Q->colorCycleGreen=0x3F;
			Q->colorCycleBlue=0;
			if (Q->colorCycleRed==0)
				Q->colorCyclePhase=2;
			break;
		case 2:
			Q->colorCycleRed=0;
			Q->colorCycleGreen=0x3F;
			Q->colorCycleBlue++;
			if (Q->colorCycleBlue==0x1F)
				Q->colorCyclePhase=3;
			break;
		case 3:
			Q->colorCycleRed=0;
			Q->colorCycleGreen--;
			Q->colorCycleBlue=0x1F;
			if (Q->colorCycleGreen==0)
				Q->colorCyclePhase=4;
			break;
		case 4:
			Q->colorCycleRed++;
			Q->colorCycleGreen=0;
			Q->colorCycleBlue=0x1F;
			if (Q->colorCycleRed==0x1F)
				Q->colorCyclePhase=5;
			break;
		default:
			Q->colorCycleRed=0x1F;
			Q->colorCycleGreen=0;
			Q->colorCycleBlue--;
			if (Q->colorCycleBlue==0)
				Q->colorCyclePhase=0;
			break;
	}
}



#ifdef TRACE_ENABLED
void TracePrint_arm(register emulState *Q)
{	
	if (!Q->traceInProgress) return;

	Q->traceNumber 	= ByteSwap32(Q->traceNumber);
	Q->traceVal 	= ByteSwap32(Q->traceVal);
	Q->traceVal2 	= ByteSwap32(Q->traceVal2);
	Q->traceVal3 	= ByteSwap32(Q->traceVal3);

	(Q->call68KFuncP)(
		Q->emulStateP,
		(UInt32)Q->TracePrint,
		NULL,
		0);
	return;
}
#endif



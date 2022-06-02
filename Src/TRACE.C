#include "Power48.h"
#include "Power48_core.h"
#include <PalmOS.h>
#include <VFSMgr.h>
#include "TRACE.H"

#ifdef TRACE_ENABLED

#define TraceFileName		"/PALM/PROGRAMS/Power48/Tracefile.txt"

#define TRACE_MAX  200000   // largest trace number

UInt32 traceEntry=0;
FileRef traceRef;
FileHand traceStream;
UInt32 previousRSTKP=0;
Int16 functionLevel=0;
Int16 displayLevel=0;
Boolean writeEntry=false;
Boolean subHide=false;

const char fieldChar1[]={'P', 'W', 'X', 'X', 'S', 'M', 'B', 'W', 'U', 'U', 'U', 'U', 'U', 'U', 'U', 'A'};
const char fieldChar2[]={' ', 'P', 'S', ' ', ' ', ' ', ' ', ' ', 'K', 'K', 'K', 'K', 'K', 'K', 'K', ' '};
const char hexChar[]=   {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};


void TraceOpen(void)
{
	RectangleType myRect;
	RGBColorType prevRGB;
	UInt32 inst,countAND,cycles;
	
	//VFSFileCreate(msVolRefNum,TraceFileName);
	//VFSFileOpen(msVolRefNum,TraceFileName,vfsModeWrite,&traceRef);
	FileDelete(0, "HP48Trace");
	traceStream = FileOpen(0, "HP48Trace", 'HItr', appFileCreator, fileModeReadWrite, NULL);
	
	Q->traceInProgress=1;
	
	WinSetDrawWindow(mainDisplayWindow);
	myRect.topLeft.x=0;
	myRect.topLeft.y=200;
	myRect.extent.x=20;
	myRect.extent.y=20;
	WinSetForeColorRGB(&whiteRGB,&prevRGB);
	WinPaintRectangle(&myRect,0);
	WinSetForeColorRGB(&prevRGB,NULL);
	
	Q->optionTrueSpeed = false;
	cycles = 2000;
	countAND = 0xf;
	inst = 16;
	Q->maxCyclesPerTick = ByteSwap32(cycles);
	Q->instCountAND = ByteSwap32(countAND);
	Q->instPerTick = ByteSwap32(inst);
}

void TraceClose(void)
{
	UInt16 			attributes;
	LocalID			id;
	RectangleType myRect;
	RGBColorType prevRGB;
	
	WinSetDrawWindow(mainDisplayWindow);
	myRect.topLeft.x=0;
	myRect.topLeft.y=200;
	myRect.extent.x=20;
	myRect.extent.y=20;
	WinSetForeColorRGB(&blackRGB,&prevRGB);
	WinPaintRectangle(&myRect,0);
	WinSetForeColorRGB(&prevRGB,NULL);

	//VFSFileClose(traceRef);
	FileClose(traceStream);
	
	// set the backup bit on the trace file
	id = DmFindDatabase(0, "HP48Trace");
	DmDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
				   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	attributes |= dmHdrAttrBackup;
	DmSetDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
					  NULL, NULL, NULL, NULL, NULL, NULL, NULL);		
	SetCPUSpeed();
}


void TracePrint(void) 
{
	char tmp1[220];
	char tmp2[150];
	char tmp3[150];
	Int16 length,i;
	UInt16 traceVal16;
	Int8 jmp;
	Boolean dumpMMU=false;
	Boolean useCurrent=false;
	UInt32 tracePC;
	UInt16 temp=0;
	UInt32 temp32=0;
	Boolean startSubHide=false;
	Boolean noLevelMarks=false;
	UInt32 traceNumber;
	UInt32 traceVal;
	UInt32 traceVal2;
	UInt32 traceVal3;
	
	traceNumber = Q->traceNumber;
	traceVal = Q->traceVal;
	traceVal2 = Q->traceVal2;
	traceVal3 = Q->traceVal3;
	
	traceVal16 = (UInt16)traceVal;
	tracePC = Q->OLD_PC;
	
	traceEntry++;
	
	//if (traceEntry==200000)
	//{
		writeEntry=true;
	//}

	if (Q->OLD_PC>0xFFFFF) { StrPrintF(tmp1,"WARNING: PC Range error detected!! \n"); goto writeToFile; }
	if (Q->OLD_D0>0xFFFFF) { StrPrintF(tmp1,"WARNING: D0 Range error detected!! \n"); goto writeToFile; } 
	if (Q->OLD_D1>0xFFFFF) { StrPrintF(tmp1,"WARNING: D1 Range error detected!! \n"); goto writeToFile; }
	if (Q->OLD_T1>0xF) { StrPrintF(tmp1,"WARNING: T1 Range error detected!! \n"); goto writeToFile; }
			
	StrPrintF(tmp1,"%5lu - ",traceEntry);
	StrPrintF(tmp2,"%lx:",tracePC);
	StrCat(tmp1,&tmp2[3]);

	switch (traceNumber)
	{
		case 0: StrPrintF(tmp2,"RTNSXM"); break;
		case 1: StrPrintF(tmp2,"RTN"); break;
		case 2: StrPrintF(tmp2,"RTNSC"); break;
		case 3: StrPrintF(tmp2,"RTNCC"); break;
		case 4: StrPrintF(tmp2,"SETHEX"); break;
		case 5: StrPrintF(tmp2,"SETDEC"); break;
		case 6: StrPrintF(tmp2,"RSTK=C"); break;
		case 7: StrPrintF(tmp2,"C=RSTK"); break;
		case 8: StrPrintF(tmp2,"CLRST"); break;
		case 9: StrPrintF(tmp2,"C=ST"); break;
		case 10: StrPrintF(tmp2,"ST=C"); break;
		case 11: StrPrintF(tmp2,"CSTEX"); break;
		case 12: StrPrintF(tmp2,"P=P+1"); break;
		case 13: StrPrintF(tmp2,"P=P-1"); break;
		
		case 14: StrPrintF(tmp2,"A=A&B  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 15: StrPrintF(tmp2,"B=B&C  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 16: StrPrintF(tmp2,"C=C&A  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 17: StrPrintF(tmp2,"D=D&C  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 18: StrPrintF(tmp2,"B=B&A  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 19: StrPrintF(tmp2,"C=C&B  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 20: StrPrintF(tmp2,"A=A&C  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 21: StrPrintF(tmp2,"C=C&D  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;

		case 22: StrPrintF(tmp2,"A=A!B  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 23: StrPrintF(tmp2,"B=B!C  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 24: StrPrintF(tmp2,"C=C!A  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 25: StrPrintF(tmp2,"D=D!C  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 26: StrPrintF(tmp2,"B=B!A  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 27: StrPrintF(tmp2,"C=C!B  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 28: StrPrintF(tmp2,"A=A!C  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 29: StrPrintF(tmp2,"C=C!D  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		
		case 30: StrPrintF(tmp2,"RTI"); break;
		
		case 31: StrPrintF(tmp2,"R0=A  W"); break;
		case 32: StrPrintF(tmp2,"R1=A  W"); break;
		case 33: StrPrintF(tmp2,"R2=A  W"); break;
		case 34: StrPrintF(tmp2,"R3=A  W"); break;
		case 35: StrPrintF(tmp2,"R4=A  W"); break;
		case 36: StrPrintF(tmp2,"R0=C  W"); break;
		case 37: StrPrintF(tmp2,"R1=C  W"); break;
		case 38: StrPrintF(tmp2,"R2=C  W"); break;
		case 39: StrPrintF(tmp2,"R3=C  W"); break;
		case 40: StrPrintF(tmp2,"R4=C  W"); break;
		case 41: StrPrintF(tmp2,"A=R0  W"); break;
		case 42: StrPrintF(tmp2,"A=R1  W"); break;
		case 43: StrPrintF(tmp2,"A=R2  W"); break;
		case 44: StrPrintF(tmp2,"A=R3  W"); break;
		case 45: StrPrintF(tmp2,"A=R4  W"); break;
		case 46: StrPrintF(tmp2,"C=R0  W"); break;
		case 47: StrPrintF(tmp2,"C=R1  W"); break;
		case 48: StrPrintF(tmp2,"C=R2  W"); break;
		case 49: StrPrintF(tmp2,"C=R3  W"); break;
		case 50: StrPrintF(tmp2,"C=R4  W"); break;
		
		case 51: StrPrintF(tmp2,"AR0EX  W"); break;
		case 52: StrPrintF(tmp2,"AR1EX  W"); break;
		case 53: StrPrintF(tmp2,"AR2EX  W"); break;
		case 54: StrPrintF(tmp2,"AR3EX  W"); break;
		case 55: StrPrintF(tmp2,"AR4EX  W"); break;
		case 56: StrPrintF(tmp2,"CR0EX  W"); break;
		case 57: StrPrintF(tmp2,"CR1EX  W"); break;
		case 58: StrPrintF(tmp2,"CR2EX  W"); break;
		case 59: StrPrintF(tmp2,"CR3EX  W"); break;
		case 60: StrPrintF(tmp2,"CR4EX  W"); break;
		
		case 61: StrPrintF(tmp2,"D0=A"); break;
		case 62: StrPrintF(tmp2,"D1=A"); break;
		case 63: StrPrintF(tmp2,"AD0EX"); break;
		case 64: StrPrintF(tmp2,"AD1EX"); break;
		case 65: StrPrintF(tmp2,"D0=C"); break;
		case 66: StrPrintF(tmp2,"D1=C"); break;
		case 67: StrPrintF(tmp2,"CD0EX"); break;
		case 68: StrPrintF(tmp2,"CD1EX"); break;
		case 69: StrPrintF(tmp2,"D0=AS"); break;
		case 70: StrPrintF(tmp2,"D1=AS"); break;
		case 71: StrPrintF(tmp2,"AD0XS"); break;
		case 72: StrPrintF(tmp2,"AD1XS"); break;
		case 73: StrPrintF(tmp2,"D0=CS"); break;
		case 74: StrPrintF(tmp2,"D1=CS"); break;
		case 75: StrPrintF(tmp2,"CD0XS"); break;
		case 76: StrPrintF(tmp2,"CD1XS"); break;

		case 77: StrPrintF(tmp2,"DAT0=A  A"); break;
		case 78: StrPrintF(tmp2,"DAT1=A  A"); break;
		case 79: StrPrintF(tmp2,"DAT0=C  A"); break;
		case 80: StrPrintF(tmp2,"DAT1=C  A"); break;
		case 81: StrPrintF(tmp2,"A=DAT0  A"); break;
		case 82: StrPrintF(tmp2,"A=DAT1  A"); break;
		case 83: StrPrintF(tmp2,"C=DAT0  A"); break;
		case 84: StrPrintF(tmp2,"C=DAT1  A"); break;
		case 85: StrPrintF(tmp2,"DAT0=A  B"); break;
		case 86: StrPrintF(tmp2,"DAT1=A  B"); break;
		case 87: StrPrintF(tmp2,"DAT0=C  B"); break;
		case 88: StrPrintF(tmp2,"DAT1=C  B"); break;
		case 89: StrPrintF(tmp2,"A=DAT0  B"); break;
		case 90: StrPrintF(tmp2,"A=DAT1  B"); break;
		case 91: StrPrintF(tmp2,"C=DAT0  B"); break;
		case 92: StrPrintF(tmp2,"C=DAT1  B"); break;
		
		case 93: StrPrintF(tmp2,"DAT0=A  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 94: StrPrintF(tmp2,"DAT1=A  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 95: StrPrintF(tmp2,"DAT0=C  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 96: StrPrintF(tmp2,"DAT1=C  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 97: StrPrintF(tmp2,"A=DAT0  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 98: StrPrintF(tmp2,"A=DAT1  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 99: StrPrintF(tmp2,"C=DAT0  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 100: StrPrintF(tmp2,"C=DAT1  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;

		case 101: StrPrintF(tmp2,"DAT0=A  %c",hexChar[traceVal]); break;
		case 102: StrPrintF(tmp2,"DAT1=A  %c",hexChar[traceVal]); break;
		case 103: StrPrintF(tmp2,"DAT0=C  %c",hexChar[traceVal]); break;
		case 104: StrPrintF(tmp2,"DAT1=C  %c",hexChar[traceVal]); break;
		case 105: StrPrintF(tmp2,"A=DAT0  %c",hexChar[traceVal]); break;
		case 106: StrPrintF(tmp2,"A=DAT1  %c",hexChar[traceVal]); break;
		case 107: StrPrintF(tmp2,"C=DAT0  %c",hexChar[traceVal]); break;
		case 108: StrPrintF(tmp2,"C=DAT1  %c",hexChar[traceVal]); break;

		case 109: StrPrintF(tmp2,"D0=D0+%x",traceVal16); break;
		case 110: StrPrintF(tmp2,"D1=D1+%x",traceVal16); break;
		case 111: StrPrintF(tmp2,"D0=D0-%x",traceVal16); break;
		case 112: StrPrintF(tmp2,"D0=(2) %x",traceVal16); break;
		case 113: StrPrintF(tmp2,"D0=(4) %lx",traceVal); break;
		case 114: StrPrintF(tmp2,"D0=(5) %lx",traceVal); break;
		case 115: StrPrintF(tmp2,"D1=D1-%x",traceVal16); break;
		case 116: StrPrintF(tmp2,"D1=(2) %x",traceVal16); break;
		case 117: StrPrintF(tmp2,"D1=(4) %lx",traceVal); break;
		case 118: StrPrintF(tmp2,"D1=(5) %lx",traceVal); break;
		case 119: StrPrintF(tmp2,"P=%c",hexChar[traceVal]); break;

		case 120: StrPrintF(tmp2,"LCHEX"); break;
		case 121: jmp=(Int8)traceVal; tracePC += jmp+1; if (jmp) StrPrintF(tmp2,"GOC %lx",tracePC); else StrPrintF(tmp2,"RTNC"); break;
		case 122: jmp=(Int8)traceVal; tracePC += jmp+1; if (jmp) StrPrintF(tmp2,"GONC %lx",tracePC); else StrPrintF(tmp2,"RTNNC"); break;
		case 123: if (traceVal&0x800) tracePC-=0xFFF-traceVal; else tracePC+=traceVal+1; StrPrintF(tmp2,"GOTO %lx",tracePC); break;
		case 124: if (traceVal&0x800) tracePC-=0xFFC-traceVal; else tracePC+=traceVal+4; StrPrintF(tmp2,"GOSUB %lx",tracePC); break;

		case 125: StrPrintF(tmp2,"OUT=CS"); break;
		case 126: StrPrintF(tmp2,"OUT=C"); break;
		case 127: StrPrintF(tmp2,"A=IN"); break;
		case 128: StrPrintF(tmp2,"C=IN"); break;
		case 129: StrPrintF(tmp2,"UNCNFG"); break;
		case 130: StrPrintF(tmp2,"CONFIG"); break;
		case 131: StrPrintF(tmp2,"C=ID"); break;
		case 132: StrPrintF(tmp2,"SHUTDN"); break;

		case 133: StrPrintF(tmp2,"INTON"); break;
		case 134: StrPrintF(tmp2,"RSI"); break;
		case 135: StrPrintF(tmp2,"LA"); break;
		case 136: StrPrintF(tmp2,"BUSCB"); break;
		case 137: StrPrintF(tmp2,"ABIT=0  %c",hexChar[traceVal]); break;
		case 138: StrPrintF(tmp2,"ABIT=1  %c",hexChar[traceVal]); break;
		case 139: StrPrintF(tmp2,"?ABIT=0  %c",hexChar[traceVal]); break;
		case 140: StrPrintF(tmp2,"?ABIT=1  %c",hexChar[traceVal]); break;
		case 141: StrPrintF(tmp2,"CBIT=0  %c",hexChar[traceVal]); break;
		case 142: StrPrintF(tmp2,"CBIT=1  %c",hexChar[traceVal]); break;
		case 143: StrPrintF(tmp2,"?CBIT=0  %c",hexChar[traceVal]); break;
		case 144: StrPrintF(tmp2,"?CBIT=1  %c",hexChar[traceVal]); break;
		case 145: StrPrintF(tmp2,"PC=(A)"); break;
		case 146: StrPrintF(tmp2,"BUSCD"); break;
		case 147: StrPrintF(tmp2,"PC=(C)"); break;
		case 148: StrPrintF(tmp2,"INTOFF"); break;

		case 149: StrPrintF(tmp2,"C=C+P+1"); break;
		case 150: StrPrintF(tmp2,"RESET"); break;
		case 151: StrPrintF(tmp2,"BUSCC"); break;
		case 152: StrPrintF(tmp2,"C=P  %c",hexChar[traceVal]); break;
		case 153: StrPrintF(tmp2,"P=C  %c",hexChar[traceVal]); break;
		case 154: StrPrintF(tmp2,"SREQ?"); break;
		case 155: StrPrintF(tmp2,"CPEX  %c",hexChar[traceVal]); break;
		
		case 156: StrPrintF(tmp2,"ASLC"); break;
		case 157: StrPrintF(tmp2,"BSLC"); break;
		case 158: StrPrintF(tmp2,"CSLC"); break;
		case 159: StrPrintF(tmp2,"DSLC"); break;
		case 160: StrPrintF(tmp2,"ASRC"); break;
		case 161: StrPrintF(tmp2,"BSRC"); break;
		case 162: StrPrintF(tmp2,"CSRC"); break;
		case 163: StrPrintF(tmp2,"DSRC"); break;

		case 164: StrPrintF(tmp2,"A=A+%x  %c%c",traceVal16+1,fieldChar1[traceVal2],fieldChar2[traceVal2]); break;
		case 165: StrPrintF(tmp2,"B=B+%x  %c%c",traceVal16+1,fieldChar1[traceVal2],fieldChar2[traceVal2]); break;
		case 166: StrPrintF(tmp2,"C=C+%x  %c%c",traceVal16+1,fieldChar1[traceVal2],fieldChar2[traceVal2]); break;
		case 167: StrPrintF(tmp2,"D=D+%x  %c%c",traceVal16+1,fieldChar1[traceVal2],fieldChar2[traceVal2]); break;
		case 168: StrPrintF(tmp2,"A=A-%x  %c%c",traceVal16+1,fieldChar1[traceVal2],fieldChar2[traceVal2]); break;
		case 169: StrPrintF(tmp2,"B=B-%x  %c%c",traceVal16+1,fieldChar1[traceVal2],fieldChar2[traceVal2]); break;
		case 170: StrPrintF(tmp2,"C=C-%x  %c%c",traceVal16+1,fieldChar1[traceVal2],fieldChar2[traceVal2]); break;
		case 171: StrPrintF(tmp2,"D=D-%x  %c%c",traceVal16+1,fieldChar1[traceVal2],fieldChar2[traceVal2]); break;

		case 172: StrPrintF(tmp2,"ASRB.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 173: StrPrintF(tmp2,"BSRB.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 174: StrPrintF(tmp2,"CSRB.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 175: StrPrintF(tmp2,"DSRB.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;

		case 176: StrPrintF(tmp2,"R0=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 177: StrPrintF(tmp2,"R1=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 178: StrPrintF(tmp2,"R2=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 179: StrPrintF(tmp2,"R3=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 180: StrPrintF(tmp2,"R4=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 181: StrPrintF(tmp2,"R0=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 182: StrPrintF(tmp2,"R1=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 183: StrPrintF(tmp2,"R2=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 184: StrPrintF(tmp2,"R3=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 185: StrPrintF(tmp2,"R4=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;

		case 186: StrPrintF(tmp2,"A=R0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 187: StrPrintF(tmp2,"A=R1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 188: StrPrintF(tmp2,"A=R2.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 189: StrPrintF(tmp2,"A=R3.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 190: StrPrintF(tmp2,"A=R4.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 191: StrPrintF(tmp2,"C=R0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 192: StrPrintF(tmp2,"C=R1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 193: StrPrintF(tmp2,"C=R2.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 194: StrPrintF(tmp2,"C=R3.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 195: StrPrintF(tmp2,"C=R4.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;

		case 196: StrPrintF(tmp2,"AR0EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 197: StrPrintF(tmp2,"AR1EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 198: StrPrintF(tmp2,"AR2EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 199: StrPrintF(tmp2,"AR3EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 200: StrPrintF(tmp2,"AR4EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 201: StrPrintF(tmp2,"CR0EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 202: StrPrintF(tmp2,"CR1EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 203: StrPrintF(tmp2,"CR2EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 204: StrPrintF(tmp2,"CR3EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 205: StrPrintF(tmp2,"CR4EX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;

		case 206: StrPrintF(tmp2,"BEEP"); break;
		case 207: StrPrintF(tmp2,"PC=A"); break;
		case 208: StrPrintF(tmp2,"PC=C"); break;
		case 209: StrPrintF(tmp2,"A=PC"); break;
		case 210: StrPrintF(tmp2,"C=PC"); break;
		case 211: StrPrintF(tmp2,"APCEX"); break;
		case 212: StrPrintF(tmp2,"CPCEX"); break;

		case 213: StrPrintF(tmp2,"ASRB"); break;
		case 214: StrPrintF(tmp2,"BSRB"); break;
		case 215: StrPrintF(tmp2,"CSRB"); break;
		case 216: StrPrintF(tmp2,"DSRB"); break;

		case 217: StrPrintF(tmp2,"HST=0  %c",hexChar[traceVal]); break;
		case 218: StrPrintF(tmp2,"?HST=0  %c",hexChar[traceVal]); break;
		case 219: StrPrintF(tmp2,"ST=0  %c",hexChar[traceVal]); break;
		case 220: StrPrintF(tmp2,"ST=1  %c",hexChar[traceVal]); break;
		case 221: StrPrintF(tmp2,"?ST=0  %c",hexChar[traceVal]); break;
		case 222: StrPrintF(tmp2,"?ST=1  %c",hexChar[traceVal]); break;
		case 223: StrPrintF(tmp2,"?P#%c",hexChar[traceVal]); break;
		case 224: StrPrintF(tmp2,"?P=%c",hexChar[traceVal]); break;

		case 225: StrPrintF(tmp2,"?A=B  A"); break;
		case 226: StrPrintF(tmp2,"?B=C  A"); break;
		case 227: StrPrintF(tmp2,"?C=A  A"); break;
		case 228: StrPrintF(tmp2,"?D=C  A"); break;
		case 229: StrPrintF(tmp2,"?A#B  A"); break;
		case 230: StrPrintF(tmp2,"?B#C  A"); break;
		case 231: StrPrintF(tmp2,"?C#A  A"); break;
		case 232: StrPrintF(tmp2,"?D#C  A"); break;
		case 233: StrPrintF(tmp2,"?A=0  A"); break;
		case 234: StrPrintF(tmp2,"?B=0  A"); break;
		case 235: StrPrintF(tmp2,"?C=0  A"); break;
		case 236: StrPrintF(tmp2,"?D=0  A"); break;
		case 237: StrPrintF(tmp2,"?A#0  A"); break;
		case 238: StrPrintF(tmp2,"?B#0  A"); break;
		case 239: StrPrintF(tmp2,"?C#0  A"); break;
		case 240: StrPrintF(tmp2,"?D#0  A"); break;
		case 241: StrPrintF(tmp2,"?A>B  A"); break;
		case 242: StrPrintF(tmp2,"?B>C  A"); break;
		case 243: StrPrintF(tmp2,"?C>A  A"); break;
		case 244: StrPrintF(tmp2,"?D>C  A"); break;
		case 245: StrPrintF(tmp2,"?A<B  A"); break;
		case 246: StrPrintF(tmp2,"?B<C  A"); break;
		case 247: StrPrintF(tmp2,"?C<A  A"); break;
		case 248: StrPrintF(tmp2,"?D<C  A"); break;
		case 249: StrPrintF(tmp2,"?A>=B  A"); break;
		case 250: StrPrintF(tmp2,"?B>=C  A"); break;
		case 251: StrPrintF(tmp2,"?C>=A  A"); break;
		case 252: StrPrintF(tmp2,"?D>=C  A"); break;
		case 253: StrPrintF(tmp2,"?A<=B  A"); break;
		case 254: StrPrintF(tmp2,"?B<=C  A"); break;
		case 255: StrPrintF(tmp2,"?C<=A  A"); break;
		case 256: StrPrintF(tmp2,"?D<=C  A"); break;

		case 257: if (traceVal&0x8000) tracePC-=0xFFFE-traceVal; else tracePC+=traceVal+2; StrPrintF(tmp2,"GOLONG %lx",tracePC); break;
		case 258: StrPrintF(tmp2,"GOVLNG %lx",traceVal); break;
		case 259: if (traceVal&0x8000) tracePC-=0xFFFA-traceVal; else tracePC+=traceVal+6; StrPrintF(tmp2,"GOSUBL %lx",tracePC); break;
		case 260: StrPrintF(tmp2,"GOSBVL %lx",traceVal); break;

		case 261: StrPrintF(tmp2,"?A=B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 262: StrPrintF(tmp2,"?B=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 263: StrPrintF(tmp2,"?C=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 264: StrPrintF(tmp2,"?D=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 265: StrPrintF(tmp2,"?A#B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 266: StrPrintF(tmp2,"?B#C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 267: StrPrintF(tmp2,"?C#A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 268: StrPrintF(tmp2,"?D#C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 269: StrPrintF(tmp2,"?A=0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 270: StrPrintF(tmp2,"?B=0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 271: StrPrintF(tmp2,"?C=0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 272: StrPrintF(tmp2,"?D=0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 273: StrPrintF(tmp2,"?A#0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 274: StrPrintF(tmp2,"?B#0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 275: StrPrintF(tmp2,"?C#0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 276: StrPrintF(tmp2,"?D#0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		
		case 277: StrPrintF(tmp2,"?A>B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 278: StrPrintF(tmp2,"?B>C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 279: StrPrintF(tmp2,"?C>A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 280: StrPrintF(tmp2,"?D>C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 281: StrPrintF(tmp2,"?A<B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 282: StrPrintF(tmp2,"?B<C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 283: StrPrintF(tmp2,"?C<A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 284: StrPrintF(tmp2,"?D<C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 285: StrPrintF(tmp2,"?A>=B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 286: StrPrintF(tmp2,"?B>=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 287: StrPrintF(tmp2,"?C>=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 288: StrPrintF(tmp2,"?D>=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 289: StrPrintF(tmp2,"?A<=B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 290: StrPrintF(tmp2,"?B<=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 291: StrPrintF(tmp2,"?C<=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 292: StrPrintF(tmp2,"?D<=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		
		case 293: StrPrintF(tmp2,"A=A+B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 294: StrPrintF(tmp2,"B=B+C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 295: StrPrintF(tmp2,"C=C+A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 296: StrPrintF(tmp2,"D=D+C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 297: StrPrintF(tmp2,"A=A+A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 298: StrPrintF(tmp2,"B=B+B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 299: StrPrintF(tmp2,"C=C+C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 300: StrPrintF(tmp2,"D=D+D.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 301: StrPrintF(tmp2,"B=B+A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 302: StrPrintF(tmp2,"C=C+B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 303: StrPrintF(tmp2,"A=A+C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 304: StrPrintF(tmp2,"C=C+D.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 305: StrPrintF(tmp2,"A=A-1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 306: StrPrintF(tmp2,"B=B-1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 307: StrPrintF(tmp2,"C=C-1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 308: StrPrintF(tmp2,"D=D-1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		
		case 309: StrPrintF(tmp2,"A=0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 310: StrPrintF(tmp2,"B=0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 311: StrPrintF(tmp2,"C=0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 312: StrPrintF(tmp2,"D=0.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 313: StrPrintF(tmp2,"A=B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 314: StrPrintF(tmp2,"B=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 315: StrPrintF(tmp2,"C=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 316: StrPrintF(tmp2,"D=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 317: StrPrintF(tmp2,"B=A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 318: StrPrintF(tmp2,"C=B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 319: StrPrintF(tmp2,"A=C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 320: StrPrintF(tmp2,"C=D.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 321: StrPrintF(tmp2,"ABEX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 322: StrPrintF(tmp2,"BCEX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 323: StrPrintF(tmp2,"CAEX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 324: StrPrintF(tmp2,"DCEX.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		
		case 325: StrPrintF(tmp2,"A=A-B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 326: StrPrintF(tmp2,"B=B-C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 327: StrPrintF(tmp2,"C=C-A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 328: StrPrintF(tmp2,"D=D-C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 329: StrPrintF(tmp2,"B=B-A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 330: StrPrintF(tmp2,"C=C-B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 331: StrPrintF(tmp2,"A=A-C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 332: StrPrintF(tmp2,"C=C-D.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 333: StrPrintF(tmp2,"A=A+1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 334: StrPrintF(tmp2,"B=B+1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 335: StrPrintF(tmp2,"C=C+1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 336: StrPrintF(tmp2,"D=D+1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 337: StrPrintF(tmp2,"A=B-A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 338: StrPrintF(tmp2,"B=C-B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 339: StrPrintF(tmp2,"C=A-C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 340: StrPrintF(tmp2,"D=C-D.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		
		case 341: StrPrintF(tmp2,"ASL.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 342: StrPrintF(tmp2,"BSL.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 343: StrPrintF(tmp2,"CSL.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 344: StrPrintF(tmp2,"DSL.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 345: StrPrintF(tmp2,"ASR.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 346: StrPrintF(tmp2,"BSR.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 347: StrPrintF(tmp2,"CSR.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 348: StrPrintF(tmp2,"DSR.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 349: StrPrintF(tmp2,"A=-A.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 350: StrPrintF(tmp2,"B=-B.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 351: StrPrintF(tmp2,"C=-C.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 352: StrPrintF(tmp2,"D=-D.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;		
		case 353: StrPrintF(tmp2,"A=-A-1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 354: StrPrintF(tmp2,"B=-B-1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 355: StrPrintF(tmp2,"C=-C-1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		case 356: StrPrintF(tmp2,"D=-D-1.F  %c%c",fieldChar1[traceVal],fieldChar2[traceVal]); break;
		
		case 357: StrPrintF(tmp2,"A=A+B  A"); break;
		case 358: StrPrintF(tmp2,"B=B+C  A"); break;
		case 359: StrPrintF(tmp2,"C=C+A  A"); break;
		case 360: StrPrintF(tmp2,"D=D+C  A"); break;
		case 361: StrPrintF(tmp2,"A=A+A  A"); break;
		case 362: StrPrintF(tmp2,"B=B+B  A"); break;
		case 363: StrPrintF(tmp2,"C=C+C  A"); break;
		case 364: StrPrintF(tmp2,"D=D+D  A"); break;
		case 365: StrPrintF(tmp2,"B=B+A  A"); break;
		case 366: StrPrintF(tmp2,"C=C+B  A"); break;
		case 367: StrPrintF(tmp2,"A=A+C  A"); break;
		case 368: StrPrintF(tmp2,"C=C+D  A"); break;
		case 369: StrPrintF(tmp2,"A=A-1  A"); break;
		case 370: StrPrintF(tmp2,"B=B-1  A"); break;
		case 371: StrPrintF(tmp2,"C=C-1  A"); break;
		case 372: StrPrintF(tmp2,"D=D-1  A"); break;

		case 373: StrPrintF(tmp2,"A=0  A"); break;
		case 374: StrPrintF(tmp2,"B=0  A"); break;
		case 375: StrPrintF(tmp2,"C=0  A"); break;
		case 376: StrPrintF(tmp2,"D=0  A"); break;		
		case 377: StrPrintF(tmp2,"A=B  A"); break;
		case 378: StrPrintF(tmp2,"B=C  A"); break;
		case 379: StrPrintF(tmp2,"C=A  A"); break;
		case 380: StrPrintF(tmp2,"D=C  A"); break;
		case 381: StrPrintF(tmp2,"B=A  A"); break;
		case 382: StrPrintF(tmp2,"C=B  A"); break;
		case 383: StrPrintF(tmp2,"A=C  A"); break;
		case 384: StrPrintF(tmp2,"C=D  A"); break;		
		case 385: StrPrintF(tmp2,"ABEX  A"); break;
		case 386: StrPrintF(tmp2,"BCEX  A"); break;
		case 387: StrPrintF(tmp2,"CAEX  A"); break;
		case 388: StrPrintF(tmp2,"DCEX  A"); break;
		
		case 389: StrPrintF(tmp2,"A=A-B  A"); break;
		case 390: StrPrintF(tmp2,"B=B-C  A"); break;
		case 391: StrPrintF(tmp2,"C=C-A  A"); break;
		case 392: StrPrintF(tmp2,"D=D-C  A"); break;		
		case 393: StrPrintF(tmp2,"B=B-A  A"); break;
		case 394: StrPrintF(tmp2,"C=C-B  A"); break;
		case 395: StrPrintF(tmp2,"A=A-C  A"); break;
		case 396: StrPrintF(tmp2,"C=C-D  A"); break;		
		case 397: StrPrintF(tmp2,"A=A+1  A"); break;
		case 398: StrPrintF(tmp2,"B=B+1  A"); break;
		case 399: StrPrintF(tmp2,"C=C+1  A"); break;
		case 400: StrPrintF(tmp2,"D=D+1  A"); break;
		case 401: StrPrintF(tmp2,"A=B-A  A"); break;
		case 402: StrPrintF(tmp2,"B=C-B  A"); break;
		case 403: StrPrintF(tmp2,"C=A-C  A"); break;
		case 404: StrPrintF(tmp2,"D=C-D  A"); break;
		
		case 405: StrPrintF(tmp2,"ASL  A"); break;
		case 406: StrPrintF(tmp2,"BSL  A"); break; 
		case 407: StrPrintF(tmp2,"CSL  A"); break;
		case 408: StrPrintF(tmp2,"DSL  A"); break;		
		case 409: StrPrintF(tmp2,"ASR  A"); break;
		case 410: StrPrintF(tmp2,"BSR  A"); break;
		case 411: StrPrintF(tmp2,"CSR  A"); break;
		case 412: StrPrintF(tmp2,"DSR  A"); break;
		case 413: StrPrintF(tmp2,"A=-A  A"); break;
		case 414: StrPrintF(tmp2,"B=-B  A"); break;
		case 415: StrPrintF(tmp2,"C=-C  A"); break;
		case 416: StrPrintF(tmp2,"D=-D  A"); break;		
		case 417: StrPrintF(tmp2,"A=-A-1  A"); break;
		case 418: StrPrintF(tmp2,"B=-B-1  A"); break;
		case 419: StrPrintF(tmp2,"C=-C-1  A"); break;
		case 420: StrPrintF(tmp2,"D=-D-1  A"); break;
		
		case 421: StrPrintF(tmp2,"BAD OPCODE - QUIT"); break;
		case 422: StrPrintF(tmp2,"XXX NVOP3 %lx",traceVal); break;
		case 423: StrPrintF(tmp2,"XXX NVOP4 %lx",traceVal); break;
		case 424: StrPrintF(tmp2,"XXX NVOP5 %lx",traceVal); break;
		case 425: StrPrintF(tmp2,"XXX NVOP6 %lx",traceVal); break;

		case 426: StrPrintF(tmp1,"          LCD Power Toggle = %u \n",ByteSwap16(Q->lcdOn)); goto writeToFile;
		case 427: StrPrintF(tmp1,"          LCD Contrast Adjusted = %u \n",ByteSwap16(Q->lcdContrastLevel)); goto writeToFile;
		case 428: StrPrintF(tmp1,"          LCD Annunciator Touched = %x \n",ByteSwap16(Q->lcdAnnunciatorState)); goto writeToFile;
		case 429: StrPrintF(tmp1,"          LCD Main Display Address = %lx \n",ByteSwap32(Q->lcdDisplayAddressStart)); goto writeToFile;
		case 430: StrPrintF(tmp1,"          LCD Byte Offset = %d \n",ByteSwap16(Q->lcdByteOffset)); goto writeToFile;
		case 431: StrPrintF(tmp1,"          LCD Line Count = %u \n",ByteSwap16(Q->lcdLineCount)); goto writeToFile;
		case 432: StrPrintF(tmp1,"          LCD Menu Display Address = %lx \n",ByteSwap32(Q->lcdMenuAddressStart)); goto writeToFile;
		case 433: StrPrintF(tmp1,"          **** DISPLAY: MAIN area write at %lx, line #%lu \n",traceVal,traceVal2); goto writeToFile;
		case 434: StrPrintF(tmp1,"          **** DISPLAY: MENU area write at %lx, line #%lu \n",traceVal,traceVal2); goto writeToFile;
		case 435: StrPrintF(tmp1,"          ROM Bankswitch - LO:%lu   HI:%lu \n",traceVal,traceVal2); goto writeToFile;
		
		case 436: StrPrintF(tmp1,"          Drawing display line: %lu \n",traceVal); goto writeToFile;
		case 437: StrPrintF(tmp1,"          T2 Timer Expiration! \n"); goto writeToFile;
		case 438: StrPrintF(tmp1,"          T1 Timer Expiration! \n"); goto writeToFile;
		case 439: StrPrintF(tmp1,"          Palm Event poll start \n"); goto writeToFile;
		case 440: StrPrintF(tmp1,"          Palm Event poll finish \n"); goto writeToFile;
		
		case 441: StrPrintF(tmp1," "); dumpMMU=true; goto writeToFile;
		
		case 442: StrPrintF(tmp1,"          MEMORY READ [IO] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 443: StrPrintF(tmp1,"          MEMORY READ [RAM] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 444: StrPrintF(tmp1,"          MEMORY READ [CE2] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 445: StrPrintF(tmp1,"          MEMORY READ [CE1] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 446: StrPrintF(tmp1,"          MEMORY READ [NCE3] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 447: StrPrintF(tmp1,"          MEMORY READ [ROM] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;

		case 448: StrPrintF(tmp1,"          MEMORY WRITE [IO] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 449: StrPrintF(tmp1,"          MEMORY WRITE [RAM] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 450: StrPrintF(tmp1,"          MEMORY WRITE [CE2] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 451: StrPrintF(tmp1,"          MEMORY WRITE [CE1] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 452: StrPrintF(tmp1,"          MEMORY WRITE [NCE3] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		case 453: StrPrintF(tmp1,"          MEMORY WRITE [ROM] - %lu nybbles at SatAdd:%lx   RealAdd:%lx \n",traceVal3,traceVal,traceVal2); goto writeToFile;
		
		case 454: StrPrintF(tmp1,"          LCD: DAS-%lx,  DAS2-%lx,  DAE-%lx,  MAS-%lx,  MAE-%lx   --   DAW:%d,  LC:%u,  BO:%d \n", ByteSwap32(Q->lcdDisplayAddressStart), ByteSwap32(Q->lcdDisplayAddressStart2),
																   ByteSwap32(Q->lcdDisplayAddressEnd), ByteSwap32(Q->lcdMenuAddressStart), ByteSwap32(Q->lcdMenuAddressEnd),
																   ByteSwap16(Q->lcdDisplayAreaWidth), ByteSwap16(Q->lcdLineCount), ByteSwap16(Q->lcdByteOffset)); goto writeToFile; 

		default: StrPrintF(tmp2,"UNKNOWN"); break;
	}
	 
	if (subHide)
	{
		traceEntry--;
		return;
	}
	
	StrPrintF(tmp3," ");
	if (!noLevelMarks)
	{
		if (displayLevel>=8)
		{
			StrCat(tmp3,"* ");
			displayLevel-=8;
		}
		for (i=0;i<displayLevel;i++)
		{
			StrCat(tmp3,". ");
		}
	}
	StrCat(tmp3,tmp2);
	
	if (Q->poporpush==1) functionLevel--;
	if (Q->poporpush==2) functionLevel++;
	Q->poporpush=0;
	if (functionLevel < 0) functionLevel = 0;
	displayLevel = functionLevel;
	if (displayLevel > 15) displayLevel = 15;
	
	length = 38-StrLen(tmp3);
	if (length<1) length=1;
	
	StrCat(tmp1,tmp3);
	
	for (i=0;i<length;i++)
		StrCat(tmp1," ");	
	
	if (!useCurrent)
	{
		StrCat(tmp1,"d0=");
		StrPrintF(tmp2,"%lx, d1=",Q->OLD_D0);
		StrCat(tmp1,&tmp2[3]);
		StrPrintF(tmp2,"%lx",Q->OLD_D1);
		StrCat(tmp1,&tmp2[3]);

		StrPrintF(tmp2,", P=%c, CY=%u, ",hexChar[Q->OLD_P],Q->OLD_CARRY);
		StrCat(tmp1,tmp2);
		
		StrPrintF(tmp2,"A=%lx%lx, B=%lx%lx, C=%lx%lx, D=%lx%lx",Q->A_HI,Q->A_LO,Q->B_HI,Q->B_LO,Q->C_HI,Q->C_LO,Q->D_HI,Q->D_LO);
		StrCat(tmp1,tmp2);

		//StrPrintF(tmp2," - ST=%x, t1=%c, t2=%lx - %lx",Q->OLD_ST,hexChar[Q->OLD_T1],Q->OLD_T2,Q->nibcode);
		//StrCat(tmp1,tmp2);
		
		StrPrintF(tmp2," - %lx\n",Q->nibcode);   // note that this line contains the new line character
		StrCat(tmp1,tmp2);
	}
	else
	{
		StrCat(tmp1,"d0=");
		StrPrintF(tmp2,"%lx, d1=",ByteSwap32(Q->d0));
		StrCat(tmp1,&tmp2[3]);
		StrPrintF(tmp2,"%lx",ByteSwap32(Q->d1));
		StrCat(tmp1,&tmp2[3]);

		StrPrintF(tmp2,", P=%c, CY=%u, ",hexChar[Q->P],Q->CARRY);
		StrCat(tmp1,tmp2);
		
		StrCat(tmp1,"A=");
		for (i=15;i>=0;i--)
		{
			StrPrintF(tmp2,"%c",hexChar[Q->A[i]]);
			StrCat(tmp1,tmp2);
		}
		StrCat(tmp1,", B=");
		for (i=15;i>=0;i--)
		{
			StrPrintF(tmp2,"%c",hexChar[Q->B[i]]);
			StrCat(tmp1,tmp2);
		}
		StrCat(tmp1,", C=");
		for (i=15;i>=0;i--)
		{
			StrPrintF(tmp2,"%c",hexChar[Q->C[i]]);
			StrCat(tmp1,tmp2);
		}
		StrCat(tmp1,", D=");
		for (i=15;i>=0;i--)
		{
			StrPrintF(tmp2,"%c",hexChar[Q->D[i]]);
			StrCat(tmp1,tmp2);
		}
		
		//temp = ((UInt16)Q->ST[3]<<12) | ((UInt16)Q->ST[2]<<8)  |  ((UInt16)Q->ST[1]<<4)  |  (UInt16)Q->ST[0];
		//StrPrintF(tmp2," - ST=%x, t1=%c, t2=%lx",temp,hexChar[Q->t1],ByteSwap32(Q->t2));
		//StrCat(tmp1,tmp2);

		StrPrintF(tmp2," - %lx\n",Q->nibcode);   // note that this line contains the new line character
		StrCat(tmp1,tmp2);
	}
	

writeToFile:

	if (subHide)
	{
		traceEntry--;
		return;
	}
	
	//VFSFileWrite(traceRef,StrLen(tmp1),tmp1,&numWritten);
	if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);
	
	if ((dumpMMU) || (traceEntry==1)) 
	{
		StrPrintF(tmp1,"   \n");
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);
		
		StrPrintF(tmp1,"          MMU INFO:  I/O       RAM       CE1       CE2       NCE3\n");
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);

		StrPrintF(tmp1,"              Size:  --------  ");
		if (Q->mcRAMSizeConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcRAMSize));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcCE1SizeConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcCE1Size));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcCE2SizeConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcCE2Size));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcNCE3SizeConfigured)
			StrPrintF(tmp2,"%lx \n",ByteSwap32(Q->mcNCE3Size));
		else
			StrPrintF(tmp2,"-------- \n");
		StrCat(tmp1,tmp2);
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);
		
		StrPrintF(tmp1,"           Address:  ");
		if (Q->mcIOAddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcIOBase));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcRAMAddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcRAMBase));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcCE1AddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcCE1Base));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcCE2AddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcCE2Base));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcNCE3AddrConfigured)
			StrPrintF(tmp2,"%lx \n",ByteSwap32(Q->mcNCE3Base));
		else
			StrPrintF(tmp2,"-------- \n");
		StrCat(tmp1,tmp2);
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL); 

		StrPrintF(tmp1,"           Ceiling:  ");
		if (Q->mcIOAddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcIOCeil));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcRAMAddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcRAMCeil));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcCE1AddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcCE1Ceil));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcCE2AddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcCE2Ceil));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcNCE3AddrConfigured)
			StrPrintF(tmp2,"%lx \n",ByteSwap32(Q->mcNCE3Ceil));
		else
			StrPrintF(tmp2,"-------- \n");
		StrCat(tmp1,tmp2);
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);

		StrPrintF(tmp1,"     Real Palm Add:  ");
		StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcRAMAddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcRAMBase)+ByteSwap32(Q->mcRAMDelta));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcCE1AddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcCE1Base)+ByteSwap32(Q->mcCE1Delta));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcCE2AddrConfigured)
			StrPrintF(tmp2,"%lx  ",ByteSwap32(Q->mcCE2Base)+ByteSwap32(Q->mcCE2Delta));
		else
			StrPrintF(tmp2,"--------  ");
		StrCat(tmp1,tmp2);
		if (Q->mcNCE3AddrConfigured)
			StrPrintF(tmp2,"%lx \n",ByteSwap32(Q->mcNCE3Base)+ByteSwap32(Q->mcNCE3Delta));
		else
			StrPrintF(tmp2,"-------- \n");
		StrCat(tmp1,tmp2);
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);

		StrPrintF(tmp1,"     ------------- \n");
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);
		
		StrPrintF(tmp1,"     ROMBase: %lx,  RAMBase: %lx,  PORT1Base: %lx,  PORT2Base: %lx \n ",romBase,ramBase,port1Base,port2Base);
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);

		StrPrintF(tmp1,"   \n");
		if (writeEntry) FileWrite(traceStream, tmp1, StrLen(tmp1), 1, NULL);
	}
	
	if (startSubHide)
	{
		startSubHide=false;
		subHide=true;
	}
	
	if ((traceEntry==TRACE_MAX) || (traceNumber==421))
	{
		TraceClose();
		writeEntry=false;
		Q->traceInProgress=0;
	}
}



#endif


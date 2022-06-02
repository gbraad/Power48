/*************************************************************************\
*
*  RPL.C
*
*  This module is responsible for all object loading and unloading from
*  the internal memory of the HP48
*
*  (c)1995 Sebastien Carlier
*
*  Portions (c)2002 by Christoff Giesselink
*
*  Portions (c)2002 by Robert Hildinger
*
\*************************************************************************/

#include <PalmOS.h>
#include "Power48Rsc.h"
#include <SonyCLIE.h>
#include "Power48.h"
#include "Power48_core.h"

// 48SX | 48GX | 49G  | Name
//#7056A #806E9 #806E9 =TEMPOB
//#7056F #806EE #806EE =TEMPTOP
//#70574 #806F3 #806F3 =RSKTOP   (B)
//#70579 #806F8 #806F8 =DSKTOP   (D1)
//#7066E #807ED #80E9B =AVMEM    (D)
//#705B0 #8072F #8076B =INTRPPTR (D0)

#define TEMPOB   ((optionTarget==HP48SX)?0x7056A:0x806E9)
#define TEMPTOP  ((optionTarget==HP48SX)?0x7056F:0x806EE)
#define RSKTOP   ((optionTarget==HP48SX)?0x70574:0x806F3)
#define DSKTOP   ((optionTarget==HP48SX)?0x70579:0x806F8)
#define AVMEM    ((optionTarget!=HP49G)?((optionTarget==HP48SX)?0x7066E:0x807ED):0x80E9B)
#define INTRPPTR ((optionTarget!=HP49G)?((optionTarget==HP48SX)?0x705B0:0x8072F):0x8076B)

#define BINARYHEADER48  		"HPHP48-W"
#define BINARYHEADER49  		"HPHP49-W"

#define MAX_FILES		200

char objectNameTmp[12];

objFileInfoType *fileList;
Boolean statusInternal=true;

const char mkString[14]={0xd,0x4,0x4,0x4,0x7,0x4,0xb,0x4,0x5,0x4,0x2,0x5,0xa,0x3};
const char mk2String[4]={0xd,0x4,0xb,0x4};
const char mk_230[6]={0x4D, 0x4B, 0x32, 0x2E, 0x33, 0x30};

// search for "MDGKER:MK2.30" or "MDGKER:PREVIE" in port1 of a HP48GX
static Boolean MetakernelPresent(void)
{
	Boolean bMkDetect = false;
	int wVersion;
	
	// card in slot1 of a HP48GX enabled
	if ((optionTarget==HP48GX) && (Q->CARDSTATUS&2))
	{
		// check for Metakernel string "MDGKER:"
		if (!MemCmp(&Q->port1[12],mkString,14))
		{
			bMkDetect = true;				// Metakernel detected
			// check for "MK"
			if (!MemCmp(&Q->port1[26],mk2String,4))
			{
				// get version number
				wVersion = ((Q->port1[30] * 10) + Q->port1[34]) * 10 + Q->port1[36];

				// version newer then V2.30, then compatible with HP OS
				bMkDetect = (wVersion <= 230);
			}
		}
	}
	return bMkDetect;
}


UInt8* GetRealAddressNoIO(UInt32 d)
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

inline UInt32 Pack5(UInt32 addr)  // pack into long
{
	UInt8 *n;
	
	n=GetRealAddressNoIO(addr);
	
	return ((UInt32)n[4]<<16)|((UInt32)n[3]<<12)|(n[2]<<8)|(n[1]<<4)|n[0];
}

inline UInt32 Pack2(UInt32 addr)  // pack into long
{
	UInt8 *n;
	
	n=GetRealAddressNoIO(addr);
	
	return (n[1]<<4)|n[0];
}

inline UInt32 BufPack5(UInt8 *n)  // pack into long
{		
	return ((UInt32)n[4]<<16)|((UInt32)n[3]<<12)|(n[2]<<8)|(n[1]<<4)|n[0];
}

inline UInt32 BufPack2(UInt8 *n)  // pack into long
{		
	return (n[1]<<4)|n[0];
}

inline void Unpack5(UInt32 addr, UInt32 val)   // unpack into addr
{
	UInt8 *n;
	
	n=GetRealAddressNoIO(addr);
		
	MemSemaphoreReserve(true);
	*n++=val&0xf;
	val>>=4;
	*n++=val&0xf;
	val>>=4;
	*n++=val&0xf;                        
	val>>=4;
	*n++=val&0xf;
	val>>=4;
	*n=val&0xf;
	MemSemaphoreRelease(true);
}

inline void BufUnpack5(UInt8 *n, UInt32 val)   // unpack into addr
{
	*n++=val&0xf;
	val>>=4;
	*n++=val&0xf;
	val>>=4;
	*n++=val&0xf;                        
	val>>=4;
	*n++=val&0xf;
	val>>=4;
	*n=val&0xf;
}

inline void ReadX(UInt8 *buf, UInt32 addr, UInt32 count)   // store buf from addr
{
	UInt8 *n;
	
	n=GetRealAddressNoIO(addr);
	
	do { *buf++=*n++; } while (--count);
}

inline void WriteX(UInt8 *buf, UInt32 addr, UInt32 count)    // move buf into addr  
{
	UInt8 *n;
	
	n=GetRealAddressNoIO(addr);
	
	MemSemaphoreReserve(true);
	do { *n++=*buf++; } while (--count);
	MemSemaphoreRelease(true);
}


/*********************************************************************
 * RPL_ObjectName: return the name of the object                     *
 *                 at location 'd' in calc memory                    *
 *********************************************************************/
static char *RPL_ObjectName(UInt32 d)
{
	UInt32 prolog;
	
	if (d==0)  return "Empty";
	
	prolog = Pack5( d ); // read prolog
	switch (prolog)
	{
		case 0x026AC: return "Flash PTR";
		case 0x02911: return "System Binary";
		case 0x02933: return "Real";
		case 0x02955: return "Long Real";
		case 0x02977: return "Complex";
		case 0x0299D: return "Long Complex";
		case 0x029BF: return "Character";
		case 0x02BAA: return "Extended Pointer";
		case 0x02E92: return "XLIB Name";
		case 0x02686: return "Symbolic matrix";
		case 0x02A74: return "List";
		case 0x02AB8: return "Algebraic";
		case 0x02ADA: return "Unit";
		case 0x02D9D: return "Program";
		case 0x0312B: return "SEMI";
		case 0x02E48: return "Global Name";
		case 0x02E6D: return "Local Name";
		case 0x02AFC: return "Tagged";
		case 0x02A96: return "Directory";
		case 0x02614: return "Precision Integer";
		case 0x026D5: return "Aplet";
		case 0x026FE: return "Mini Font"; 
		case 0x029E8: return "Array";
		case 0x02A0A: return "Linked Array";
		case 0x02A2C: return "String";
		case 0x02A4E: return "Binary Integer";
		case 0x02B1E: return "Graphic";
		case 0x02B40: return "Library";
		case 0x02B62: return "Backup";
		case 0x02B88: return "Library Data";
		case 0x02BCC: return "Reserved 1";
		case 0x02BEE: return "Reserved 2";
		case 0x02C10: return "Reserved 3";
		case 0x02DCC: return "Code";
		case 0x0263A: return "Precision Real";
		case 0x02660: return "Precision Complex";
		default: break;
	}
	StrPrintF(objectNameTmp,"%lx",prolog);
	return objectNameTmp;
}



/*********************************************************************
 * RPL_ObjectSize: return the size (in nibbles) of the object        *
 *                 at location 'd' in calc memory                    *
 *********************************************************************/

static long RPL_ObjectSize(UInt32 d)
{
	UInt32 n,l = 0;
	UInt32 prolog;
	
	prolog = Pack5( d ); // read prolog
	switch (prolog)
	{
		case 0x026AC: l = (optionTarget!=HP49G) ? 5 : 12; break; 					// Flash PTR (HP49G)
		case 0x02911: l = 10; break;												// System Binary
		case 0x02933: l = 21; break; 												// Real
		case 0x02955: l = 26; break; 												// Long Real
		case 0x02977: l = 37; break; 												// Complex
		case 0x0299D: l = 47; break; 												// Long Complex
		case 0x029BF: l =  7; break; 												// Character
		case 0x02BAA: l = 15; break; 												// Extended Pointer
		case 0x02E92: l = 11; break; 												// XLIB Name
		case 0x02686: if (optionTarget!=HP49G) { l = 5; break; }					// Symbolic matrix (HP49G)
		case 0x02A74: 																// List
		case 0x02AB8: 																// Algebraic
		case 0x02ADA: 																// Unit
		case 0x02D9D: n=5; do { l+=n; d+=n; n=RPL_ObjectSize(d); } while (n);		// Program
						   l += 5; break; 
		case 0x0312B: l = 0; break; 												// SEMI
		case 0x02E48: 																// Global Name
		case 0x02E6D: 																// Local Name
		case 0x02AFC: n=7+Pack2(d+5)*2; l=n+RPL_ObjectSize(d+n); break;				// Tagged
		case 0x02A96: 																// Directory
			n = Pack5(d+8);
			if (n==0) // empty dir
			{
				l=13;
			}
			else
			{
				l = 8+n;
				n = Pack2(d+l)*2+4;
				l += n;
				l += RPL_ObjectSize(d+l);
			}
			break;
		case 0x02614:																// Precision Integer (HP49G)
		case 0x026D5: 																// Aplet (HP49G)
		case 0x026FE: if (optionTarget!=HP49G) { l = 5; break; }					// Mini Font (HP49G)
		case 0x029E8: 																// Array
		case 0x02A0A: 																// Linked Array
		case 0x02A2C: 																// String
		case 0x02A4E: 																// Binary Integer
		case 0x02B1E: 																// Graphic
		case 0x02B40: 																// Library
		case 0x02B62: 																// Backup
		case 0x02B88: 																// Library Data
		case 0x02BCC: 																// Reserved 1, Font (HP49G)
		case 0x02BEE: 																// Reserved 2
		case 0x02C10: 																// Reserved 3
		case 0x02DCC: l=5+Pack5(d+5); break;										// Code
		case 0x0263A: l=5; if (optionTarget==HP49G) { l += Pack5(d+l);				// Precision Real (HP49G)
													  l += Pack5(d+l); } break;
		case 0x02660: l=5; if (optionTarget==HP49G) { l += Pack5(d+l);				// Precision Complex (HP49G)
													  l += Pack5(d+l);
													  l += Pack5(d+l);
													  l += Pack5(d+l); } break;	
		default: l=5;
	}
	return l;
}




/**********************************************************************************
 * RPL_ObjectSizeInBuffer: return the size (in nibbles) of the object in          *
 *                         local buffer memory                                    *
 **********************************************************************************/

static UInt32 RPL_ObjectSizeInBuffer(UInt8 *d)
{
	UInt32 n,l = 0;
	UInt32 prolog;
	
	prolog = BufPack5( d ); // read prolog
	switch (prolog)
	{
		case 0x026AC: l = (optionTarget!=HP49G) ? 5 : 12; break; 					// Flash PTR (HP49G)
		case 0x02911: l = 10; break;												// System Binary
		case 0x02933: l = 21; break; 												// Real
		case 0x02955: l = 26; break; 												// Long Real
		case 0x02977: l = 37; break; 												// Complex
		case 0x0299D: l = 47; break; 												// Long Complex
		case 0x029BF: l =  7; break; 												// Character
		case 0x02BAA: l = 15; break; 												// Extended Pointer
		case 0x02E92: l = 11; break; 												// XLIB Name
		case 0x02686: if (optionTarget!=HP49G) { l = 5; break; }					// Symbolic matrix (HP49G)
		case 0x02A74: 																// List
		case 0x02AB8: 																// Algebraic
		case 0x02ADA: 																// Unit
		case 0x02D9D: n=5;do {l+=n;d+=n;n=RPL_ObjectSizeInBuffer(d);} while (n);	// Program
						   l += 5; break; 
		case 0x0312B: l = 0; break; 												// SEMI
		case 0x02E48: 																// Global Name
		case 0x02E6D: 																// Local Name
		case 0x02AFC: n=7+BufPack2(d+5)*2; l=n+RPL_ObjectSizeInBuffer(d+n); break;	// Tagged
		case 0x02A96: 																// Directory
			n = BufPack5(d+8);
			if (n==0) // empty dir
			{
				l=13;
			}
			else
			{
				l = 8+n;
				n = BufPack2(d+l)*2+4;
				l += n;
				l += RPL_ObjectSizeInBuffer(d+l);
			}
			break;
		case 0x02614:																// Precision Integer (HP49G)
		case 0x026D5: 																// Aplet (HP49G)
		case 0x026FE: if (optionTarget!=HP49G) { l = 5; break; }					// Mini Font (HP49G)
		case 0x029E8: 																// Array
		case 0x02A0A: 																// Linked Array
		case 0x02A2C: 																// String
		case 0x02A4E: 																// Binary Integer
		case 0x02B1E: 																// Graphic
		case 0x02B40: 																// Library
		case 0x02B62: 																// Backup
		case 0x02B88: 																// Library Data
		case 0x02BCC: 																// Reserved 1, Font (HP49G)
		case 0x02BEE: 																// Reserved 2
		case 0x02C10: 																// Reserved 3
		case 0x02DCC: l=5+BufPack5(d+5); break;										// Code
		case 0x0263A: l=5; if (optionTarget==HP49G) { l += BufPack5(d+l);			// Precision Real (HP49G)
													  l += BufPack5(d+l); } break;
		case 0x02660: l=5; if (optionTarget==HP49G) { l += BufPack5(d+l);			// Precision Complex (HP49G)
													  l += BufPack5(d+l);
													  l += BufPack5(d+l);
													  l += BufPack5(d+l); } break;	
		default: l=5;
	}
	return l;
}




/*********************************************************************
 * RPL_SkipOb: return the address after skiping object in 'd'        *
 *********************************************************************/
static UInt32 RPL_SkipOb( UInt32 d )
{
   return( d + RPL_ObjectSize(d) );
}



/*********************************************************************
 * RPL_CreateTemp: return address to 'l' free nibs in Temp Memory    *
 *********************************************************************/
static UInt32 RPL_CreateTemp(UInt32 l)
{
	UInt32 a, b, c;
	UInt8 *p;
	
	l += 6;									// memory for link field (5) + marker (1) and end
	a = Pack5(TEMPTOP);						// tail address of top object 
	b = Pack5(RSKTOP);						// tail address of rtn stack
	c = Pack5(DSKTOP);						// top of data stack
	
	if ((b+l)>c) return 0;					// check if there's enough memory to move DSKTOP

	Unpack5(TEMPTOP, a+l);					// adjust new end of top object
	Unpack5(RSKTOP,  b+l);					// adjust new end of rtn stack
	Unpack5(AVMEM, (c-b-l)/5);				// calculate free memory (*5 nibbles)

	p = (UInt8*)MemPtrNew(b-a);				// move down rtn stack
	ReadX(p,a,b-a);
	WriteX(p,a+l,b-a);
	MemPtrFree(p);
	
	
	Unpack5(a+l-5,l);						// set object length field
	return (a+1);							// return base address of new object
}




/*********************************************************************
 * RPL_Pick: get the address of the object in stack level 'l'        *
 *********************************************************************/
static UInt32 RPL_Pick(int l)
{
	UInt32 stkp;

	if (!l) return 0;
	if (MetakernelPresent()) ++l;					// Metakernel support
	stkp = Pack5(DSKTOP) + (l-1)*5;
	return Pack5(stkp);
}



/*********************************************************************
 * RPL_Push: push the object in n into stack level 1                 *
 *********************************************************************/
static void RPL_Push(UInt32 n)
{
	UInt32 stkp, avmem;

	avmem = Pack5(AVMEM);					// amount of free memory
	if (avmem==0) return;					// no memory free
	avmem--;								// fetch memory
	Unpack5( AVMEM, avmem );					// save new amount of free memory
	stkp = Pack5( DSKTOP );					// get pointer to stack level 1
	if (MetakernelPresent())				// Metakernel running ?
	{
		Unpack5(stkp-5,Pack5(stkp));			// copy object pointer of stack level 1 to new stack level 1 entry
		Unpack5(stkp,n);						// save pointer to new object on stack level 2
		stkp-=5;							// fetch new stack entry
	}
	else
	{
		stkp-=5;							// fetch new stack entry
		Unpack5( stkp, n );					// save pointer to new object on stack level 1
	}
	Unpack5( DSKTOP, stkp );					// save new pointer to stack level 1
}



/*********************************************************************
 * SaveObject: Save the object in level1 to a VFS file               *
 *********************************************************************/
static Boolean SaveObject(char *fileName)
{
	UInt16		volRefNum;
	UInt32		volIterator = vfsIteratorStart;
	FileRef		myRef,dirRef;
	UInt8		*buf;
	void		*mem;
	UInt32 		i;
	UInt32		objAddress,objSize=0;
	char		fileNameFull[120];
	Err			error;
	Boolean		dirFound=false;
	UInt8*		calcMem;
	UInt16 		attributes;
	LocalID		id;
	FileHand	fileHandle;
	
	if (StrLen(fileName)>31) return false;
	
	objAddress = RPL_Pick(1);
	if (objAddress == 0) return false;
	objSize = (RPL_SkipOb(objAddress) - objAddress + 1) / 2;

	if (FtrPtrNew(appFileCreator, tempObjMemFtrNum, objSize, &mem)) return false;
	
	buf=(UInt8*)mem;
	
	MemSemaphoreReserve(true);
	calcMem=GetRealAddressNoIO(objAddress);
	for( i = 0; i < objSize; i++ )
	{
		buf[i]=calcMem[i*2]|(calcMem[(i*2)+1]<<4);
	}
	MemSemaphoreRelease(true);
	
	if (statusInternal)
	{
		fileHandle = FileOpen(0, fileName, ObjFileType, appFileCreator, fileModeReadWrite, &error);
		if (error) 
		{
			FtrPtrFree(appFileCreator, tempObjMemFtrNum);
			return false;
		}
		if (optionTarget!=HP49G)
			FileWrite(fileHandle,BINARYHEADER48,1,8,NULL);
		else
			FileWrite(fileHandle,BINARYHEADER49,1,8,NULL);
			
		FileWrite(fileHandle,buf,1,objSize,NULL);
		
		FileClose(fileHandle);
		
		// set the backup bit on the file
		id = DmFindDatabase(0, fileName);
		DmDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
					   NULL, NULL, NULL, NULL, NULL, NULL, NULL);
		attributes |= dmHdrAttrBackup;
		DmSetDatabaseInfo(0, id, NULL, &attributes, NULL, NULL,
						  NULL, NULL, NULL, NULL, NULL, NULL, NULL);		
	}
	else
	{
		StrPrintF(fileNameFull,objectVFSDirName);
		StrCat(fileNameFull,fileName);
		StrCat(fileNameFull,OBJFileExtension);

		while (volIterator != vfsIteratorStop)
		{
			if (VFSVolumeEnumerate(&volRefNum, &volIterator)) return false;
			error = VFSFileOpen(volRefNum,objectVFSDirName,vfsModeRead,&dirRef);
			if (!error) 
			{
				dirFound=true;
				break;
			}
		}
		if (!dirFound) 
		{
			FtrPtrFree(appFileCreator, tempObjMemFtrNum);
			return false;
		}
		
		VFSFileClose(dirRef);
		
		error = VFSFileCreate(volRefNum,fileNameFull);
		if (error==vfsErrFileAlreadyExists)
		{
			VFSFileDelete(volRefNum,fileNameFull);
			VFSFileCreate(volRefNum,fileNameFull);
		}
		
		if (VFSFileOpen(volRefNum,fileNameFull,vfsModeWrite,&myRef))
		{
			FtrPtrFree(appFileCreator, tempObjMemFtrNum);
			return false;
		}

		if (optionTarget!=HP49G)
			VFSFileWrite(myRef,8,BINARYHEADER48,NULL);
		else
			VFSFileWrite(myRef,8,BINARYHEADER49,NULL);
		
		VFSFileWrite(myRef,objSize,buf,NULL);
		
		VFSFileClose(myRef);
	}
	
	FtrPtrFree(appFileCreator, tempObjMemFtrNum);
	return true;
}



/*********************************************************************
 * LoadObject: load an HP48 object or an ascii file on the stack     *
 *********************************************************************/
static Boolean LoadObject(Int16 fileIndex)
{
	UInt16		volRefNum;
	UInt32		volIterator = vfsIteratorStart;
	FileRef		myRef;
	UInt32		fileSize;
	UInt8		*buf;
	void		*mem;
	UInt32 		numRead;
	Boolean		objectIsBinary;
	UInt32 		i;
	UInt8		byte; 
	UInt32		objAddress,objSize=0;
	UInt8		head[5];	
	char		fileNameFull[120];
	Err			error;
	Boolean		fileFound=false;
	FileHand 	fileHandle;

	
	if (fileList[fileIndex].isInternal)
	{
		fileHandle = FileOpen(fileList[fileIndex].cardNo, fileList[fileIndex].name, 
							  ObjFileType, appFileCreator, fileModeReadOnly, &error);
		if (error) return false;

		FileTell(fileHandle,(Int32*)&fileSize,&error);

		if (fileSize>0x40000)				// max files size 256K
			return false;	

		if (FtrPtrNew(appFileCreator, tempObjMemFtrNum, fileSize*2, &mem)) return false;
		
		buf=(UInt8*)mem;
		
		
		numRead = FileDmRead(fileHandle,mem,fileSize,1,fileSize,&error);
		FileClose(fileHandle);
	 	if (numRead != fileSize )
		{
			FtrPtrFree(appFileCreator, tempObjMemFtrNum);
			return false;
		}
	}
	else
	{
		StrPrintF(fileNameFull,objectVFSDirName);
		StrCat(fileNameFull,fileList[fileIndex].name);
		
		while (volIterator != vfsIteratorStop)
		{
			if (VFSVolumeEnumerate(&volRefNum, &volIterator)) return false;

			// check for presence of file
			error = VFSFileOpen(volRefNum,fileNameFull,vfsModeRead,&myRef);
			if (!error) 
			{
				fileFound=true;
				break;
			}
		}

		if (!fileFound) return false;
		
		if (VFSFileSize(myRef,&fileSize)) return false;
		
		if (fileSize>0x40000)				// max files size 256K
			return false;	
			
		if (FtrPtrNew(appFileCreator, tempObjMemFtrNum, fileSize*2, &mem)) return false;
		
		buf=(UInt8*)mem;
		
		
	 	VFSFileReadData(myRef,fileSize,mem,fileSize,&numRead);
		VFSFileClose(myRef);
	 	if (numRead != fileSize )
		{
			FtrPtrFree(appFileCreator, tempObjMemFtrNum);
			return false;
		}
	}
	
	MemSemaphoreReserve(true);
		
	objectIsBinary = ((buf[fileSize+0]==0x48) &&
					  (buf[fileSize+1]==0x50) &&
					  (buf[fileSize+2]==0x48) &&
					  (buf[fileSize+3]==0x50) &&
					  (buf[fileSize+4]==0x34) &&
					  (buf[fileSize+5]==((optionTarget!=HP49G) ? 0x38 : 0x39)) &&
					  (buf[fileSize+6]==0x2d));

	for( i = 0; i < fileSize; i++ )
	{
		byte=buf[i+fileSize];
		buf[i*2]=byte&0xf;
		buf[(i*2)+1]=byte>>4;
	}

	MemSemaphoreRelease(true);
		
	if (objectIsBinary)
	{ // load as binary
		objSize    = RPL_ObjectSizeInBuffer(buf+16);
		objAddress = RPL_CreateTemp(objSize);
		
		if (objAddress == 0) 
		{
			FtrPtrFree(appFileCreator, tempObjMemFtrNum);
			return false;
		}

		WriteX(buf+16,objAddress,objSize);
	}
	else
	{ // load as string
		objSize = fileSize*2;
		objAddress = RPL_CreateTemp(objSize+10);
		if (objAddress == 0)
		{
			FtrPtrFree(appFileCreator, tempObjMemFtrNum);
			return false;
		}

		BufUnpack5(head,0x02A2C);			// String
		WriteX(head,objAddress,5);
		BufUnpack5(head,objSize+5);			// length of String
		WriteX(head,objAddress+5,5);
		WriteX(buf,objAddress+10,objSize);	// data
	}
	RPL_Push(objAddress);
	
	FtrPtrFree(appFileCreator, tempObjMemFtrNum);
	return true;
}






Int16 listPosition;
Int16 numFiles;
Int16 selectedFile=0;
Boolean selectionHighlighted=false;

const objectButton objectLSLayout[] =  // coordinates relative to upper left of objectLS dialog
{
	{ 131,   3, 203,  31, 2},		// objectLoad, scope field def: 1 = active in Load dialog
	{ 206,   3, 279,  31, 1},		// objectSave,                  2 = active in Save dialog
	{  19, 219,  88, 253, 3},		// objectCancel,                3 = active in both
	{ 193, 219, 262, 253, 3},		// objectOK
	{   7,  58, 233, 200, 1},		// objectList
	{ 244, 150, 270, 173, 1},		// objectUp
	{ 244, 179, 270, 202, 1},		// objectDown
	{ 149, 150, 165, 166, 2},		// objectInternal
	{ 211, 150, 227, 166, 2},		// objectCard
}; 


static UInt16 TranslatePenToObjectButton(Int16 x, Int16 y)
{
	UInt16 i;
	Int16 tmp;
	
	if (statusPortraitViaLandscape)
	{
		tmp=x;
		x=(screenHeight-1)-y;
		y=tmp;
	}
	
	for (i=0;i<maxObjectButtons;i++)
	{
		if ((x >= (objectLSLayout[i].upperLeftX+objectLSDialogOriginX)) &&
		    (x <= (objectLSLayout[i].lowerRightX+objectLSDialogOriginX)) &&
		    (y >= (objectLSLayout[i].upperLeftY+objectLSDialogOriginY)) &&
		    (y <= (objectLSLayout[i].lowerRightY+objectLSDialogOriginY)) &&
		    (statusObjectLSActive&objectLSLayout[i].scope))
		{
			return i;
		}
	}
	return noObject;
}

 
static void UpdateObjectFileListDisplay(RPLactioncode actionCode)
{
	RectangleType myRect,vacatedRect;
	Int16 index;
	Int16 maxFiles;
	Int16 position=58;
	RGBColorType prevRGB,prevRGB2;
	char tmp[256];
	
	maxFiles=(((numFiles-listPosition)<13)?(numFiles-listPosition):13);
	FntSetFont(stdFont);
	WinSetTextColorRGB(&blackRGB,NULL);
	WinSetForeColorRGB(&greyRGB,&prevRGB);
	WinSetBackColorRGB(&greyRGB,&prevRGB2);

	myRect.topLeft.x = objectLSLayout[objectList].upperLeftX+objectLSDialogOriginX;
	myRect.topLeft.y = objectLSLayout[objectList].upperLeftY+objectLSDialogOriginY;
	myRect.extent.x = objectLSLayout[objectList].lowerRightX-objectLSLayout[objectList].upperLeftX+1;
	myRect.extent.y = objectLSLayout[objectList].lowerRightY-objectLSLayout[objectList].upperLeftY+1;
	if (statusPortraitViaLandscape) swapRectPVL(&myRect,screenHeight);
	switch (actionCode)
	{
		case SCROLL_UP:
			WinScrollRectangle(&myRect, ((statusPortraitViaLandscape)?winLeft:winUp), 11, &vacatedRect);
			WinDrawRectangle(&vacatedRect, 0);
			StrCopy(tmp,(fileList[listPosition+12].isInternal)?"I: ":"C: ");
			StrCat(tmp,fileList[listPosition+12].name);
			DrawCharsExpanded(tmp, StrLen(tmp), 10+objectLSDialogOriginX, position+objectLSDialogOriginY+132, 184, winPaint, SINGLE_DENSITY);
			StrPrintF(tmp,"%dK",fileList[listPosition+12].size);
			DrawCharsExpanded(tmp, StrLen(tmp), 196+objectLSDialogOriginX, position+objectLSDialogOriginY+132, 28, winPaint,SINGLE_DENSITY);
			break;
		case SCROLL_DOWN:
			WinScrollRectangle(&myRect, ((statusPortraitViaLandscape)?winRight:winDown), 11, &vacatedRect);
			WinDrawRectangle(&vacatedRect, 0);
			StrCopy(tmp,(fileList[listPosition].isInternal)?"I: ":"C: ");
			StrCat(tmp,fileList[listPosition].name);
			DrawCharsExpanded(tmp, StrLen(tmp), 10+objectLSDialogOriginX, position+objectLSDialogOriginY, 184, winPaint, SINGLE_DENSITY);
			StrPrintF(tmp,"%dK",fileList[listPosition].size);
			DrawCharsExpanded(tmp, StrLen(tmp), 196+objectLSDialogOriginX, position+objectLSDialogOriginY, 28, winPaint,SINGLE_DENSITY);
			break;
		case REDRAW:
		default:
			WinDrawRectangle(&myRect, 0);
			for (index=listPosition;index<(maxFiles+listPosition);index++)
			{
				StrCopy(tmp,(fileList[index].isInternal)?"I: ":"C: ");
				StrCat(tmp,fileList[index].name);
				DrawCharsExpanded(tmp, StrLen(tmp), 10+objectLSDialogOriginX, position+objectLSDialogOriginY, 184, winPaint, SINGLE_DENSITY);
				StrPrintF(tmp,"%dK",fileList[index].size);
				DrawCharsExpanded(tmp, StrLen(tmp), 196+objectLSDialogOriginX, position+objectLSDialogOriginY, 28, winPaint,SINGLE_DENSITY);
				position+=11;
			}
			break;
	}
	
	if ((selectedFile>=listPosition)&&(selectedFile<=(listPosition+12)))
	{
		if (!selectionHighlighted)
		{
			WinSetBackColorRGB(&greyRGB,NULL);
			WinSetForeColorRGB(&blackRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = objectLSLayout[objectList].upperLeftX+objectLSDialogOriginX;
			myRect.topLeft.y = objectLSLayout[objectList].upperLeftY+objectLSDialogOriginY+((selectedFile-listPosition)*11);
			myRect.extent.x = objectLSLayout[objectList].lowerRightX-objectLSLayout[objectList].upperLeftX+1;
			myRect.extent.y = 11;
			P48WinPaintRectangle(&myRect,0);
			WinSetDrawMode(winPaint);
			selectionHighlighted=true;
		}
	}
	else
	{
		selectionHighlighted=false;
	}	
	WinSetBackColorRGB(&prevRGB2,NULL);
	WinSetForeColorRGB(&prevRGB,NULL);
}


static void DepressObjectLSButton(UInt16 objectCode)
{
	RectangleType 	myRect;
	RGBColorType  	prevRGB;
	Int16			penX,penY;
	Boolean			penDown=false;
	Int16			count=0;
	Int16			tempPos;
	Int16			prevSelectedFile=selectedFile;
	
	WinSetForeColorRGB(&redRGB,&prevRGB);

	switch (objectCode)
	{
		case objectUp:
		case objectDown:
			WinSetBackColorRGB(&blackRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = objectLSLayout[objectCode].upperLeftX+objectLSDialogOriginX;
			myRect.topLeft.y = objectLSLayout[objectCode].upperLeftY+objectLSDialogOriginY;
			myRect.extent.x = objectLSLayout[objectCode].lowerRightX-objectLSLayout[objectCode].upperLeftX+1;
			myRect.extent.y = objectLSLayout[objectCode].lowerRightY-objectLSLayout[objectCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,0);
			if (numFiles)
			{
				do 
				{
					EvtGetPen(&penX,&penY,&penDown);
					if (!count)
					{
						count = 100;
						if (objectCode==objectDown) 
						{
							if (listPosition<(numFiles-13))
							{
								listPosition++;
								UpdateObjectFileListDisplay(SCROLL_UP);
								SysTaskDelay(3);
							}
						}
						if (objectCode==objectUp) 
						{
							if (listPosition>0)
							{
								listPosition--;
								UpdateObjectFileListDisplay(SCROLL_DOWN);
								SysTaskDelay(3);
							}
						}
					}
					count--;
				} while (penDown);
			}
			WinSetBackColorRGB(&blackRGB,NULL);
			WinSetDrawMode(winSwap);
			WinPaintRectangle(&myRect,0);
			WinSetDrawMode(winPaint);
			WinSetBackColorRGB(&backRGB,NULL);
			break;
		case objectList:
			if (numFiles)
			{
				do 
				{
					EvtGetPen(&penX,&penY,&penDown);
					if (statusPortraitViaLandscape) penY=penX;
					tempPos = ((Int16)((penY*2)-(objectLSLayout[objectList].upperLeftY+objectLSDialogOriginY))/11);
					if (tempPos<0) tempPos=0;
					if (tempPos>(numFiles-listPosition-1)) tempPos=(numFiles-listPosition-1);
					if (tempPos>12) tempPos=12;
					selectedFile=listPosition+tempPos;
					if (selectedFile!=prevSelectedFile)
					{
						selectionHighlighted=false;
						UpdateObjectFileListDisplay(REDRAW);
						prevSelectedFile=selectedFile;
					}
				} while (penDown);
			}
			break;
		case objectInternal:
		case objectCard:
			WinSetBackColorRGB(&blackRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = objectLSLayout[objectCode].upperLeftX+objectLSDialogOriginX;
			myRect.topLeft.y = objectLSLayout[objectCode].upperLeftY+objectLSDialogOriginY;
			myRect.extent.x = objectLSLayout[objectCode].lowerRightX-objectLSLayout[objectCode].upperLeftX+1;
			myRect.extent.y = objectLSLayout[objectCode].lowerRightY-objectLSLayout[objectCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,10);
			WinSetDrawMode(winPaint);
			WinSetBackColorRGB(&backRGB,NULL);
			break;
		case objectLoad:
		case objectSave:
		case objectCancel:
		case objectOK:
			WinSetBackColorRGB(&greyRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = objectLSLayout[objectCode].upperLeftX+objectLSDialogOriginX;
			myRect.topLeft.y = objectLSLayout[objectCode].upperLeftY+objectLSDialogOriginY;
			myRect.extent.x = objectLSLayout[objectCode].lowerRightX-objectLSLayout[objectCode].upperLeftX+1;
			myRect.extent.y = objectLSLayout[objectCode].lowerRightY-objectLSLayout[objectCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,10);
			WinSetDrawMode(winPaint);
			WinSetBackColorRGB(&backRGB,NULL);
			break;
		default:break;
	}
	
	WinSetForeColorRGB(&prevRGB,NULL);
}


static void ReleaseObjectLSButton(UInt16 objectCode)
{
	RectangleType myRect;
	RGBColorType  prevRGB;
	
	WinSetForeColorRGB(&redRGB,&prevRGB);

	switch (objectCode)
	{
		case objectInternal:
		case objectCard:
			WinSetBackColorRGB(&blackRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = objectLSLayout[objectCode].upperLeftX+objectLSDialogOriginX;
			myRect.topLeft.y = objectLSLayout[objectCode].upperLeftY+objectLSDialogOriginY;
			myRect.extent.x = objectLSLayout[objectCode].lowerRightX-objectLSLayout[objectCode].upperLeftX+1;
			myRect.extent.y = objectLSLayout[objectCode].lowerRightY-objectLSLayout[objectCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,10);
			WinSetDrawMode(winPaint);
			break;
		case objectLoad:
		case objectSave:
		case objectCancel:
		case objectOK:
			WinSetBackColorRGB(&greyRGB,NULL);
			WinSetDrawMode(winSwap);
			myRect.topLeft.x = objectLSLayout[objectCode].upperLeftX+objectLSDialogOriginX;
			myRect.topLeft.y = objectLSLayout[objectCode].upperLeftY+objectLSDialogOriginY;
			myRect.extent.x = objectLSLayout[objectCode].lowerRightX-objectLSLayout[objectCode].upperLeftX+1;
			myRect.extent.y = objectLSLayout[objectCode].lowerRightY-objectLSLayout[objectCode].upperLeftY+1;
			P48WinPaintRectangle(&myRect,10);
			WinSetDrawMode(winPaint);
			break;
		default:break;
	}
	
	WinSetForeColorRGB(&prevRGB,NULL);
	WinSetBackColorRGB(&backRGB,NULL);
}


static void UpdateAvailableMemoryDisplay(void)
{
	RGBColorType prevRGB;
	Char tmp[10];
	long b, c;
	int freeMem;
	
	b = Pack5(RSKTOP);				// tail address of rtn stack
	c = Pack5(DSKTOP);				// top of data stack
	
	freeMem = (int)((c-b)/2048);	// free memory in kilobytes
	
	FntSetFont(largeFont);
	WinSetTextColorRGB(&blackRGB,NULL);
	WinSetForeColorRGB(&greyRGB,&prevRGB);
	WinSetBackColorRGB(&greyRGB,NULL);

	StrPrintF(tmp,"%dK",freeMem);
	DrawCharsExpanded(tmp, StrLen(tmp), 240+objectLSDialogOriginX, 96+objectLSDialogOriginY, 36, winPaint,SINGLE_DENSITY);

	WinSetForeColorRGB(&prevRGB,NULL);
}



static void computeBoxCoordinates(UInt16 objectCode, Coord *x, Coord *y)
{
	if (statusPortraitViaLandscape)
	{
		*x=objectLSLayout[objectCode].upperLeftY+objectLSDialogOriginY;
		*y=screenHeight-(objectLSLayout[objectCode].upperLeftX+objectLSDialogOriginX)-17;
	}
	else
	{
		*x=objectLSLayout[objectCode].upperLeftX+objectLSDialogOriginX;
		*y=objectLSLayout[objectCode].upperLeftY+objectLSDialogOriginY;
	}
}



static void UpdateCheckBoxes(void)
{
	RectangleType trueRect;
	RectangleType falseRect;

	Coord x,y;
	
	trueRect.topLeft.x = 0;
	trueRect.topLeft.y = 0;
	trueRect.extent.x = 17;
	trueRect.extent.y = 17;
	falseRect.topLeft.x = 17;
	falseRect.topLeft.y = 0;
	falseRect.extent.x = 17;
	falseRect.extent.y = 17;
	
	if (statusObjectLSActive==2)
	{
		computeBoxCoordinates(objectInternal,&x,&y);
		WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((statusInternal)?&trueRect:&falseRect),x,y,winPaint);

		computeBoxCoordinates(objectCard,&x,&y);
		WinCopyRectangle(checkBoxWindow,mainDisplayWindow,((!statusInternal)?&trueRect:&falseRect),x,y,winPaint);
	}
}




Boolean alternate=true;

static void UpdateInsPoint(char *saveFile)
{
	Int16 endPos;

	FntSetFont(largeFont);
	endPos = FntCharsWidth(saveFile,StrLen(saveFile));
	if (alternate)
	{
		alternate=false;
		WinSetForeColorRGB(&redRGB,NULL);
	}
	else
	{
		alternate=true;
		WinSetForeColorRGB(&greyRGB,NULL);
	}
	if (statusPortraitViaLandscape)
		WinDrawLine(objectLSSaveFieldOriginY,(Int16)screenHeight-(objectLSSaveFieldOriginX+endPos)-1,objectLSSaveFieldOriginY+FntLineHeight()-1,(Int16)screenHeight-(objectLSSaveFieldOriginX+endPos)-1);
	else
		WinDrawLine(objectLSSaveFieldOriginX+endPos,objectLSSaveFieldOriginY,objectLSSaveFieldOriginX+endPos,objectLSSaveFieldOriginY+FntLineHeight()-1);
}
	




static void UpdateSaveFileName(char *saveFile, Boolean doBackspace)
{	
	char tmp[10];
	UInt16 nullPos;
	Int16 charPos;
	
	FntSetFont(largeFont);
	if (doBackspace)
	{
		alternate=false;
		UpdateInsPoint(saveFile);
		
		WinSetTextColorRGB(&greyRGB,NULL);
		WinSetBackColorRGB(&greyRGB,NULL);
		nullPos = StrLen(saveFile);
		StrCopy(tmp,&saveFile[nullPos-1]);
		saveFile[nullPos-1]=0;
		charPos = FntCharsWidth(saveFile,nullPos-1);
		
		DrawCharsExpanded(tmp, 1, objectLSSaveFieldOriginX+charPos, objectLSSaveFieldOriginY, objectLSSaveFieldExtentX, winPaint, DOUBLE_DENSITY);
		
		UpdateInsPoint(saveFile);
	}
	else
	{
		WinSetTextColorRGB(&blackRGB,NULL);
		WinSetBackColorRGB(&greyRGB,NULL);

		DrawCharsExpanded(saveFile, StrLen(saveFile), objectLSSaveFieldOriginX, objectLSSaveFieldOriginY, objectLSSaveFieldExtentX, winPaint, DOUBLE_DENSITY);
			
		alternate=true;
		UpdateInsPoint(saveFile);		
	}
}

	
	


static void UpdateStackInfoDisplay(void)
{
	RGBColorType prevRGB;
	Char tmp[40];
	
	FntSetFont(largeFont);
	WinSetTextColorRGB(&blackRGB,NULL);
	WinSetForeColorRGB(&greyRGB,&prevRGB);
	WinSetBackColorRGB(&greyRGB,NULL);

	StrPrintF(tmp,"Level 4: ");
	StrCat(tmp,RPL_ObjectName(RPL_Pick(4)));
	DrawCharsExpanded(tmp, StrLen(tmp), 50+objectLSDialogOriginX, 60+objectLSDialogOriginY, 200, winPaint, SINGLE_DENSITY);
	StrPrintF(tmp,"Level 3: ");
	StrCat(tmp,RPL_ObjectName(RPL_Pick(3)));
	DrawCharsExpanded(tmp, StrLen(tmp), 50+objectLSDialogOriginX, 75+objectLSDialogOriginY, 200, winPaint, SINGLE_DENSITY);
	StrPrintF(tmp,"Level 2: ");
	StrCat(tmp,RPL_ObjectName(RPL_Pick(2)));
	DrawCharsExpanded(tmp, StrLen(tmp), 50+objectLSDialogOriginX, 90+objectLSDialogOriginY, 200, winPaint, SINGLE_DENSITY);
	StrPrintF(tmp,"Level 1: ");
	StrCat(tmp,RPL_ObjectName(RPL_Pick(1)));
	DrawCharsExpanded(tmp, StrLen(tmp), 50+objectLSDialogOriginX, 105+objectLSDialogOriginY, 200, winPaint, SINGLE_DENSITY);

	WinSetForeColorRGB(&prevRGB,NULL);
}


static void UpdateObjectLoadOrSaveTab(void)
{
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;
	WinHandle		lsBackStore;
	Coord 			picWidth,picHeight;
	Err				error;
		
	picHandle = DmGetResource(bitmapRsc, ((statusObjectLSActive==1)?ObjectLoadDialogBitmapFamily:ObjectSaveDialogBitmapFamily));
	picBitmap = MemHandleLock(picHandle);
	BmpGetDimensions(picBitmap,&picWidth,&picHeight,NULL);
	if (statusPortraitViaLandscape)
		lsBackStore = WinCreateOffscreenWindow(picHeight,picWidth,nativeFormat,&error);
	else
		lsBackStore = WinCreateOffscreenWindow(picWidth,picHeight,nativeFormat,&error);
	if (error) 
	{
		WinSetDrawWindow(mainDisplayWindow);
		FastDrawBitmap(picBitmap,objectLSDialogOriginX,objectLSDialogOriginY);
	}
	else
	{
		WinSetDrawWindow(lsBackStore);
		FastDrawBitmap(picBitmap,0,0);
		WinSetDrawWindow(mainDisplayWindow);
		if (statusPortraitViaLandscape)
			CrossFade(lsBackStore,mainDisplayWindow,objectLSDialogOriginY,screenHeight-objectLSDialogOriginX-picWidth,true,128);
		else
			CrossFade(lsBackStore,mainDisplayWindow,objectLSDialogOriginX,objectLSDialogOriginY,true,128);
		WinDeleteWindow(lsBackStore,false);
	}
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
		

	if (statusObjectLSActive==2)  // show file name field for save tab
	{
		UpdateStackInfoDisplay();
		UpdateCheckBoxes();
	}
	else
	{
		if (numFiles)
		{
			selectionHighlighted=false;
			UpdateObjectFileListDisplay(REDRAW);
		}
		UpdateAvailableMemoryDisplay();
	}
}



/*************************************************************************\
*
*  FUNCTION:  OpenObjectLoadSaveDialog(void)
*
\*************************************************************************/


void OpenObjectLoadSaveDialog(void)
{
	RectangleType 	oldRect,copyRect;
    EventType 		event;
	Int16 			penX, penY;
	Boolean 		penDown;
	RGBColorType 	prevRGB,prevRGB2;
	char			saveFile[50],newChar,tmp[32];
	UInt16			nullPos;
	UInt16			objectCode;
	UInt16			objectReleasePending=noObject;
	int				applyChangesOrCancel=0;
	Err				error;
	FileRef			dirRef,fileRef;
	Boolean			dirFound=false;
	UInt16			volRefNum;
	UInt32			volIterator = vfsIteratorStart;
	UInt32			fileIterator = vfsIteratorStart;
	FileInfoType	fileInfo;
	Char 			fileName[256];
	Char			*tempFileNamePtr;
	objFileInfoType *objFile;
	Int16			i,j;
	int				fileIndex=0;
	UInt32			fileSize;
	objFileInfoType	temp;
	Boolean			fileDataRetrieved=false;
	Boolean			revertScreen=false;
	DmSearchStateType mySearch;
	UInt16			cardNo;
	LocalID			dbID;
	MemHandle 		picHandle;
	BitmapPtr 		picBitmap;
	Coord 			picWidth,picHeight;
	
	objFile = MemPtrNew(sizeof(objFileInfoType)*MAX_FILES);
	if (objFile==NULL) return;

	Q->mcRAMBase	= ByteSwap32(Q->mcRAMBase);
	Q->mcRAMCeil	= ByteSwap32(Q->mcRAMCeil);
	Q->mcRAMDelta	= ByteSwap32(Q->mcRAMDelta);
	Q->mcCE2Base	= ByteSwap32(Q->mcCE2Base);
	Q->mcCE2Ceil	= ByteSwap32(Q->mcCE2Ceil);
	Q->mcCE2Delta	= ByteSwap32(Q->mcCE2Delta);
	Q->mcCE1Base	= ByteSwap32(Q->mcCE1Base);
	Q->mcCE1Ceil	= ByteSwap32(Q->mcCE1Ceil);
	Q->mcCE1Delta	= ByteSwap32(Q->mcCE1Delta);
	Q->mcNCE3Base	= ByteSwap32(Q->mcNCE3Base);
	Q->mcNCE3Ceil	= ByteSwap32(Q->mcNCE3Ceil);
	Q->mcNCE3Delta	= ByteSwap32(Q->mcNCE3Delta);
	Q->flashRomLo	= (UInt8*)ByteSwap32(Q->flashRomLo);
	Q->flashRomHi	= (UInt8*)ByteSwap32(Q->flashRomHi);
	Q->rom			= (UInt8*)ByteSwap32(Q->rom);
	Q->port1		= (UInt8*)ByteSwap32(Q->port1);

	statusObjectLSActive=1;
	listPosition=0;
	numFiles=0;
	
	if ((optionThreeTwentySquared)&&(statusDisplayMode2x2))
	{
		toggle_lcd(); // put calc screen in 1x1 mode
		revertScreen=true;
	}

	HidePanel();
	 
	copyRect.topLeft.x = 0;
	copyRect.topLeft.y = 0;
	copyRect.extent.x = 320;
	copyRect.extent.y = ((optionThreeTwentySquared)?320:450);
	P48WinCopyRectangle(mainDisplayWindow,backStoreWindow,&copyRect,0,0,winPaint);

	// Get dialog check box graphic
	picHandle = DmGetResource(bitmapRsc, PrefCheckboxGraphicBitmapFamily);
	picBitmap = MemHandleLock(picHandle);
	BmpGetDimensions(picBitmap,&picWidth,&picHeight,NULL);
	checkBoxWindow = WinCreateOffscreenWindow(picWidth,picHeight,nativeFormat,&error);
	if (!error) 
	{
		WinSetDrawWindow(checkBoxWindow);
		WinDrawBitmap(picBitmap,0,0);
		WinSetDrawWindow(mainDisplayWindow);
	}
	MemHandleUnlock(picHandle);
	DmReleaseResource(picHandle);
	if (error)
	{
		OpenAlertConfirmDialog(alert,errorDisplayingOLSString,false);
		goto exitObjectLS;
	}
	
redrawRPL:

	UpdateObjectLoadOrSaveTab();

	if (!fileDataRetrieved)
	{
		// Print message saying "please wait"...
		FntSetFont(stdFont);
		WinSetTextColorRGB(&redRGB,NULL);
		WinSetForeColorRGB(&greyRGB,&prevRGB);
		WinSetBackColorRGB(&greyRGB,&prevRGB2);

		// look for all object files on card if mounted
		if (statusVolumeMounted)
		{
			DrawCharsExpanded("Getting card files, please wait...", 34, 20+objectLSDialogOriginX, 70+objectLSDialogOriginY, 200, winPaint, SINGLE_DENSITY);
			while (volIterator != vfsIteratorStop)
			{
				if (VFSVolumeEnumerate(&volRefNum, &volIterator)) goto exitObjectLS;
				error = VFSFileOpen(volRefNum,objectVFSDirName,vfsModeRead,&dirRef);
				if (!error) 
				{
					dirFound=true;
					break;
				}
			}
			if (dirFound) 
			{
				fileInfo.nameP = fileName;
				fileInfo.nameBufLen = sizeof(fileName);
				
				while (fileIterator != vfsIteratorStop)
				{
					if (VFSDirEntryEnumerate(dirRef, &fileIterator, &fileInfo)) break;
					tempFileNamePtr = MemPtrNew(StrLen(fileInfo.nameP)+1);
					StrCopy(tempFileNamePtr,fileInfo.nameP);
					objFile[fileIndex].name = tempFileNamePtr;
					objFile[fileIndex].isInternal = false;
					objFile[fileIndex].cardNo = 0;
					objFile[fileIndex].dbID = 0;
					fileIndex++;
				}
				numFiles=fileIndex;
				VFSFileClose(dirRef);
				
				DrawCharsExpanded("Getting card sizes, please wait...   ", 37, 20+objectLSDialogOriginX, 70+objectLSDialogOriginY, 200, winPaint, SINGLE_DENSITY);
				for (i=0;i<numFiles;i++)
				{
					objFile[i].size=0;
					StrPrintF(fileName,objectVFSDirName);
					StrCat(fileName,objFile[i].name);
					if (VFSFileOpen(volRefNum,fileName,vfsModeRead,&fileRef)) continue;
					if (VFSFileSize(fileRef,&fileSize)) continue;
					VFSFileClose(fileRef);
					objFile[i].size=(UInt16)(fileSize/1024);
				}
			}
		}
		
		DrawCharsExpanded("Getting internal files, please wait...", 38, 20+objectLSDialogOriginX, 70+objectLSDialogOriginY, 200, winPaint, SINGLE_DENSITY);
		error = DmGetNextDatabaseByTypeCreator(true, &mySearch, ObjFileType, appFileCreator, false, &cardNo, &dbID);
		if (!error)
		{
			do
			{
				DmDatabaseInfo(cardNo,dbID,tmp,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
				tempFileNamePtr = MemPtrNew(StrLen(tmp)+1);
				StrCopy(tempFileNamePtr,tmp);
				objFile[fileIndex].name = tempFileNamePtr;
				DmDatabaseSize(cardNo,dbID,NULL,NULL,&fileSize);
				objFile[fileIndex].size=(UInt16)(fileSize/1024);
				objFile[fileIndex].isInternal = true;
				objFile[fileIndex].cardNo = cardNo;
				objFile[fileIndex].dbID = dbID;
				fileIndex++;
				numFiles++;
				error = DmGetNextDatabaseByTypeCreator(false, &mySearch, ObjFileType, appFileCreator, false, &cardNo, &dbID);
			} while (!error);
		}

		fileList=objFile;
		
		for (i=0;i<100;i++)
		{
			StrPrintF(saveFile,"Object%d",i);
			for (j=0;j<numFiles;j++)
			{
				if (StrCaselessCompare(saveFile,objFile[j].name)==0) break;
			}
			if (j==numFiles) break;
		}
		if (i==100) StrPrintF(saveFile,"");
    
		DrawCharsExpanded("Sorting files, please wait...   ", 29, 20+objectLSDialogOriginX, 70+objectLSDialogOriginY, 200, winPaint, SINGLE_DENSITY);
		for (i=0;i<numFiles-1;i++)
		{
			for (j=0;j<numFiles-1-i;j++)
			{
				if (StrCaselessCompare(objFile[j+1].name,objFile[j].name)<0)
				{
					MemMove(&temp,&objFile[j],sizeof(objFileInfoType));
					MemMove(&objFile[j],&objFile[j+1],sizeof(objFileInfoType));
					MemMove(&objFile[j+1],&temp,sizeof(objFileInfoType));
				}
			}
		}

		
		fileDataRetrieved=true;
	
		oldRect.topLeft.x = 19+objectLSDialogOriginX;
		oldRect.topLeft.y = 69+objectLSDialogOriginY;
		oldRect.extent.x  = 202;
		oldRect.extent.y  = 13;
		P48WinDrawRectangle(&oldRect,0);
		WinSetTextColorRGB(&blackRGB,NULL);
		WinSetForeColorRGB(&prevRGB,NULL);
		WinSetBackColorRGB(&prevRGB2,NULL);
	}
	
	if ((statusObjectLSActive==1) && (numFiles))
		UpdateObjectFileListDisplay(REDRAW);
	if (statusObjectLSActive==2)
		UpdateSaveFileName(saveFile,false);
	
UserInputLoop:

	EvtFlushPenQueue();

    do 
    {
    	WinSetCoordinateSystem(kCoordinatesStandard);
        EvtGetEvent(&event, 30);  // generate nil event every half second
    	WinSetCoordinateSystem(kCoordinatesNative);
    	
		if (event.eType==keyDownEvent)
		{
			if ((event.data.keyDown.chr >= chrSpace) && 
				(event.data.keyDown.chr <= chrTilde) &&
				(statusObjectLSActive==2))
			{
				nullPos = StrLen(saveFile);
				if (nullPos<15)
				{
					newChar = (char)event.data.keyDown.chr;
					saveFile[nullPos]=newChar;
					saveFile[nullPos+1]=0;
					UpdateSaveFileName(saveFile,false);
				}
				continue;
			}
			
			if ((event.data.keyDown.chr == chrBackspace) && 
				(statusObjectLSActive==2))
			{
				nullPos = StrLen(saveFile);
				if (nullPos>0)
				{
					UpdateSaveFileName(saveFile,true);
				}
				continue;
			}

			if (event.data.keyDown.chr == chrLineFeed)
			{
				DepressObjectLSButton(objectOK);
				SysTaskDelay(10);
				ReleaseObjectLSButton(objectOK);
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
		
		if ((event.eType==nilEvent) && (statusObjectLSActive==2))
		{
			UpdateInsPoint(saveFile);
			continue;
		}
		
		if (event.eType==penDownEvent)
        {
        	EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
			objectCode = TranslatePenToObjectButton(penX,penY);
			if (objectCode != noObject)
			{
				DepressObjectLSButton(objectCode);
				objectReleasePending = objectCode;
				continue;
			}
			else
			{
				objectReleasePending = noObject;
			}
		}
		
        if (event.eType==penUpEvent)
		{
			if (objectReleasePending != noObject)
			{
				EvtGetPenNative(mainDisplayWindow, &penX, &penY, &penDown);
				objectCode = TranslatePenToObjectButton(penX,penY);
				ReleaseObjectLSButton(objectReleasePending);
					
				if (objectReleasePending == objectCode)
				{
					switch(objectReleasePending)
					{
						case objectLoad:
							statusObjectLSActive=1;
							UpdateObjectLoadOrSaveTab();
							break;
						case objectSave:
							statusObjectLSActive=2;
							UpdateObjectLoadOrSaveTab();
							UpdateSaveFileName(saveFile,false);
							break;
						case objectInternal:
							statusInternal=true;
							UpdateCheckBoxes();
							break;								
						case objectCard:
							statusInternal=false;
							UpdateCheckBoxes();
							break;								
						case objectCancel:
							applyChangesOrCancel=1;
							break;
						case objectOK:
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
			DepressObjectLSButton(objectCancel);
			SysTaskDelay(10);
			ReleaseObjectLSButton(objectCancel);
			Q->statusQuit=true;
			applyChangesOrCancel=1;
			EvtAddEventToQueue(&event);  // put app stop back on queue
			continue;
		}

	 	if (event.eType==volumeUnmountedEvent)
		{
			if (!statusInternalFiles)
			{
				if (event.data.generic.datum[0]==msVolRefNum)
				{
					statusInUnmountDialog = true;
					if (OpenAlertConfirmDialog(alert,pleaseReinsertCardString,false))
						statusVolumeMounted=false;
					else
						statusVolumeMounted=true;
					statusInUnmountDialog = false;
				}
				continue;
			}
		}
		
		if (event.eType==volumeMountedEvent)
		{
			if (!statusInternalFiles)
			{
				CheckForRemount();
				continue;
			}
		}
		
		if (event.eType==winDisplayChangedEvent)
		{
			if (CheckDisplayTypeForGUIRedraw()) GUIRedraw();
			goto redrawRPL;
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
	
	if (applyChangesOrCancel==2)
	{
		if (statusObjectLSActive==1)
		{
			if (numFiles)
			{
				LoadObject(selectedFile);
				simulate_keypress(keyON);
			}
		}
		else
		{
   			for (j=0;j<numFiles;j++)
			{
				if (StrCompare(saveFile,objFile[j].name)==0) break;
			}
			if (j!=numFiles)
			{
				if (OpenAlertConfirmDialog(confirm,fileExistsString,false))
				{
					SaveObject(saveFile);
				}
				else
				{
					applyChangesOrCancel=0;
					goto UserInputLoop;
				}
			}
    		SaveObject(saveFile);
		}
	}
	
	for (fileIndex=numFiles-1;fileIndex>=0;fileIndex--)
		MemPtrFree(objFile[fileIndex].name);
	
	WinDeleteWindow(checkBoxWindow,false);

exitObjectLS:
	
	CrossFade(backStoreWindow,mainDisplayWindow,0,0,false,128);
	
	ShowPanel();
	
	if (revertScreen)
		toggle_lcd();

	Q->mcRAMBase	= ByteSwap32(Q->mcRAMBase);
	Q->mcRAMCeil	= ByteSwap32(Q->mcRAMCeil);
	Q->mcRAMDelta	= ByteSwap32(Q->mcRAMDelta);
	Q->mcCE2Base	= ByteSwap32(Q->mcCE2Base);
	Q->mcCE2Ceil	= ByteSwap32(Q->mcCE2Ceil);
	Q->mcCE2Delta	= ByteSwap32(Q->mcCE2Delta);
	Q->mcCE1Base	= ByteSwap32(Q->mcCE1Base);
	Q->mcCE1Ceil	= ByteSwap32(Q->mcCE1Ceil);
	Q->mcCE1Delta	= ByteSwap32(Q->mcCE1Delta);
	Q->mcNCE3Base	= ByteSwap32(Q->mcNCE3Base);
	Q->mcNCE3Ceil	= ByteSwap32(Q->mcNCE3Ceil);
	Q->mcNCE3Delta	= ByteSwap32(Q->mcNCE3Delta);
	Q->flashRomLo	= (UInt8*)ByteSwap32(Q->flashRomLo);
	Q->flashRomHi	= (UInt8*)ByteSwap32(Q->flashRomHi);
	Q->rom			= (UInt8*)ByteSwap32(Q->rom);
	Q->port1		= (UInt8*)ByteSwap32(Q->port1);
	
	MemPtrFree(objFile);
}

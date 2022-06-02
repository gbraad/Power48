/*
 *  Copyright © 2004 mobilevoodoo.com
 *  author: Robert Hildinger
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum { CARD, INTERNAL } destType;
typedef enum { HP48SX, HP48GX, HP49G, UNKNOWN } romFileType;

typedef unsigned char  UInt8;
typedef   signed char  Int8;
typedef unsigned short UInt16;
typedef   signed short Int16;
typedef unsigned long  UInt32;
typedef   signed long  Int32;

#define ByteSwap16(n) ( ((((UInt16) n) << 8) & 0xFF00) | \
                        ((((UInt16) n) >> 8) & 0x00FF) )

#define ByteSwap32(n) ( ((((UInt32) n) << 24) & 0xFF000000) |	\
                        ((((UInt32) n) <<  8) & 0x00FF0000) |	\
                        ((((UInt32) n) >>  8) & 0x0000FF00) |	\
                        ((((UInt32) n) >> 24) & 0x000000FF) )

#define TRUE 1
#define FALSE 0

#define dbHdrSize 0x4E

char dbHdr[dbHdrSize];
char dbCreator[]="HILD";
char dbType[]="romF";
char dbRECType[]="DBLK";

char patch49[0x80]=
{
0xF0,0x08,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xF0,0x09,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xF0,0x0A,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xF0,0x0B,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xF0,0x0C,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xF0,0x0D,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xF0,0x0E,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
0xF0,0x0F,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

static char GetAscii(UInt32 nybAddress, UInt8* data, UInt32 maxAddress)
{
	char c;
	UInt32 a;
	
	if (nybAddress>=(maxAddress*2)) return 0;
	
	if (nybAddress&0x1)
	{
		// odd address
		a = nybAddress>>1;
		c = (data[a+1]<<4)|(data[a]>>4);
	}
	else
	{
		//even address
		c = data[nybAddress>>1];
	}
	
	return c;
}


static romFileType GetROMTypeAndVersion(char *verString, UInt8* data, UInt32 maxAddress)
{
	UInt32 verAddress,i;
	romFileType romType;
	char c;
	
	if ((GetAscii(0x7FFF0,data,maxAddress)=='H') && 
		(GetAscii(0x7FFF0+2,data,maxAddress)=='P'))
	{
		romType = HP48SX;
		verAddress=0x7FFF0;
		for (i=0;i<6;i++)
			verString[i]=GetAscii(verAddress+(i*2),data,maxAddress);
		verString[i]=0;
	}
	else if ((GetAscii(0x7FFBF,data,maxAddress)=='H') && 
			 (GetAscii(0x7FFBF+2,data,maxAddress)=='P'))
	{
		romType = HP48GX;
		verAddress=0x7FFBF;
		for (i=0;i<6;i++)
			verString[i]=GetAscii(verAddress+(i*2),data,maxAddress);
		verString[i]=0;
	}
	else if ((GetAscii(0x740E5,data,maxAddress)=='H') && 
			 (GetAscii(0x740E5+2,data,maxAddress)=='P'))
	{
		romType = HP49G;
		verAddress=0x740E5;
		for (i=0;i<32;i++)
		{
			c=GetAscii(verAddress+(i*2),data,maxAddress);
			if (c==',') break;
			verString[i]=c;
		}
		verString[i]=0;
	}
	else
	{
		romType = UNKNOWN;
		verString=NULL;
		return romType;
	}
	return romType;
}


int main(int argc, char *argv[])
{
	destType destination;
	FILE *romFILE,*convFILE;
	UInt32 numRead,numWritten;
	Int32 romFileSize;
	UInt8 *memBase,*data,*a,*b;
	UInt8 temp;
	int validROM = 0;
	int packROM = 0;
	int swapROM = 0;
	int series48ROM = 0;
	char verString[40];
	char fileName[40]="";
	UInt32 i;
	romFileType romType;
	UInt32 checksum;
	UInt32 *p32;
	UInt16 numBlocks;
	UInt16 *p16;
	UInt32 chunk,cswap,size;
	UInt32 temp32,addHighBit;
	
	if ((argc!=3) || 
		(strcmp(argv[2],"CARD") && strcmp(argv[2],"INTERNAL")))
	{
		printf("\n");
		printf("Power48 ROM Converter ver. 1.0\n");
		printf("------------------------------\n");
		printf("\n");
		printf("Usage:\n");
		printf("\t%s ROM_name destination\n", argv[0]);
		printf("\n");
		printf("where ROM_name    = the name of the ROM file to convert\n");
		printf("      destination = CARD or INTERNAL. CARD specifies that the file be \n");
		printf("                    converted to format suitable for storing on a VFS \n");
		printf("                    compatible storage medium such as SD card or memory\n");
		printf("                    stick. INTERNAL specifies that the file be converted\n");
		printf("                    to the PalmOS .pdb format for storing internally.\n");
		printf("\n");
		return 1;
	}

	if (strcmp(argv[2],"CARD"))
	{
		destination = INTERNAL;
		printf("\nConverting file %s to .pdb format...\n",argv[1]);
	}
	else
	{
		destination = CARD;
		printf("\nConverting file %s to VFS compatible format...\n",argv[1]);
		printf("Note: Power48 must be installed and run once\n");
		printf("      to register the default VFS location\n");
		printf("      for .p48rom files, which will then enable a\n");
		printf("      user to hotsync .p48rom files directly to \n");
		printf("      the expansion card.\n");
	}
	
	romFILE = fopen(argv[1],"rb");
	if (romFILE == NULL)
	{
		printf("ERROR: Unable to open file %s\n",argv[1]);
		return 1;
	}
	else
	{
		printf("Opening file %s for conversion...\n",argv[1]);
	}
	
	
	fseek(romFILE,0,SEEK_END);
	romFileSize = ftell(romFILE);
	fseek(romFILE,0,SEEK_SET);
	printf("ROM file size: %lu bytes\n",romFileSize);
	
	memBase = malloc(romFileSize+4);  // include 4 extra bytes for checksum
	if (memBase==NULL)
	{
		fclose(romFILE);
		printf("ERROR: Unable to allocate enough memory to load ROM file!\n");
		return 1;
	}

	data = memBase + 4; // ROM data starts after checksum
	
	numRead = fread(data,1,romFileSize,romFILE);
	fclose(romFILE);
	if (numRead != romFileSize)
	{
		printf("ERROR: Unable to read ROM file!\n");
		return 1;
	}
		
	
	
	switch (data[0])
	{
		case 0x23:
			series48ROM = TRUE;
			validROM = TRUE;
			packROM = FALSE;
			swapROM = TRUE;
			break;
		case 0x32:
			series48ROM = TRUE;
			validROM = TRUE;
			packROM = FALSE;
			swapROM = FALSE;
			break;
		case 0x02:
			if (data[1]==0x03)
			{
				series48ROM = TRUE;
				validROM = TRUE;
				packROM = TRUE;
				swapROM = FALSE;
			}
			if (data[1]==0x06)
			{
				series48ROM = FALSE;
				validROM = TRUE;
				packROM = TRUE;
				swapROM = TRUE;
			}
			break;
		case 0x03:
			if (data[1]==0x02)
			{
				series48ROM = TRUE;
				validROM = TRUE;
				packROM = TRUE;
				swapROM = TRUE;
			}
			break;
		case 0x62:
			series48ROM = FALSE;
			validROM = TRUE;
			packROM = FALSE;
			swapROM = TRUE;
			break;
		case 0x26:
			series48ROM = FALSE;
			validROM = TRUE;
			packROM = FALSE;
			swapROM = FALSE;
			break;
		case 0x06:
			if (data[1]==0x02)
			{
				series48ROM = FALSE;
				validROM = TRUE;
				packROM = TRUE;
				swapROM = FALSE;
			}
			break;
	}

	if (!validROM)
	{
		free(memBase);
		printf("ERROR: ROM file does not appear to be valid!\n");
		return 1;
	}
	
	printf("ROM series: %s\n",(series48ROM)?"48":"49");
	printf("Compression Necessary? %s\n",(packROM)?"YES":"NO");
	printf("Nybble swap Necessary? %s\n",(swapROM)?"YES":"NO");

	if (packROM)
	{
		printf("Now compressing ROM...");
		
		i = romFileSize;
		
		a=data;
		b=a;
		i>>=1;
			
		do
		{
			*a = (*b++)&0xf;
			*a++ |= (*b++)<<4;
		} while(--i);
				
		romFileSize>>=1;
			
		printf(" finished.\n");
	}

	if (swapROM)
	{
		printf("Now nybble swapping ROM...");
				
		for (i=0;i<romFileSize;i++)
		{
			temp = data[i]>>4;
			data[i] = (data[i]<<4) | temp;
		}

		printf(" finished.\n");
	}
	
	romType = GetROMTypeAndVersion(verString,data,romFileSize);
	if (romType==UNKNOWN)
	{
		printf("Unknown ROM Type\n");
		free(memBase);
		return 1;
	}
	else
		printf("ROM Version: %s\n",verString);


	// Apply any ROM patches necessary
	if (romType==HP49G)
	{
		printf("Truncating flash portion from 49G ROM.\n");
		if (romFileSize>0x100000) 
		{
			romFileSize=0x100000;
		
			for (i=0;i<0x80;i++)
				data[i+0xFFF80] = patch49[i];
		}
	}

	
	// Compute checksum value
	checksum=0;
	for (i=0;i<romFileSize;i++)
	{
		addHighBit=FALSE;
		temp32=data[i];
		temp32^=0xFF;
		checksum+=temp32;
		if (checksum&0x1) addHighBit=TRUE;
		checksum>>=1;
		if (addHighBit) checksum|=0x80000000;
	}
	printf("ROM checksum: %lx\n",checksum);

	p32 = (UInt32*)memBase;
	*p32 = ByteSwap32(checksum);
	romFileSize+=4;
	
	if (romType==HP48SX)
		strcat(fileName,"HP48SX");
	else if (romType==HP48GX)
		strcat(fileName,"HP48GX");
	else
		strcat(fileName,"HP49G");
		

	if (destination==CARD)
	{
		// Rom file is destined for VFS card
		printf("Dumping VFS compatible Power48 ROM file...\n");
		
		strcat(fileName,".p48rom");
		convFILE = fopen(fileName,"wb+");
		if (convFILE == NULL)
		{
			printf("ERROR: Unable to create output file\n");
			free(memBase);
			return 1;
		}
		
		numWritten = fwrite(memBase,1,romFileSize,convFILE);
		
		if (numWritten != romFileSize)
			printf("ERROR: ROM file did not get created correctly!\n");
		
		fclose(convFILE);
	}
	else
	{
		// Rom file is destined for INTERNAL storage convert to .PDB
		printf("Dumping .pdb format Power48 ROM file...\n");
		
		strcat(fileName,"ROM.pdb");
		convFILE = fopen(fileName,"wb+");
		if (convFILE == NULL)
		{
			printf("ERROR: Unable to create output file\n");
			free(memBase);
			return 1;
		}
		 
		// insert name in db header
		for (i=0;i<dbHdrSize;i++) dbHdr[i]=0;
		if (romType==HP48SX)
			strcpy(dbHdr,"Power48 48SX ROM");
		else if (romType==HP48GX)
			strcpy(dbHdr,"Power48 48GX ROM");
		else
			strcpy(dbHdr,"Power48 49G ROM");
		
		// insert type and creator fields in header
		strcpy(dbHdr+0x3C,dbType);
		strcpy(dbHdr+0x40,dbCreator);
		
		// set db attributes...
		p16=&dbHdr[0x20];
		*p16 = 0x8000;   // little-endian conversion
		
		// set db version...
		p16=&dbHdr[0x22];
		*p16 = 0x0100;   // little-endian conversion

		// set creation times...
		temp32 = time(NULL) + 0x7c25f6d0;
		p32=&dbHdr[0x24];
		*p32 = ByteSwap32(temp32);;   // little-endian conversion
		p32=&dbHdr[0x28];
		*p32 = ByteSwap32(temp32);;   // little-endian conversion

		// insert number of records in header
		numBlocks = romFileSize / 4096L;
		if ((romFileSize-((UInt32)numBlocks*4096L))>0) numBlocks++;
		p16=&dbHdr[0x4C];
		*p16 = ByteSwap16(numBlocks);
		
		// write header to file
		numWritten = fwrite(dbHdr,1,dbHdrSize,convFILE);
		
		// write record list to file
		size = (numBlocks*8) + 2;
		numWritten=0;
		chunk=dbHdrSize + size;
		for (i=0;i<numBlocks;i++)
		{
			cswap=ByteSwap32(chunk);
			numWritten += fwrite(&cswap,1,4,convFILE);
			cswap=0;
			numWritten += fwrite(&cswap,1,4,convFILE);
			chunk += (4096+8);
		}
		numWritten += fwrite(&cswap,1,2,convFILE); // add two 0x00's to the end of the list
		if (numWritten != size)
			printf("ERROR: ROM file did not get created correctly!\n");
		
		// write ROM records to file
		numWritten=0;
		data=memBase;
		size = romFileSize + (numBlocks*8);
		
		for (i=0;i<numBlocks;i++)
		{
			numWritten += fwrite(dbRECType,1,4,convFILE);
			if (romFileSize>=4096)
			{
				temp32=4096;
				temp32=ByteSwap32(temp32);
				numWritten += fwrite(&temp32,1,4,convFILE);
				numWritten += fwrite(data,1,4096,convFILE);
			}
			else
			{
				temp32=romFileSize;
				temp32=ByteSwap32(temp32);
				numWritten += fwrite(&temp32,1,4,convFILE);
				numWritten += fwrite(data,1,romFileSize,convFILE);
			}
			
			data += 4096;
			romFileSize-=4096;
		}
		if (numWritten != size)
			printf("ERROR: ROM file did not get created correctly!\n");
		
		fclose(convFILE);
	}

	printf("...Finished!\n\n");

	free(memBase);
	return 0;
}

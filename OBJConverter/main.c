/*
 *  Copyright © 2004 mobilevoodoo.com
 *  author: Robert Hildinger
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef enum { CARD, INTERNAL } destType;

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
#define dbHdrNameSize 0x1F

char dbHdr[dbHdrSize];
char dbCreator[]="HILD";
char dbType[]="objF";
char dbRECType[]="DBLK";


int main(int argc, char *argv[])
{
	destType destination;
	FILE *objFILE,*convFILE;
	UInt32 numRead,numWritten;
	Int32 objFileSize;
	UInt8 *memBase,*data;
	char fileNameBuf[150]="";
	Int32 i;
	UInt16 numBlocks;
	UInt16 *p16;
	UInt32 *p32;
	UInt32 chunk,cswap,size;
	UInt32 temp32;
	int objectIsBinary=FALSE;
	char *fName;
	
	
	if ((argc!=3) || 
		(strcmp(argv[2],"CARD") && strcmp(argv[2],"INTERNAL")))
	{
		printf("\n");
		printf("Power48 Object File Converter ver. 1.1\n");
		printf("--------------------------------------\n");
		printf("\n");
		printf("Usage:\n");
		printf("\t%s file_name destination\n", argv[0]);
		printf("\n");
		printf("where file_name   = the name of the object file to convert\n");
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
		printf("      for .p48obj files, which will then enable a\n");
		printf("      user to hotsync .p48obj files directly to \n");
		printf("      the expansion card.\n");
	}
	
	objFILE = fopen(argv[1],"rb");
	if (objFILE == NULL)
	{
		printf("ERROR: Unable to open file %s\n",argv[1]);
		return 1;
	}
	else
	{
		printf("Opening file %s for conversion...\n",argv[1]);
	}
	
	
	fseek(objFILE,0,SEEK_END);
	objFileSize = ftell(objFILE);
	fseek(objFILE,0,SEEK_SET);
	printf("Object file size: %lu bytes\n",objFileSize);
	
	memBase = malloc(objFileSize);
	if (memBase==NULL)
	{
		fclose(objFILE);
		printf("ERROR: Unable to allocate enough memory to load object file!\n");
		return 1;
	}

	data = memBase;
	
	numRead = fread(data,1,objFileSize,objFILE);
	fclose(objFILE);
	if (numRead != objFileSize)
	{
		printf("ERROR: Unable to read object file!\n");
		return 1;
	}
		
	
	objectIsBinary = ((data[0]==0x48) &&
					  (data[1]==0x50) &&
					  (data[2]==0x48) &&
					  (data[3]==0x50) &&
					  (data[4]==0x34) &&
					  ((data[5]==0x38) || (data[5]==0x39)) &&
					  (data[6]==0x2d));
	
	if (objectIsBinary) 
	{
		if (data[5]==0x38)
			printf("File contains an HP48 binary object\n");
		else
			printf("File contains an HP49 binary object\n");
	}
	
	
	strncat(fileNameBuf,argv[1],128);  // don't exceed buffer length
	
	// remove absolute path information
	fName = fileNameBuf;
	for (i=strlen(fileNameBuf)-1;i>=0;i--)
	{
		if (fileNameBuf[i]=='\\' ) 
		{
			fName = &fileNameBuf[i];
			fName++;
			break;
		}
	}
	
	if (destination==CARD)
	{
		// Object file is destined for VFS card
		printf("Dumping VFS compatible Power48 Object file...\n");
		
		strcat(fName,".p48obj");
		convFILE = fopen(fName,"wb+");
		if (convFILE == NULL)
		{
			printf("ERROR: Unable to create output file\n");
			free(memBase);
			return 1;
		}
		
		numWritten = fwrite(memBase,1,objFileSize,convFILE);
		
		if (numWritten != objFileSize)
			printf("ERROR: Object file did not get created correctly!\n");
		
		fclose(convFILE);
	}
	else
	{
		// Object file is destined for INTERNAL storage convert to .PDB
		printf("Dumping .pdb format Power48 Object file...\n");
		
		// insert name in db header
		for (i=0;i<dbHdrSize;i++) dbHdr[i]=0;		
		for (i=0;((i<dbHdrNameSize)&&(i<strlen(fName)));i++) dbHdr[i]=fName[i];
		
		strcat(fName,".pdb");
		convFILE = fopen(fName,"wb+");
		if (convFILE == NULL)
		{
			printf("ERROR: Unable to create output file\n");
			free(memBase);
			return 1;
		}
		 
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
		numBlocks = objFileSize / 4096L;
		if ((objFileSize-((UInt32)numBlocks*4096L))>0) numBlocks++;
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
		
		// write object records to file
		numWritten=0;
		data=memBase;
		size = objFileSize + (numBlocks*8);
		
		for (i=0;i<numBlocks;i++)
		{
			numWritten += fwrite(dbRECType,1,4,convFILE);
			if (objFileSize>=4096)
			{
				temp32=4096;
				temp32=ByteSwap32(temp32);
				numWritten += fwrite(&temp32,1,4,convFILE);
				numWritten += fwrite(data,1,4096,convFILE);
			}
			else
			{
				temp32=objFileSize;
				temp32=ByteSwap32(temp32);
				numWritten += fwrite(&temp32,1,4,convFILE);
				numWritten += fwrite(data,1,objFileSize,convFILE);
			}
			
			data += 4096;
			objFileSize-=4096;
		}
		if (numWritten != size)
			printf("ERROR: Object file did not get created correctly!\n");
		
		fclose(convFILE);
	}

	printf("...Finished!\n\n");

	free(memBase);
	return 0;
}

#ifndef _INTRFACE_H_
#define _INTRFACE_H_

#include "util.h"

/************************************************************************
* function definition
*************************************************************************/

/************************************************************************
* algorithm utility functions		
*************************************************************************/
int algoPreset(const char algoFileName[]);
int algoInit();
int algoGetByte(unsigned char *byteOut);
int algoFinal();

/************************************************************************
* data utility functions
*************************************************************************/
typedef struct toc{
	unsigned char ID;
	unsigned int uncomp_size;
	unsigned char compression;
	unsigned int address;
} DATA_TOC;

typedef struct dataPointer{
	unsigned char ID;
	unsigned int address;
} DATA_BUFFER;

int dataPreset(const char dataFileName[]);
int dataInit();			// initialize data
int dataGetByte(unsigned char *byteOut, short int incCurrentAddr, CSU *checksumUnit);	// get one byte from current column
int dataReset(unsigned char isResetBuffer);	// reset data pointer
int dataFinal();

int HLDataGetByte(unsigned char dataSet, unsigned char *dataByte, unsigned int uncomp_bitsize);
int dataReadthroughComment();
unsigned char getRequestNewData();
int dataLoadTOC(short int storeTOC);
int dataRequestSet(unsigned char dataSet);

/************************************************************************
* decompression utility functions
*************************************************************************/

void set_compression(unsigned char cmp);
unsigned char get_compression();
short int decomp_initFrame(int bitSize);
short int decomp_getByte(unsigned char *byteOut);
short int decomp_getNum();

#endif
#ifndef _UTIL_H_
#define _UTIL_H_

/************************************************************************
* Utility functions
************************************************************************/

typedef struct checksum{
	unsigned int csValue;
	short int csWidth;
	short int csChunkSize;
} CSU;

void init_CS(CSU *cs, short int width, short int chunkSize);
unsigned int getCheckSum(CSU *cs);
void putChunk(CSU *cs, unsigned int chunk);

#endif
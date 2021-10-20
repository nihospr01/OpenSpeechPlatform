#ifndef _SSPIEM_H_
#define _SSPIEM_H_

int SSPIEm_preset(const char devicePath[], const char algoFileName[], const char dataFileName[]);
int SSPIEm(unsigned int algoID);

#endif
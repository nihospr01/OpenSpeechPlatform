/************************************************************************
* Lattice Semiconductor Corp. Copyright 2011
*
* SSPI Embedded System
*
* Version: 4.0.0
*		Add SSPIEm_preset() function prototype 
*
*
* Main processing engine
*
*************************************************************************/
#include "SSPIEm.h"
#include "intrface.h"
#include "core.h"
#include "hardware.h"

/*********************************************************************
*
* SSPIEm_preset()
*
* This function calls algoPreset and dataPreset to set which algorithm
* and data files are going to be processed.  Input(s) to the function
* may depend on configuration.
*
*********************************************************************/

int SSPIEm_preset(const char devicePath[], const char algoFileName[], const char dataFileName[])
{
	int retVal = setdevicePath(devicePath);
	if(retVal)
		retVal = algoPreset(algoFileName);
	if(retVal)
		retVal = dataPreset(dataFileName);
	return retVal;
}
/************************************************************************
* Function SSPIEm
* The main function of the processing engine.  During regular time,
* it automatically gets byte from external storage.  However, this 
* function requires an array of buffered algorithm during 
* loop / repeat operations.  Input bufAlgo must be 0 to indicate
* regular operation.
*
* To call the VME, simply call SSPIEm(int debug);
*************************************************************************/
int SSPIEm(unsigned int algoID)
{
	int retVal = 0;
	retVal = SSPIEm_init(algoID);
	if(retVal <= 0)
		return retVal;
	retVal = SSPIEm_process(0,0);
	return retVal;
}


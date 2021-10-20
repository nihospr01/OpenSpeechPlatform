/*********************************************************************
* Lattice Semiconductor Corp. Copyright 2011
* hardware.cpp
* Version 4.0
*
* This file contains interface between SSPI Embedded and the hardware.
* Depending on hardware availability, implementation may be different.
* Therefore the user is responsible to modify functions here in order
*	to use SSPI Embedded engine to drive the hardware.
***********************************************************************/

#include "intrface.h"
#include "opcode.h"
#include "debug.h"
#include "hardware.h"
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

/***********************************************************************
*
* Global variables.
*
***********************************************************************/
unsigned int a_uiCheckFailedRow = 0;
unsigned int a_uiRowCount		= 0;

// spidev file descriptor (spi device handle)
int _fd;
// spidev device path
char _device[32]  = {0};

/***********************************************************************
*
* Debug utility functions
*
***********************************************************************/

/**********************************************************************
* There are 2 functions here:
*
* dbgu_init() -	this function is responsible to initialize debug 
*					functionality.
*
* dbgu_putint() -	this function take 2 integers from SSPI Embedded
*					processing system.  It is up to the user to take
*					advantage of these integers.
***********************************************************************/

/***********************************************************************
* Function dbgu_init()
* Purpose: Initialize debugging unit.
*
* Return:		1 - succeed
*				0 - fail
*
* If you don't want to use the debug option, you don't need to 
* implement this function.
*
************************************************************************/

/*********************************************************************
* here you may implement debug initializing function.  
**********************************************************************/
int dbgu_init()
{
	return 1;
}

/***********************************************************************
* Function dbgu_putint(int debugCode, int debugCode2)
* Purpose: Return 2 integers from the core for user to run debug.
*
* If you don't want to use the debug option, you don't need to 
* implement this function.
*
* 0x[debugCode][debugCode2] forms a char number that will map to
* a constant string.  You may use these strings to implement flexible
* debugging option
************************************************************************/

/***********************************************************************
* here you may implement debug function.  
************************************************************************/
void dbgu_putint(int debugCode, int debugCode2)
{
	printf("debug codes %d %d\n", debugCode, debugCode2);
}

/***********************************************************************
*
* Debug utility functions
*
************************************************************************/

/***********************************************************************
* The following functions may require user modification:
*
* SPI_init() -	this function is responsible to initialize SSPI 
*					port.
*
* SPI_final() -	this function is responsible to turn off SSPI port.
*
* wait() -		this function take the number of millisecond and
*					wait for the time specified.
*
* TRANS_transmitBytes() -
*					this function is responsible to transmit data over
*					SSPI port.
*
* TRANS_receiveBytes() -
*					this function is responsible to receive data 
*					through SSPI port.
*
* TRANS_starttranx() -
*					this function initiates a transmission by pulling
*					chip-select low.
*
* TRANS_endtranx() -
*					this function terminates the transmission by pulling
*					chip-select high.
*
* TRANS_cstoggle() -
*					this function pulls chip-select low, then pulls it
*					high.
* TRANS_trsttoggle() -
*					this function pulls CRESET low or high.
* TRANS_runClk() -
*					this function is responsible to drive extra clocks 
*					after chip-select is pulled high.
* 
*
* In order to use stream transmission, dataBuffer[] is required to 
* buffer the data.  Please refer to devices' specification for the
* number of bytes required.  For XP2-40, minimum is 423 bytes.
* Declare a little bit more than the minimum, just to be safe.
*
*
************************************************************************/
unsigned char dataBuffer[1024];

/************************************************************************
* Function setdevicePath()
* Purpose: set the path to the spi device
* 
* Return:		1 - succeed
*				0 - fail
*
************************************************************************/
int setdevicePath(const char devicePath[])
{
	strcpy(_device, devicePath);
	return 1;
}

/************************************************************************
* Function dumpstat()
* Purpose: display details about the spi connection
*
************************************************************************/
static void dumpstat(const char *name, int fd)
{
        __u8    lsb, bits;
        __u32   mode, speed;

        if (ioctl(fd, SPI_IOC_RD_MODE32, &mode) < 0) {
                perror("SPI rd_mode");
                return;
        }
        if (ioctl(fd, SPI_IOC_RD_LSB_FIRST, &lsb) < 0) {
                perror("SPI rd_lsb_fist");
                return;
        }
        if (ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits) < 0) {
                perror("SPI bits_per_word");
                return;
        }
        if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed) < 0) {
                perror("SPI max_speed_hz");
                return;
        }

        printf("%s: spi mode 0x%x, %d bits %sper word, %d Hz max\n",
                name, mode, bits, lsb ? "(lsb first) " : "", speed);
}

/************************************************************************
* Function SPI_init()
* Purpose: Initialize SPI port
* 
* Return:		1 - succeed
*				0 - fail
*
* If you already initialize the SPI port in your embedded system,
* simply make the function to return 1.
************************************************************************/

/************************************************************************
* here you may implement SSPI initialization functions.
************************************************************************/
int SPI_init()
{
	printf("\nopening %s\n", _device);
	_fd = open(_device, O_RDWR);
	if (_fd < 0)
	{
		perror("OPEN FAIL");
		return 0;
	}
	dumpstat(_device, _fd);
	return 1;
}
/************************************************************************
* Function SPI_final()
* Purpose: Finalize SPI port
* 
* Return:		1 - succeed
*				0 - fail
*
* If you plan to leave the SPI port on, or it is managed in your 
* embedded system, simply make the function return 1.
************************************************************************/

/************************************************************************
* here you may implement SSPI disable functions.
************************************************************************/
int SPI_final()
{
	printf("closing %s\n", _device);
	close(_fd);
	return 1;
}

/************************************************************************
* Function wait(int ms)
* Purpose: Hold the process for some time (unit millisecond)
* Users must implement a delay to observe a_usTimeDelay, where
* bit 15 of the a_usTimeDelay defines the unit.
*      1 = milliseconds
*      0 = microseconds
* Example:
*      a_usTimeDelay = 0x0001 = 1 microsecond delay.
*      a_usTimeDelay = 0x8001 = 1 millisecond delay.
*
* This subroutine is called upon to provide a delay from 1 millisecond to a few 
* hundreds milliseconds each time. 
* It is understood that due to a_usTimeDelay is defined as unsigned short, a 16 bits
* integer, this function is restricted to produce a delay to 64000 micro-seconds 
* or 32000 milli-second maximum. The VME file will never pass on to this function
* a delay time > those maximum number. If it needs more than those maximum, the VME
* file will launch the delay function several times to realize a larger delay time
* cummulatively.
* It is perfectly alright to provide a longer delay than required. It is not 
* acceptable if the delay is shorter.
*
* Delay function example--using the machine clock signal of the native CPU------
* When porting ispVME to a native CPU environment, the speed of CPU or 
* the system clock that drives the CPU is usually known. 
* The speed or the time it takes for the native CPU to execute one for loop 
* then can be calculated as follows:
*       The for loop usually is compiled into the ASSEMBLY code as shown below:
*       LOOP: DEC RA;
*             JNZ LOOP;
*       If each line of assembly code needs 4 machine cycles to execute, 
*       the total number of machine cycles to execute the loop is 2 x 4 = 8.
*       Usually system clock = machine clock (the internal CPU clock). 
*       Note: Some CPU has a clock multiplier to double the system clock for 
*              the machine clock.
*
*       Let the machine clock frequency of the CPU be F, or 1 machine cycle = 1/F.
*       The time it takes to execute one for loop = (1/F ) x 8.
*       Or one micro-second = F(MHz)/8;
*
* Example: The CPU internal clock is set to 100Mhz, then one micro-second = 100/8 = 12
*
* The C code shown below can be used to create the millisecond accuracy. 
* Users only need to enter the speed of the cpu.
*
************************************************************************/
int wait(int a_msTimeDelay)
{
	struct timespec requested, remaining;
	int seconds = a_msTimeDelay / 1000;
	int milliseconds = a_msTimeDelay % 1000;
	requested.tv_sec = seconds;
	requested.tv_nsec = (milliseconds + 1) * 1000000; // +1 just to be safe

	printf("sleeping %dms\n", a_msTimeDelay);

	if (nanosleep(&requested, &remaining) < 0)
	{
		perror("Nano sleep system call failed");
		return 0;
	}
	return 1;
}

/************************************************************************
* Function TRANS_transmitBytes_CS_helper(unsigned char firstByte, int numBytes)
* Purpose: To determine the proper value for cs_change based on the command
*          and size of data being sent
*
* Since we are using spidev and don't have direct control over the chip
* select line we need to inspect the tx data to determine whether or not
* to release the chip select line after transmitting using the cs_change property.
*
* Commands expecting a response and the start of program data command
* need to hold CS low while the other commands need to release it to
* signal to the FPGA that the transmission is done.
* 
* Returns 0 to hold CS low after tx
* Returns 1 to release CS after tx
************************************************************************/
int TRANS_transmitBytes_CS_helper(unsigned char firstByte, int numBytes)
{
	if (numBytes < 10 &&
		firstByte != 0xE0 && // GET ID
		firstByte != 0x3C && // GET STATUS
		firstByte != 0x7A && // TX BITSTREAM
		firstByte != 0xC0)   // GET USERCODE
	{
		return 0;
	}
	return 1;
}

/************************************************************************
* Function TRANS_transmitBytes(unsigned char *trBuffer, int trCount)
* Purpose: To transmit certain number of bits, indicating by trCount,
* over SPI port.
*
* Data for transmission is stored in trBuffer.
* trCount indicates number of bits to be transmitted.  It should be 
* divisible by 8.  If it is not divisible by 8, pad the data with 1's.
* The function returns 1 if success, or 0 if fail.
************************************************************************/

/************************************************************************
* here you may implement transmitByte function
************************************************************************/
int TRANS_transmitBytes(unsigned char *trBuffer, int trCount)
{
	int status;
	struct spi_ioc_transfer xfer[1];
	int bitsToPad = trCount % 8;
	int numBytes = (trCount + bitsToPad) / 8;

	memset(xfer, 0, sizeof xfer);
	xfer[0].tx_buf = (unsigned long)trBuffer;
	xfer[0].len = numBytes;
	xfer[0].cs_change = TRANS_transmitBytes_CS_helper(trBuffer[0], numBytes);

	printf("tx %d bytes", numBytes);

	if (bitsToPad != 0)
	{
		printf(" - padding %d bits", bitsToPad);
		unsigned char padding = 0xFF >> 8 - bitsToPad;
		trBuffer[numBytes - 1] |= padding;
	}

	if (numBytes < 10)
	{
		printf(" -");
		for (int i = 0; i < numBytes; i++)
		{
			printf(" %02X", trBuffer[i]);
		}
	}
	printf("\n");

	status = ioctl(_fd, SPI_IOC_MESSAGE(1), xfer);
	if (status < 0)
	{
		perror("TX FAIL");
		return 0;
	}

	return 1;
}

/************************************************************************
* Function TRANS_receiveBytes(unsigned char *rcBuffer, int rcCount)
* Purpose: To receive certain number of bits, indicating by rcCount,
* over SPI port.
*
* Data received can be stored in rcBuffer.
* rcCount indicates number of bits to receive.  It should be 
* divisible by 8.  If it is not divisible by 8, pad the data with 1's.
* The function returns 1 if success, or 0 if fail.
*************************************************************************/

/*********************************************************************
* here you may implement transmitByte function
*********************************************************************/
int TRANS_receiveBytes(unsigned char *rcBuffer, int rcCount)
{
	int status;
	struct spi_ioc_transfer xfer[1];
	int bitsToPad = rcCount % 8;
	int numBytes = (rcCount + bitsToPad) / 8;

	memset(xfer, 0, sizeof xfer);
	xfer[0].rx_buf = (unsigned long)rcBuffer;
	xfer[0].len = numBytes;
	xfer[0].cs_change = 0;

	printf("rx %d bytes", numBytes);

	status = ioctl(_fd, SPI_IOC_MESSAGE(1), xfer);
	if (status < 0)
	{
		perror("RX FAIL");
		return 0;
	}

	if (bitsToPad != 0)
	{
		printf(" - padding %d bits", bitsToPad);
		unsigned char padding = 0xFF >> 8 - bitsToPad;
		rcBuffer[numBytes - 1] |= padding;
	}

	if (numBytes < 10)
	{
		printf(" -");
		for (int i = 0; i < numBytes; i++)
		{
			printf(" %02X", rcBuffer[i]);
		}
	}
	printf("\n");

	return 1;
}

/************************************************************************
* Function TRANS_starttranx(unsigned char channel)
* Purpose: To start an SPI transmission
*
* Return:		1 - succeed
*				0 - fail
*
* This function is responsible to pull chip select low.
* If your embedded system has a dedicated SPI port that does not require
* manually pulling chip select low, simply return 1.
*************************************************************************/
/*********************************************************************
* here you should implement starting SPI transmission.
**********************************************************************/	
int TRANS_starttranx(unsigned char channel)
{
	return 1;
}
/************************************************************************
* Function TRANS_endtranx()
* Purpose: To end an SPI transmission
*
* Return:		1 - succeed
*				0 - fail
*
* If your embedded system has a dedicated SPI port that does not require
* implementing this function, simply return 1.
*************************************************************************/

/*********************************************************************
* here you should implement ending SPI transmission.
**********************************************************************/
int TRANS_endtranx()
{
	return 1;
}
/************************************************************************
* Function TRANS_trsttoggle(unsigned char toggle)
* Purpose: To toggle CRESET (TRST) signal
*
* Return:		1 - succeed
*				0 - fail
*
*************************************************************************/
int TRANS_trsttoggle(unsigned char toggle)
{
	/*********************************************************************
	* here you should implement toggling CRESET signal.
	*
	* Currently it prints message on screen and in log file.
	**********************************************************************/
	return 1;
}
/************************************************************************
* Function TRANS_cstoggle(unsigned char channel)
* Purpose: To toggle chip select (CS) of specific channel
*
* Return:		1 - succeed
*				0 - fail
*
* If your embedded system has a dedicated SPI port that does not 
* allow bit banging, simply transmit a byte of 0xFF to the device,
* and the device will ignore that.
*************************************************************************/
int TRANS_cstoggle(unsigned char channel)
{
	if(channel != 0x00)
		return 0;
	else{
	/*********************************************************************
	* here you should implement toggling CS.
	*
	* Currently it prints message on screen and in log file.
	**********************************************************************/

		return 1;
	}
}

/************************************************************************
* Function TRANS_runClk(int clk)
* Purpose: To drive extra clock.
*
* Return:		1 - succeed
*				0 - fail
*
* If your embedded system has a dedicated SPI port that does not 
* allow bit banging, simply transmit a byte of 0xFF on another channel
* that is not being used, so the device will only see the clock.
*************************************************************************/

/*********************************************************************
* here you should implement running free clock
**********************************************************************/
int TRANS_runClk(int clk)
{
	return 1;
}
/************************************************************************
* Function TRANS_transceive_stream(int trCount, unsigned char *trBuffer, 
* 					int trCount2, int flag, unsigned char *trBuffer2
* Purpose: Transmits opcode and transceive data
*
* It will have the following operations depending on the flag:
*
*		NO_DATA: end of transmission.  trCount2 and trBuffer2 are discarded
*		BUFFER_TX: transmit data from trBuffer2
*		BUFFER_RX: receive data and compare it with trBuffer2
*		DATA_TX: transmit data from external source
*		DATA_RX: receive data and compare it with data from external source
*
* If the data is not byte bounded and your SPI port only transmit/ receive
* byte bounded data, you need to have padding to make it byte-bounded.
* If you are transmit non-byte-bounded data, put the padding at the beginning
* of the data.  If you are receiving data, do not compare the padding,
* which is at the end of the transfer.
*************************************************************************/

#define NO_DATA		0
#define BUFFER_TX	1
#define BUFFER_RX	2
#define DATA_TX		3
#define DATA_RX		4

int TRANS_transceive_stream(int trCount, unsigned char *trBuffer, 
							int trCount2, int flag, unsigned char *trBuffer2,
							int mask_flag, unsigned char *maskBuffer)
{
	int i                         = 0;
	unsigned short int tranxByte  = 0;
	unsigned char trByte          = 0;
	unsigned char dataByte        = 0;
	int mismatch                  = 0;
	unsigned char dataID          = 0;

	if(trCount > 0)
	{
		/* calculate # of bytes being transmitted */
		tranxByte = (unsigned short) (trCount / 8);
		if(trCount % 8 != 0){
			tranxByte ++;
			trCount += (8 - (trCount % 8));
		}
		if( !TRANS_transmitBytes(trBuffer, trCount) )
			return ERROR_PROC_HARDWARE;
	}
	switch(flag){
	case NO_DATA:
		return 1;
		break;
	case BUFFER_TX:
		tranxByte = (unsigned short) (trCount2 / 8);
		if(trCount2 % 8 != 0){
			tranxByte ++;
			trCount2 += (8 - (trCount2 % 8));
		}
		
		if(!TRANS_transmitBytes(trBuffer2, trCount2) )
			return ERROR_PROC_HARDWARE;

		return 1;
		break;
	case BUFFER_RX:
		tranxByte = (unsigned short)(trCount2 / 8);
		if(trCount2 % 8 != 0){
			tranxByte ++;
			trCount2 += (8 - (trCount2 % 8));
		}

		if( !TRANS_receiveBytes(trBuffer2, trCount2) )
			return ERROR_PROC_HARDWARE;

		return 1; 
		break;
	case DATA_TX:
		tranxByte = (unsigned short)((trCount2 + 7) / 8);
		if(trCount2 % 8 != 0){
			trByte = (unsigned char)(0xFF << (trCount2 % 8));
		}
		else
			trByte = 0;

		if(trBuffer2 != 0)
			dataID = *trBuffer2;
		else
			dataID = 0x04;

		for (i=0; i<tranxByte; i++){
			if(i == 0){
				if( !HLDataGetByte(dataID, &dataByte, trCount2) )
					return ERROR_INIT_DATA;
			}
			else{
				if( !HLDataGetByte(dataID, &dataByte, 0) )
					return ERROR_INIT_DATA;
			}
			if(trCount2 % 8 != 0)
				trByte += (unsigned char) (dataByte >> (8- (trCount2 % 8)));
			else
				trByte = dataByte;

			dataBuffer[i] = trByte;

			/* do not remove the line below!  It handles the padding for 
			   non-byte-bounded data */
			if(trCount2 % 8 != 0)
				trByte = (unsigned char)(dataByte << (trCount2 % 8));
		}
		if(trCount2 % 8 != 0){
			trCount2 += (8 - (trCount2 % 8));
		}
		if(!TRANS_transmitBytes(dataBuffer, trCount2))
			return ERROR_PROC_HARDWARE;
		return 1;
		break;
	case DATA_RX:
		tranxByte = (unsigned short)(trCount2 / 8);
		if(trCount2 % 8 != 0){
			tranxByte ++;
		}
		if(trBuffer2 != 0)
			dataID = *trBuffer2;
		else
			dataID = 0x04;
		if(!TRANS_receiveBytes(dataBuffer, (tranxByte * 8) ))
			return ERROR_PROC_HARDWARE;
		for(i=0; i<tranxByte; i++){
			if(i == 0){
				if( !HLDataGetByte(dataID, &dataByte, trCount2) )
					return ERROR_INIT_DATA;
			}
			else{
				if( !HLDataGetByte(dataID, &dataByte, 0) )
					return ERROR_INIT_DATA;
			}

			trByte = dataBuffer[i];
			if(mask_flag)
			{
				trByte = trByte & maskBuffer[i];
				dataByte = dataByte & maskBuffer[i];
			}
			if(i == tranxByte - 1){
				trByte = (unsigned char)(trByte ^ dataByte) & 
					(unsigned char)(0xFF << (8 - (trCount2 % 8)));
			}
			else
				trByte = (unsigned char)(trByte ^ dataByte);
			
			if(trByte)
				mismatch ++;
		}
		if(mismatch == 0)
		{
			if(a_uiCheckFailedRow)
			{
				a_uiRowCount++;
			}
			return 1;
		}
		else{
			if(dataID == 0x01 && a_uiRowCount == 0)
			{
				return ERROR_IDCODE;
			}
			else if(dataID == 0x05)
			{
				return ERROR_USERCODE;
			}
			else if(dataID == 0x06)
			{
				return ERROR_SED;
			}
			else if(dataID == 0x07)
			{
				return ERROR_TAG;
			}
			return ERROR_VERIFICATION;
		}
		break;
	default:
		return ERROR_INIT_ALGO;
	}
}
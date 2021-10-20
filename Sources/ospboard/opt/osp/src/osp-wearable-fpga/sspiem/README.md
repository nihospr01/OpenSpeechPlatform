# OSP FPGA Configuration Utility

The directory contains Lattice Semiconductor's embedded FPGA configuration and programming utility. It has been modified to work on the OSP Board using the spidev driver. The implementation has been tested and validated with Lattice Diamond software version 3.11 and 3.12. This application is called from a script that is run on system startup to configure the onboard FPGA. The FPGA onboard flash is not written to, the firmware is copied to the FPGA's RAM. If firmware updates are made, the Lattice Deployment tool needs to be run with the Fast Configuration option to generate an .sea and .sed file for this application to use.


To build:
```
> cmake .
> make
```
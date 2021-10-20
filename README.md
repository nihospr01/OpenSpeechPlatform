#  OSP Release Notes - 2021A

## Transducers: (BTE-RICs)

Hearing Aids with BTE-RIC formfactor have been stable since [Release 2020A](https://github.com/nihospr01/OpenSpeechPlatform-UCSD/tree/2020a). In summary, these peripherals connect to PCD with a 4-wire digital interface for Left (L) and Right (R) sockets on the PCD. Each BTE-RIC comprises 2 mics (front and rear); a third bone conduction mic for acquiring self speech without ambient noise; a fourth mic facing ear drum for in-ear measurements; and a 6-degrees of freedom inertial measurement unit (IMU) with a 3-D accelerometer and a 3-D gyroscope. All the data from the sensors is digital — a field programmable gate array (FPGA) chip multiplexes data and creates 1 ms frames for transmission to the PCD. The PCD sends processed data in 1 ms frames for rendering on the RICs.

## Hardware: (Processing and Communication Device — PCD)

The V8 version of the PCD has been stable since [Release 2020A](https://github.com/nihospr01/OpenSpeechPlatform-UCSD/tree/2020a). In summary, the PCD has an FPGA that de-multiplexes BTE-RICs data and provides the same to the CPUs for processing. The FPGA also receives the processed data from the CPUs for L and R, multiplexes with the BTE_RIC control data and transmits the same in 1 ms frames. The PCD has WiFi, Bluetooth, and GPS modems. Release 2021A uses WiFi for connecting to devices such as smartphones, tablets and PCs wirelessly.

## Firmware and OS

* This release includes all the necessary code to build embedded Linux images for the PCD device.
* Updated to Linux 5.10 kernel
* Improved Wifi stability.
* Reduced image size.
* Added utilities for monitoring temperature and usage of the multiple CPU cores.
* This release introduces a new utility called `pcdtool` that simplifies the process of upgrading firmware and low-level software (flashing the devices).  
* `pcdtool` functions as a terminal to connect to the device over USB, which is used to configure and setup the PCD in either “network mode” or “hotspot mode.”

## Software

* `libosp` is enhanced to record audio output to a file or stream over a TCP port.  
* CPU usage during audio processing is reduced by 20-30% compared to previous releases.
* `libosp` now includes Python bindings and example Jypyter notebooks. Jupyter notebooks are interactive sessions that leverage Python bindings. Jupyter notebooks facilitate experimenting with and validating OSP features for novel psychophysical investigations.
* Example Jupyter notebooks demonstrate how to control and monitor OSP.  A freping (frequency warping feature of OSP) notebook uses the new libosp TCP streaming feature to show realtime signal output in spectral domain while changing freping values interactively, while the realtime processing of speech and audio remain glitch-free.
* OpenMHA ([http://www.openmha.org/](http://www.openmha.org/)) is another master hearing aid suite of algorithms developed under the open-source initiative supported by NIDCD. openMHA works in the spectral domain, in contrast to temporal domain processing of the RT-MHA algorithms in OSP. Release 2021A now includes openMHA within the  OSP framework. Researchers can run valid openMHA configuration files on the PCD.
* Release 2021A includes `libimu` to support IMU acquisition and processing.  `libimu` provides access to 6-D raw data from the three IMUs (L, R, and PCD – a total of 18 dimensional vectors at 100 Hz sampling). The three IMUs can be used for advanced research on physical activity, balance, head orientation, etc. Further, IMU based gesture recognitions such as flip, spin, taps, etc. can be used to simplify the way listeners control PCD. An example of this is included in the revised beamforming app to enable or disable beamforming.

## Applications

A new landing page allows users to change profiles, select the number of bands, or launch any installed web application.

### *Changes to Apps*:

* **Researcher Page**: Improved formatting that enables 6 and 10 band operations.

* **Freping Demo**: Extended to work for both 6 and 10 bands.

* **Beamforming app** allows users to enable or disable beamforming. This can be done using both the user’s device such as a smartphone and gesture input using IMU by rotating the PCD.

* **4AFC Demo is renamed Multi-AFC Demo**, to incorporate any number of forced choices. The Multi-AFC demo currently supports four and six forced choices with stimuli similar to those used in Modified Rhyme Tests (MRT).  Dozens of new stimulus files are currently available on PCD for such tests.  The number of questions can be selected.  In the Multi-AFC demo, audio can be played either through BTE-RICs  or by any browser enabled device with speakers for external audio field stimuli.

## Guides and Documentation

2021A documentation introduces manuals, which provide far more transparency and explanation of most functioning components of the OSP software for both macOS-only software builds and software builds for hardware (PCD) usage. These Manuals comprehensively explain:
* installation and troubleshooting steps
* sanity checks and usage of software builds for both macOS only and for the OSP hardware
* explanation of the apps and demos hosted on the Embedded Web Server (EWS) running on both laptops and PCD
* access, usage, and customization of the Real-Time Master Aid via the terminal Command Line Interface (CLI)

New and existing documentation have been reorganized into separate software and hardware folders:

* **OSP Hardware Documentation** found in the GitHub folder ["Documentation-Hardware-2021A"](Documentation-Hardware-2021A). There are quick guides for setting up the hardware (PCD) for [macOS/Linux](Documentation-Hardware-2021A/01-QuickGuide-macOS_Linux.pdf) and [Windows](Documentation-Hardware-2021A/01-QuickGuide-Windows.pdf), a [Getting Started Guide](Documentation-Hardware-2021A/02-GettingStartedGuide_Hardware.pdf) for a more detailed explanation of the quick guides with sanity checks, and a [Manual](Documentation-Hardware-2021A/03-Manual-Hardware.pdf) to manipulate hardware (PCD) operation.
* **OSP on MacOS Documentation** found in the GitHub folder [Documentation-Software-2021A](Documentation-Software-2021A). There is a [Quick Guide](Documentation-Software-2021A/01-QuickGuide-macOS.pdf) to set up OSP for a macOS laptop/desktop. There is also a [Getting Started Guide](Documentation-Software-2021A/02-GettingStartedGuide-macOS.pdf) for a more detailed explanation of the quick guide with added sections for sanity checks and troubleshooting.
* **OSP Software Documentation** found in the [PDF File "Manual - OSP Software"](Documentation-Software-2021A/03-Manual-Software.pdf). This manual explains how to use the apps and demos in the Embedded Web Server (EWS) interface as well as how to leverage the Command Line Interface (CLI) to manipulate the Real-time Master Hearing Aid (RT-MHA).

Downloadable PDF files have been numbered in ascending order, starting from “01”. Please try to follow the guides in order.

## OSP Forum

Forum has been removed from the website. An improved Forum is under construction.
Meanwhile, please submit general inquiries to the OSP team by completing the [Google Form “OSP Feedback Form”](https://forms.gle/WXt8XbyV5FeAryGe7).
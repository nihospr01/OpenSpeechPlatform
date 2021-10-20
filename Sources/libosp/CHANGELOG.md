# Changelog
All **notable** changes to this project will be documented in this file.

## [1.5.4] - 2021-08-06

### New
- audio_record can now stream output to a TCP port.
- Notebooks to demonstrate TCP streaming of output

### Changes
- alpha is preserved across band switches
- band switches are now even faster
- audio_record just takes 0 or 1, instead of 0,1, or 2.  

### Bug Fixes
- Fixed factor of two bug in the 11-band WDRC
## [1.5.3] - 2021-07-13
### New
- 11-band calibration
- add freping and adaptivefilter python api
### Bug Fixes
- fix min_phase and aligned parameter parsing
- fix afc initialization problem
- fix serious 11-band issue
- fix issues with AFC when playing audio from a file
## [1.5.2] - 2021-06-24

- add get_media command to get OSP_MEDIA and OSP_REC paths
- osp_cli uses 'rich' module and get_media.
- example notebooks to demonstrate controlling osp

## [1.5.1] - 2021-06-16

- add audio_alpha
## [1.5.0] - 2021-06-15

- FIR filters and Resamplers are restructured and optimized for ARMv8.
- WDRC11 bugs fixed.

## [1.4.2] - 2021-06-02

- audio_play=2 sets alpha to .5, plays file, then restores alpha
## [1.4.1] - 2021-06-02

- Add "record" command to osp_cli
- Improved history in osp_cli
- 11-band aligned by default
## [1.4.0] - 2021-06-01

- 11-Band Multirate WDRC with Half-Band polyphase resamplers
## [1.3.1] - 2021-05-12

- Better JSON error handling.
## [1.3.0] - 2021-05-07

- Added 11-Band WDRC
- Band switches no longer restart everything
- Mono wav files play correctly
- Python bindings for Wdrc11, FirFilter, ElevenBand, Hilbert
- Parameters are saved when bands are switched

## [1.2.1] - 2021-02-18

- Fixed occassional crash on recording stop.
## [1.2.0] - 2021-02-17
### Added
- Audio recording
- osp_convert utility to convert recording dumps to WAV
## [1.1.3] - 2021-02-15

- File playing performance improvements
- Adds latency_test. Optional benchmark for device testing.
## [1.1.2] - 2021-01-24

- Improve performance on hyperthreaded CPUs
## [1.1.1] - 2020-12-17

### Fixed
- Multithread restarts without handing
- Restarted threads have correct priority

### Changed
- AFC off by default
- Microphone input on by default (alpha=0)
- Set affinity to even numbered CPU cores if possible (not on PCD)
## [1.1.0] - 2020-12-14

### Added
- osp_cli can list audio file with "play ?"
- "restart" command restarts osp

### Fixed
- Fixes pops when enabling freping
- Switching bands or restarting now works with multithread
### Changed
- switching bands restarts osp

### Removed
- "audio" toggle removed.  Use gain instead.

## [1.0.10] - 2020-12-06

- Increase delay during band switch to avoid race condition..
## [1.0.9] - 2020-12-06

- Fixes 1.0.8 change that broke 10-band switch.
## [1.0.8] - 2020-12-04

- Checks that array sizes match the number of bands.
- Passing a float for a band array sets all bands to that value.

## [1.0.7] - 2020-11-23

- Exits with code -1 when audio stream fails.

## [1.0.6] - 2020-11-06

- Mac compilation fix to find portaudio.h

## [1.0.5] - 2020-11-05

- fix crash with bad audio files

## [1.0.4] - 2020-10-30

- improved signal handling and cleanup

## [1.0.3] - 2020-10-30

- works on Mac now.

## [1.0.2] - 2020-10-29

### Added
- hostname arg for osp_cli

### Changed
- better handling of failed audio streams

## [1.0.1] - 2020-10-21

### Removed
- no longer requires any zmq libs

### Added
- new parameter "audio_playing"

## [1.0.0] - 2020-10-21

### Added
- Rewrite osp and move from osp-clion-cxx
- osp_cli

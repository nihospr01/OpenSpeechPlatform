
## Architecture

![](images/HighLevel.png)  

This document describes the green box in the image above.
Inside the green box, we have the following.
![](images/Framework.png)  


The osp process
initializes the audio inputs and outputs and sets up the audio pipeline
so that incoming audio packets are sent to the various audio processing
algorithms.  It also parses the incoming requests from the EWS (Embedded Web Server) and sets or gets algorithm parameters as requested.  

The RT-MHA engine algorithms are implemented in `libosp/OSP`.


## RTMHA Parameters

Parameters are set by sending a JSON string to the osp process. The parameters are set for the left and right channels.  Some parameters, such as beamforming apply to both channels.  In this case, the right channge is
ignored.

Examples:

```json
{"method": "set", "data": {"left": {"alpha": 1}, "right": {"alpha": 1}}}
{"method": "set", "data": {"left": {"audio_filename": "audio/MPEG_es01_s.wav", "audio_play": 1, "audio_alpha": 1}}}
```

### Global

Name | Default | Description
---|---|---
alpha|0.0|The mixer of virtual sound (file playback) and actual sound (from the mics) where 0 is complete actual sound and 1 is complete virtual sound
en_ha|1|Enable hearing aid algorithm. 0=off, 1=on
gain|0.0|The gain in dB
global_mpo|120.0|Set the global mpo limit

### AFC (Adaptive Feedback Cancellation)

Name | Default | Description
---|---|---
afc|0|Enable Adaptive Feedback Cancellation. 0=off, 1=on
afc_delay|4.6875|Loopback delay in millisecond to adjust for device to device variation in feedback path.
afc_mu|0.005|Adjust the step size for feedback management
afc_reset|0|Reset the taps of AFC filter to default values. A signal, not a state!
afc_rho|0.9|Adjust the forgetting factor for feedback management
afc_type|3| Adaptation type for AFC. 0: Stop adaptation, 1: FXLMS, 2: IPNLMS, 3: SLMS

### Audio Playback and Recording

Name | Default | Description
---|---|---
audio_alpha|0.0|Temporarily set the alpha value for the next file played.  Alpha is restored to previous level when play is finished
audio_filename|""|Audio file to play
audio_play|0|1: Play audio, 0: stop.
audio_repeat|0|Repeat the audio file
audio_rfile|""|Record file name or stream.  Use "tcp://{ip_addr}:{port}" to stream output to a TCP server. Example: "tcp://127.0.0.1:5001"
audio_record|0|0: stops recording, 1: records outputs

Filenames for playing and recording are relative to some environment variables.

**OSP_MEDIA** must be set to the directory for the files to play.

**OSP_REC** ooptionally can be a writeable directory for recordings.  If it is not set
**OSP_MEDIA** will be used.

Recordings are a dump of 48000 Hz samples with 32-bit floats alternating left and right.
The *osp_convert* utility will convert the files to proper WAV files.

### Beamforming

Name | Default | Description
---|---|---
amc_forgetting_factor|0.8|Adjust the forgetting factor of the power estimate for GSC beamforming [0-1]  
amc_thr|2.0|Adjust the threshold of the AMC for GSC beamforming [0 to 10]
bf|0|Enable Beamformer. 0=off, 1=on
bf_alpha|0.0| A number between -1 to 1 for different degrees of sparsity in IPNLMS-l_0
bf_amc_on_off|0|Enable adaptation mode control beamformer. 0=off, 1=on
bf_beta|150.0|A number between 0 to 500 for different degrees of sparsity in IPNLMS-l_0
bf_c|0.001|A small positive number for preventing stagnation in SLMS
bf_delta|1e-6| A small positive number to prevent dividing zero
bf_mu|0.01|Adjust the step size for GSC beamforming (Help has wrong default)
bf_nc_on_off|0| Enable norm-constrained beamformer: 0=off, 1=on
bf_imu_on_off|0| Enable toggling beamforming using IMU gestures (ie. double-tap): 0=off, 1=on
bf_p|1.3|A number between 1.0 to 2.0 for fitting different degrees of sparsity in SLMS
bf_rho|0.985|Adjust the forgetting factor for beamforming [0-1]
bf_type|3|Adaptation type for GSC beamforming. 0: Stop adaptation, 1: Modified LMS, 2: IPNLMS, 3: SLMS
nc_thr|1.0|Adjust the threshold of the norm-constrained adaptation for GSC beamforming

### Beamforming Constants

These may be set in the default config file for osp, but cannot be modified after
osp is started.

Name | Default | Description
---|---|---
bf_fir_length|319| The number of filter taps of adaptive filter in beamformer
bf_power_estimate|0.0|Adjust the power estimate for beamforming [remove.  make constant 0]
bf_delay_len|160|The length of delay line in samples for beamformer

### Freping

Name | Default | Description
---|---|---
freping|0|Enable freping: 0=off, 1=on
freping_alpha [num_bands]| 0.0 | Set the alpha parameter for freping [-0.02 to 0.02 ]

### Wide Dynamic Range Compression (WDRC)

Name | Default | Description
---|---|---
attack [num_bands]|5.0|Set the attack time for WDRC for the bands
g50 [num_bands]|0.0|Set the gain values at 50 dB SPL input level
g80 [num_bands]|0.0|Set the gain values at 80 dB SPL input level
knee_low [num_bands]|45.0|Set the lower knee points for the bands
mpo_band [num_bands]|120.0|Set the MPO (upper knee points) for the bands
release [num_bands]|20.0|Set the release time for WDRC for the bands
aligned|1|Enable 10-band filterbank alignment. Aligns the delays in each subband.


### Speech Enhancement

6 and 10 Band only

Name | Default | Description
---|---|---
noise_estimation_type|0|Noise Estimation type: 0=Disable; 1=Arslan; 2=Hirsch and Ehrlicher; 3=Cohen and Berdugo
spectral_subtraction|0.0|Amount to subtract between range: [0,1)
spectral_type|0|Enable spectral subtraction: 0=Disable, 1=Enable

## RTMHA Commands

These are not channel parameters and are instead sent like this:

```json
{"method": "set", "data": {"num_bands": 10}}
{"method": "set", "data": {"restart": 1}}
{"method": "get", "data": {"restart": 1}}
{"method": "get_media"}
```


Name | Default | Description
---|---|---
num_bands|6| 6, or 10 band. When switching bands, "alpha" is preserved, otherwise the current (or default) parameters for the band are loaded.
restart|1|Tells osp (RT_MHA) to restart.  May take a few seconds.
get||Gets the current parameter state
get_media|| Gets the paths to the media files.
---

### Command Line Options

```
> ./release/src/osp -h
OSP (RT-MHA) Version 0.9-9-g57eeead

 Usage: ./release/src/osp [OPTIONS]
  Options:
   -c --conf_file filename   Read configuration from the file
   -l --log_file  filename   Write logs to the file
   -d --daemon               Daemonize this application
   -p --pid_file  filename   PID file used by daemonized app
   -i --input_device num     input device number
   -o --output_device num    output device number
   -I --input_channels num   number of input channels
   -O --output_channels num  number of output channels
   -m --multithreaded bool   multithreaded
```

The `-c` option allows for setting the initial state of the system.  

`-d` runs osp in the background, detached from any terminal.  You should use `-l` with it to specify a file for the output.  

---

![](images/Basic.png)  

![](images/Advanced.png)  

![](images/Threads.png)  


Currently, the thread priorities are set in several places.  

osp_process() sets SCHED_FIFO and the current priority level. *Update: this has been removed*

Then openspeechplatform() starts the console thread with that scheduler and priority.  *Update: it now uses the default priority*

Then it starts the EWS thread, also at that priority.

Then it sets the scheduler to FIFO and sets a new priority. And calls portaudio_wrapper() which calls PaAlsa_EnableRealtimeScheduling() (which does not seem to have any effect) and starts some audio threads (named "threaded-ml").


![](images/410c.png)  


CPU affinity on the 410c is very simple.  The OS and non-realtime tasks are set to use CPU 0 and the realtime audio runs on CPUs 1-3.  It may be necessary to modify this if additional load is placed on CPU 0 (such as streaming video or other high-bancwidth information).

For other hardware, the current CPU affinity code is not as appropriate.  Maybe some redesign is necessary here to accomodate other architectures.  For example, in hyperthreaded (SMT) CPUs, cpus 0 and 1 are the same physical core, as are cpus 2 and 3.  Testing indicates that the osp process runs more reliably if hyperthreading is disabled or affinity is set to not use the same physical cores.

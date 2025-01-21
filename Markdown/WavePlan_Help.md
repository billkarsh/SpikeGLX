## Wave Planner and WavePlayer

### Overview

SpikeGLX contains easy to use tools for stimulus waveform output.

* Use the SpikeGLX 'Wave Planner' dialog under the 'Windows' menu,
to create 'wave' files that you can then load, trigger and control
using our remote scripting API.

* Wave files can be executed using either OneBox, or NI devices with
waveform capable analog output features.

### Waves

A 'wave' is a pair of descriptor files:

1. A **mywave.meta** file is in human readable 'ini' format, just like a
SpikeGLX run.meta file. It's a handful of plain text 'keyword=value' pairs.
Here's an example:

```
[WaveMeta]
sample_frequency_Hz_dbl=10000
wave_Vpp_dbl=2
device_Vpp_dbl=5.0
data_type_txt_i16_f32=txt
num_samples_i32=0
```

2. The corresponding data file can have three formats, selected by the
**data_type_txt_i16_f32** meta item:

a. 'txt' data type: file **mywave.txt** contains a plain text script that generates the samples.
b. 'i16' data type: file **mywave.bin** contains the numerical samples as 16-bit signed integers.
c. 'f32' data type: file **mywave.bin** contains the numerical samples as floats.

**IMPORTANT**:

* All wave descriptors should live in the `_Waves` subfolder within the
SpikeGLX application folder.

* API functions that take a wave name should be given just the wave base-name
itself, stripped of any path or file extension. For example, if your metafile
full path was 'C:/SpikeGLX_stuff/SpikeGLX/_Waves/mysinwave.meta', then you
should refer to this simply as 'mysinwave'.

### Binary Wave Files

The binary formats are provided to allow maximum flexibility to
generate samples using your own external tools. Valid binary data
follow these rules:

* num_samples_i32 should be set to the accurate count of samples in the *.bin file.
* num_samples_i32 should be an even number.
* num_samples_i32 should not exceed 16777214.
* i16 data should be in the range [-32768,32767]. Upon loading, your values
will be multiplied by (wave_Vpp_dbl / device_Vpp_dbl).
* f32 data should be in the range [-1.0,1.0]. Upon loading, your values
will be multiplied by (32767 * wave_Vpp_dbl / device_Vpp_dbl).

> The scaled values are forced to range [-32768,32767].

> Although you must generate the *.bin file and its samples yourself, you
can use the SpikeGLX Wave Planner dialog to edit the frequency and voltage
scales in the *.meta file.

### Text Wave Syntax

Use the Wave Planner 'Data' text box to write your generator script.

Our wave mini-language uses four commands that can be strung together
in any combination:

* level( V, t_ms ) : Generate samples of amplitude V (float) in range [-1.0,1.0]
for time span t_ms (float) milliseconds.

* ramp( V1, V2, t_ms ) : Generate samples with linearly ramping amplitude
from V1 to V2 (floats) each in range [-1.0,1.0] for time span t_ms
(float) milliseconds.

* sin( A, B, f_Hz, t_ms ) : Generate a sine wave with amplitude A (float),
baseline B (float), where (A + B) is in range [-1.0,1.0]. The sine wave
has frequency f_Hz (float) and total time span t_ms (float) milliseconds.

* do N { } : Repeat whatever is included in curly braces N (integer) times.
Nesting of do-loops is permitted.

> White-space (returns, spaces, tabs) are completely ignored. Use space
freely to make a script readable.

> At load-time your script is compiled into scaled samples; scale factor:
(32767 * wave_Vpp_dbl / device_Vpp_dbl).

**Example**

```
do 10 {
    level(     0,   50 )
    ramp( 0,   0.5, 10 )
    level(     0.5, 100 )
    ramp( 0.5, 0,   10 )
}
```


_fin_


## Wave Planner and WavePlayer

**Topics**:

* [Overview](#overview)
* [Waves](#waves)
* [Binary Wave Files](#binary-wave-files)
* [Text Wave Syntax](#text-wave-syntax)
* [Remote Script Software Start](#remote-script-software-start)
* [Remote Script Triggered Start](#remote-script-triggered-start)

### Overview

SpikeGLX contains easy to use tools for stimulus waveform output.

* Use the SpikeGLX 'Wave Planner' dialog under the 'Windows' menu,
to create 'wave' files that you can then load, trigger and control
using our remote scripting API.

* Wave files can be executed using either OneBox, or NI devices with
waveform capable analog output features.

--------

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

--------

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

--------

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

--------

### Remote Script Software Start

MATLAB script 'wp_soft_start':

- Plays wave 'jwave' at OneBox channel AO-0.
- Playback is triggered by software command.

```
function wp_soft_start

    % handle to SpikeGLX
    hSGL = SpikeGL( '127.0.0.1' );

    % For demo purposes we assume the OneBox is not recording...
    % So we refer to it using its slot index
    ip = -1;
    slot = 21;

    % Load the wave plan
    wave_name = 'jwave';
    OBX_Wave_Load( hSGL, ip, slot, wave_name );

    % Select software triggering and infinite looping
    trigger = -2;
    looping = 1;
    OBX_Wave_Arm( hSGL, ip, slot, trigger, looping );

    % Start playback now, output is always at AO-0
    start = 1;
    OBX_Wave_StartStop( hSGL, ip, slot, start );

    % This section demonstrates a method to capture your
    % wave plan in action. The best pause parameters will
    % depend upon the duration of your wave plan and how
    % fast your SpikeGLX graphs are sweeping
    %
    % Let this play for 1 second
    % Then pause the SpikeGLX Graphs Window for 2 seconds
    % Then resume Graphs for 5 seconds
    pause( 1.0 );
    PauseGraphs( hSGL, 1 );
    pause( 2.0 );
    PauseGraphs( hSGL, 0 );
    pause( 5.0 );

    % Stop playback
    start = 0;
    OBX_Wave_StartStop( hSGL, ip, slot, start );

end % wp_soft_start
```

--------

### Remote Script Triggered Start

MATLAB script 'wp_trig_start':

- Plays wave 'jwave' at OneBox channel AO-0.
- Playback is triggered when OneBox channel AI-1 goes high.
- User needs to list AI-1 in the XA box of OBX Setup tab.
- We will configure NI device to send TTL rising edge from line-4.
- User needs to wire NI line-4 to OneBox AI-1.

```
function wp_trig_start

    % handle to SpikeGLX
    hSGL = SpikeGL( '127.0.0.1' );

    % OneBox assumed to be recording stream ip=0...
    % So the slot number is ignored in this case
    ip = 0;
    slot = -1;

    % Load the wave plan
    wave_name = 'jwave';
    OBX_Wave_Load( hSGL, ip, slot, wave_name );

    % Select AI-1 triggering and no looping
    trigger = 1;
    looping = 0;
    OBX_Wave_Arm( hSGL, ip, slot, trigger, looping );

    % Configure NI line-4
    digOutTerm = '/PXI1Slot6_2/line4';
    digBits = 0x10; % binary 1 at bit-4, zero elsewhere
    NI_DO_Set( hSGL, digOutTerm, 0 );

    % Start playback now, output is always at AO-0
    NI_DO_Set( hSGL, digOutTerm, digBits );

    % This section demonstrates a method to capture your
    % wave plan in action. The best pause parameters will
    % depend upon the duration of your wave plan and how
    % fast your SpikeGLX graphs are sweeping
    %
    % Let this play for 1 second
    % Then pause the SpikeGLX Graphs Window for 2 seconds
    % Then resume Graphs
    pause( 1.0 );
    PauseGraphs( hSGL, 1 );
    pause( 2.0 );
    PauseGraphs( hSGL, 0 );

    % Stop playback
    start = 0;
    OBX_Wave_StartStop( hSGL, ip, slot, start );

end % wp_trig_start
```


_fin_


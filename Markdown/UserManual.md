# SpikeGLX User Manual

**Topics:**

* [Overview](#overview)
    + [Screen Saver and Power Settings](#screen-saver-and-power-settings)
    + [Installation and Setup](#installation-and-setup)
        + [Calibration Data](#calibration-data)
        + [Remote Command Servers](#remote-command-servers)
        + [Data Directory](#data-directory)
        + [Multidrive Run Splitting](#multidrive-run-splitting)
    + [Data Stream](#data-stream)
    + [Supported Streams](#supported-streams)
        + [Stream Length](#stream-length)
    + [Channel Naming and Ordering](#channel-naming-and-ordering)
    + [Shank Map](#shank-map)
    + [Channel Map](#channel-map)
    + [Save Channel Subset](#save-channel-subset)
    + [Output File Format and Tools](#output-file-format-and-tools)
    + [Synchronization](#synchronization)
        + [Procedure to Calibrate Sample Rates](#procedure-to-calibrate-sample-rates)
        + [Running Without a Generator](#running-without-a-generator)
        + [Running With a Squarewave Generator](#running-with-a-squarewave-generator)
        + [Updating the Calibration](#updating-the-calibration)
    + [Gates and Triggers](#gates-and-triggers)
* [Console Window](#console-window)
* [Run Metrics Window](#run-metrics-window)
* [Configure Acquisition Dialog](#configure-acquisition-dialog)
* [Graphs Window Tools](#graphs-window-tools)
* [Offline File Viewer](#offline-file-viewer)
* [Checksum Tools](#checksum-tools)

**Appendix:**

* [SpikeGLX FAQ](SpikeGLX_FAQ.html)
* [Metadata Tags](Metadata_30.html)

--------

## Overview

### Screen Saver and Power Settings

The following settings guard against interruption during prolonged
data acquisition runs (running on batteries is discouraged):

Screen saver settings group:

* Screen saver: (None).

> Note 1: The screen saver settings are a control panel and you can get there
by typing 'screen saver' into a control panel search box. Screen saver is
a program that draws entertaining animation on your otherwise black screen.
The running of this class of programs disrupts acquisition. Turn that off.

> Note 2: In the power settings you can set the time until the screen turns
off. This is a safe option. It shouldn't affect anything except that you
may have to log in again after the screen blanks.

Power plan settings:

> Keep drilling down until you find the following advanced power plan options:

* Put the computer to sleep: Never.
* Hard disk/Turn off hard disk after: Never.
* Sleep/Sleep after: Never.
* Sleep/Allow wake timers: Disable.
* USB settings/USB selective suspend setting: Disable.
* Intel(R) Graphics Settings/Intel(R) Graphics Power Plan: Maximum Performance.
* PCI Express/Link State Power Management: Off.
* Processor power management/Minimum processor state: 100%.
* Processor power management/System cooling policy: Active.
* Processor power management/Maximum processor state: 100%.

> Tip: For some settings, 'Never' might not appear as a choice. Try typing
either 'never' or '0' directly into the box.

--------

### Installation and Setup

To install SpikeGLX on a new system, just copy a virgin SpikeGLX folder
to your C-drive and double click SpikeGLX.exe to begin.

The contents of a virgin (see below) SpikeGLX folder:

```
SpikeGLX/
    _Calibration/
    _Help/
    bearer/
    iconengines/
    imageformats/
    platforms/
    styles/
    translations/
    D3Dcompiler_47.dll
    FpgaManager.dll
    FTD3XX.dll
    libEGL.dll
    libgcc_s_seh-1.dll
    libGLESV2.dll
    libstdc++-6.dll
    libwinpthread-1.dll
    NeuropixAPI_version_info.dll
    opengl32sw.dll
    Qt5Core.dll
    Qt5Gui.dll
    Qt5Network.dll
    Qt5OpenGL.dll
    Qt5Svg.dll
    Qt5Widgets.dll
    SpikeGLX.exe
    SpikeGLX_NISIM.exe
    vcruntime(version).dll
```

>Virgin: The SpikeGLX folder does not contain a `_Configs` subfolder.

#### _Configs Folder

There are no hidden Registry settings or other components placed into your
system folders. Your personal preferences and settings will be stored in
`SpikeGLX/_Configs`.

>If you give the software to someone else (please do), delete the _Configs
folder because several settings in there are machine-dependent.

The _Configs folder is automatically created (as needed) when SpikeGLX
is launched.

>Tip: As you work with SpikeGLX you'll create several of your own custom
files to remember preferred settings {channel mappings, Imec readout tables, ...}.
**Resist the urge to store these in the SpikeGLX folder**. If you want to
upgrade, and, **we will add cool features over time**, the clutter will
make it much harder to figure out what you have to replace.

#### Calibration Data

Each imec probe has a **folder**, _labeled by probe serial number_, containing
its associated calibration data files. For example, NP 1.0 probes need gain
and ADC files, while NP 2.0 probes need only gain files. Other probe types
may differ. In all cases, place each probe's **whole calibration folder**
into the SpikeGLX `_Calibration` subfolder.

SpikeGLX reads an EEPROM chip on the probe to obtain its serial and model
number. The serial number is used to look up the matching calibration
folder name.

Some older NP 2.0 headstages do not have EEPROM chips. For those headstages
a dialog will pop up when you click `Detect` so that you can manually enter
those headstage serial numbers (find them on the tags/stickers on the
headstage). This allows manual association of the headstages with their
calibration folders. If you run again with the identical collection of
parts the dialog will fill in your previous serial numbers as a convenience.

When you click `Detect` the Imec box may display a yellow warning icon and
the text `Cal Issue`. This means either that, on the `IM Setup` tab you have
selected the run-time calibration policy `Skip all calibration`, or, for at
least one of the probes, the calibration folder could not be found. In other
words, this run will be performed without applying calibration files to one
or more of the probes.

By the way, the `_Calibration` subfolder also contains supplementary
SpikeGLX data:

* The results of imec headstage sample rate calibration.
* The results of NI device sample rate calibration.
* Your settings choices for each probe.
* Your settings choices for each Onebox.

>**Do keep (transplant) the `_Calibration` subfolder when you upgrade
SpikeGLX!**

#### Remote Command Servers

SpikeGLX contains two TCP/IP command servers:

* A general purpose **Remote Command** server that is accessed by our
provided command APIs: {MATLAB-SDK, CPP-SDK}.

* A legacy **Gate/Trigger** server that supports an early stimulation
application called StimGL. The server is retained for backward compatibility
but its gate/trigger operations have been added into the general server
via the `TriggerGT` API command.

Upon first launch SpikeGLX configures both servers with a local-host
(loopback) address. The severs are initially disabled for security.
To enable scriptability use the server settings dialogs under the `Options`
menu. Click `My Address` to set an appropriate interface (IP address).
We recommend keeping the default port and timeout values.

>Note: If your SpikeGLX address was assigned by a DNS service, it might
change if other machines are added or removed on the network. Just click
`My Address` again to read the updated value.

#### Data Directory

On first startup, the software will automatically create a directory called
`C:/SGL_DATA` as a default output file storage location. Of course, the
C:/ drive is the worst possible choice, but it's the only drive we know
you have. Please use menu item `Options/Choose Data Directory` to select
an appropriate folder on your data drive.

You can store your data files anywhere you want. The menu item is a
convenient way to "set it and forget it" for those who keep everything
in one place. Alternatively, each time you configure a run you can revisit
this choice on the `Save tab` of the `Configure Acquisition` dialog.

#### Multidrive Run Splitting

If you do very long recordings or use many probes you can distribute the
data streams across multiple drives like this:

1. Select `Options/Choose Data Directory`.
2. Check the `Multidrive` box.
3. Add additional paths to the directory table.

The result is as follows:

- There are N directories total = main + those in table.
- Logs, Onebox and NI data are always written to the main directory (dir-0).
- Output for logical probe-j is written to `dir-(j mod N)`.

>The mod operation is just the remainder when dividing j by N. For example,
7 mod 3 = 1.

--------

### Data Stream

The following technical background will help you understand and configure
your system, and help explain data storage formats. A key concept is the
`data stream` which has several parts:

![Stream Components](Stream.png)

On the **input** side, stream-specific hardware acquires data at its own
characteristic sample rate and feeds that into a long stream buffer
(FIFO queue). This happens in a `reader thread`.

The enqueued data are then available to other **output** threads:

* The trigger module scans the stream for conditions you've specified and
then opens, writes and closes files accordingly.
* The graph fetcher pulls recent data from the stream and pushes it into
that stream's custom viewer.
* The Audio module fetches recent data for the sound driver.
* The Command server fetches data from the stream on behalf of remote
applications.

>Notes:
>
>**Each stream** gets its **own metadata and binary data files**.
>
>The file saving **Trigger unit is shared** between streams so that data
>files are better time synchronized.
>
>Likewise, the **GraphFetcher is shared** to facilitate synchronous data viewing.
>
>_More cores allow better load balancing among these activities._

--------

### Supported Streams

SpikeGLX supports multiple concurrent data streams that you can enable
independently each time you run:

* `imec0`: Imec probe-0 data operating over PXIe or USB.
* `imec1`: Imec probe-1 data operating over PXIe or USB.
* ... : And so on. Up to 4 probes per PXIe module, 2 probes per USB Onebox.
* `obx0`: Imec Onebox-0 analog and digital data operating over USB.
* `obx1`: Imec Onebox-1 analog and digital data operating over USB.
* ... : And so on.
* `nidq`: Whisper/NI-DAQ acquisition from PXIe, PCI or USB devices.

Imec probes currently read out 384 channels of neural data and have 8 bits
of status data (stored as a 16-bit SY word).  Bit #0 signals that custom user
FPGA code running on the Enclustra has detected an interesting neural
event (NOT YET IMPLEMENTED). Bit #6 is the sync waveform, the other bits
are error flags. Each probe is its own stream.

Imec Oneboxes are compact and inexpensive alternatives to PXI chassis. Each
Onebox connects via USB. A box has two ports for neural headstages. The probes
you plug into a Onebox are treated as additional imecj data streams, as if
those probes were plugged into PXI modules. Oneboxes can also read up to
12 analog channels and those channels can be thresholded, making 12 pseudo
digital channels. These nonneural inputs are referred to as Onebox streams,
with labels `obx0`, `obx1`, and so on.

An Nidq device (M, X or S-series, digital, a.k.a. 62xx, 63xx, 61xx, 65xx)
can be used to record auxiliary, usually non-neural, experiment signals.
These devices offer several analog and digital channels. You can actually
use two such devices if needed.

The Whisper system is a 32X multiplexer add-on that plugs into an NI device,
giving you 256 input channels. Whisper requires S-series devices (61xx).

#### Stream Length

To allow fetching of peri-event context data the streams are sized to hold
the smaller of {8 seconds of data, 40% of your available RAM}. We always
generate a warning message with the length, like this:
*"Stream length limited to 8 seconds."* Making it a warning gives it a
highlight color in the logs so you'll take notice of it.

--------

### Channel Naming and Ordering

#### Imec Channels

Each Imec stream acquires up to **three distinct types** of channels:

```
1. AP = 16-bit action potential channels
2. LF = 16-bit local field potential channels (some probes)
3. SY = The single 16-bit sync input channel (sync is bit #6)
```

All probes read out 384 AP and a single SY channel. Some probes read out a
separate LF band with the same channel count as the AP band.

Throughout the software the channels are maintained in `acquisition order`.
That is, each acquired **sample** (or **timepoint**) contains all 384 AP
channels, followed by the 384 LF channels (if present), followed by the
SY channel.

The channels all have names with two (zero-based) indices, like this:

```
AP0;0 .. AP383;383 | LF0;384 .. LF383;767 | SY0;768
```

For example, LF1;385 tells you:
- This is an LF channel
- It's the second channel in the LF group
- It's the 386th channel overall

The second "overall" index (after the semicolon) is the index you
should use for all GUI functions that select channels. For example:

* Which channel to observe in a TTL trigger.
* Which channel to send to audio output.
* Which channels to selectively save.

**Imec Data Files Are Split**

In memory, the LF channels are upsampled to 30kHz for symmetry with the
AP channels. However, for better disk efficiency the AP and LF data are
written out separately and the LF data have their natural sampling rate
of 2.5kHz.

If you elected to save all channels `YourFile.imec0.ap.bin` would contain:

```
AP0;0 .. AP383;383 | SY0;768
```

and `YourFile.imec0.lf.bin` would contain:

```
LF0;384 .. LF383;767 | SY0;768
```

Note that the sync channel is duplicated into both files for alignment in
your offline analyses. Note, too, that each binary file has a partner meta
file.

#### Onebox Channels

Each Onebox stream acquires up to **three distinct types** of channels:

```
1. XA = 16-bit analog channels
2. XD = 16-bit packed digital lines
3. SY = The single 16-bit sync input channel (sync is bit #6)
```

You can specify up to 12 analog channels to read out.

If you click the XD checkbox, all 12 channels are thresholded at 0.5V and
read out as the low-12 bits of a single 16-bit word.

Throughout the software the channels are maintained in `acquisition order`.
That is, each acquired **sample** (or **timepoint**) contains the XA
channels (if present), followed by the XD channel (if present), followed by
the SY channel.

The channels all have names with two (zero-based) indices, like this:

```
XA0;0 .. XA11;11 | XD0;12 | SY0;13
```

For example, XD0;12 tells you:
- This is the XD channel
- It's the 0th channel in the XD group
- It's the twelfth channel overall

The second "overall" index (after the semicolon) is the index you
should use for all GUI functions that select channels. For example:

* Which channel to observe in a TTL trigger.
* Which channel to send to audio output.
* Which channels to selectively save.

#### NIDQ Channels

There are four categories of channels {MN, MA, XA, XD} and these are
acquired and stored in that order, though they may be acquired from
either one or two NI devices (named say, 'dev1 and 'dev2').

```
1. MN = dev1 multiplexed neural signed 16-bit channels
2. (likewise from dev2)
3. MA = dev1 multiplexed aux analog signed 16-bit channels
4. (likewise from dev2)
5. XA = dev1 non-muxed aux analog signed 16-bit channels
6. (likewise from dev2)
7. XD = dev1 non-muxed aux digital unsigned 16-bit words
8. (likewise from dev2)
```

>Notes:
>
> 1. Within a multiplexed subgroup, like MN or MA, all the channels connected
>	to a given multiplexer are grouped together. The names of the channels
>	acquired from neural muxer #2 are "MN2C0"..."MN2C31". Zero-based labeling
>	is used throughout.
>
> 2. If a second device is used, each MN, MA, ... category within the
>	central stream is seamlessly expanded as if there were a single
>	higher capacity device.
>
> 3. Channel names, e.g. "MA1C2;34" indicate both which channel this is
>   within its own category (here, the 3rd channel in group MA1) and,
>   which it is across all the channels in this stream (here, the 35th
>   channel in the stream). The latter index (34) is how you should refer
>   to this channel in save-strings, in trigger setups and for audio
>   out selection.
>
> 4. Up to 32 digital lines can be acquired from your main device (say, dev1)
>   and from a secondary device (say, dev2). The number of bytes needed to
>   hold dev1's lines depends on the highest numbered line. If the highest
>   named line is #31, then 32 bits are required, hence 4 bytes. If #14 is
>   the highest, then 16 bits, hence 2 bytes are used to store the data for
>   that device. Dev1 may need {0,1,2,3 or 4} bytes to hold its XD lines.
>   Dev2 is evaluated the same way, but independently. In the stream, all
>   the bytes for dev1 are together, followed by all those for dev2.
>
> 5. Trigger line numbering depends on bytes. Say XD1="0:4,22" and XD2="9."
>   Suppose you want to use line #9 on dev2 as a TTL trigger input. You
>   should specify bit #33, here's why: There are 6 bits used on dev1, but
>   the highest is #22, so three bytes are needed. Therefore, the offset to
>   the first bit (bit #0) on dev2 is 24. Add 9 to that to get 33.
>
> 6. The streams, hence, graphs and data files, always hold an integral
>   number of 16-bit fields. The bytes of digital data are likewise grouped
>   into 16-bit words. There are anywhere from 0..4 bytes (B1) of dev1
>   lines followed by 0..4 bytes (B2) for dev2. The count of 16-bit words
>   is `int(1 + B1 + B2)/2)`. That means, divide by 2 and truncate (round
>   down) to an integer.
>
> 7. The Graphs window depicts digital data words as groups of 16-lines.
>   The lowest line number in a group is at the bottom. In files the data
>   words have the lowest numbered lines in the lowest order bits.

--------

### Shank Map

A `shank map` is a table describing where each neural channel is on your
physical probe. This information is used for spatial channel averaging,
and for activity visualization. Both imec and nidq streams can be used
to record from probes, so can have associated shank maps.

A rudimentary tool is provided to create, edit and save nidq shank maps (and
shank map (.smp) files). However, the shank map is automatically derived
from the imro table for imec probes; imec shank maps are internal, they are
not edited or saved directly.

> If you do not supply a **nidq** map, a `default` layout is used for
MN channels. The default is a probe with 1 shank, 2 columns and a row count
equal to MN/2 (neural channel count / [2 columns]).

To make and use a custom nidq map you must save it in a file. The file format
looks like this:

```
1,2,480     // header: nShanks,nColsPerShank,nRowsPerShank
0 0 0 1     // entry: iShank <space> iCol <space> iRow <space> iUsed
0 1 0 1
0 0 1 1
0 1 1 1
...         // one entry per spiking acquisition channel
```

This universal layout scheme has a few simple rules:

* Number of shanks is [1..8].
* Each shank has (nCols x nRows) sites.
* Shank indices (iShank) start at 0 and advance left to right.
* Column indices (iCol) start at 0 and advance left to right.
* Row indices (iRow) start at 0 and advance tip to tail.
* Each site has a `used` index (Boolean 0 or 1) denoting inclusion in spatial averages.

> You can mark a site `used=0` if you know it is broken or disconnected. For
Imec probes, we automatically set `used=0` for reference sites and those you
have turned off (bad channels) in the `IM Setup tab`.

**Most importantly** a shank map is a mapping from an acquisition channel
to a probe location. So...while there can be more potential sites
(nShanks x nCols x nRows) than channels...

* **The number of table entries for niqd maps must equal MN.**

--------

### Channel Map

The Graphs window arranges the channels in the standard `acquisition` order
(AP, LF, SY), (XA, XD, SY) and (MN, MA, XA, XD) or in a `user` order that
you can specify using a `channel Map`. Each stream gets its own channel
map file.

A rudimentary tool is provided to create, edit and save channel maps (and
channel map (.cmp) files).

> If you do not supply a map, the `default` user order depends upon the
stream. The **imec** default order follows the imro table, ordered first
by shank, then going upward from tip to base. The **nidq** default is
acquisition channel ordered.

To make and use a custom map you must save it in a file. The file format
looks like this:

```
6,2,32,0,1   // header (type counts): MN,MA,C,XA,XD
MN0C0;0 256  // entry: channel-name;acq-index  <space>  sort-index
MN0C1;1 1
MN0C2;2 2
MN0C3;3 3
...
XD0;256 0    // this example makes the digital graph first
```

You can save and reuse channel map files in another run by loading that
file from the Channel Map dialog. However, this only makes sense if the
loaded map describes the same types and counts of channels as you've
configured in the current run, hence, the header values, which are counts
of channel types. The `C` value is the number of channels per muxer.

Editing the sort order simply consists of reordering the right-most column
of sort-index values which must be in the range [0..N-1], where N is the
total channel count. For digital data we don't count individual lines.
Rather we count 16-line blocks of channels.

> You can edit these files in any text editor if you prefer. You can
change channel name strings too (shh).

> You can also change the channel map from the Graphs window by right-clicking
on the graphs area and selecting `Edit Channel Order...`.

--------

### Save Channel Subset

Each hardware configuration tab determines which channels are acquired from
that hardware and held in the central data stream. All acquired channels
are shown in the Graphs window. However, you don't have to save all of the
channels to your disk files.

You can enter a print-page-range style string for the subset of channels
that you want to save. This string is composed of index numbers in the
range [0..N-1], where N is the total channel count. To save all channels
you can use the shorthand string `all`, or just `*`.

>Notes:
>
> 1. You can also change this list from the Graphs window by right-clicking
>   on the graphs area and selecting `Edit Saved Channels...`.
>
> 2. Remember that digital lines are grouped into 16-bit words which are
>   essentially pseudo-channels in your subset string. If you don't save
>   a given word of digital line data, several lines will be affected.

--------

### Output File Format and Tools

Output data files are always paired; a `.bin` and a matching `.meta` file.

* The `.bin` file is the binary data. There is no header. The data are
packed timepoints. Within each timepoint the 16-bit channels are packed
and ordered exactly as described above in the section
[Channel Naming and Ordering](#channel-naming-and-ordering). Note that a
timepoint is always a whole number of 16-bit words. There is one 16-bit
word per saved analog channel. At the rear of the timepoint are any saved
digital lines, bundled together as the bits of 16-bit words as described
in the notes above.

* The `.meta` data are text files in ".ini" file format. That is, every
line has the pattern `tag=value`. All of the meta data entries are described
in the document `Metadata.html` found at the top level of your release
software download.

>Each metadata file is written three times:
>
>1. When created.
>2. When `firstSample` is determined.
>3. When {`fileSHA1`, `fileTimeSecs`, `fileSizeBytes}` are determined.
>
> fileTimeSecs = (`fileSizeBytes`/2/`nSavedChans`) / `xxSampRate`, xx={im,ob,ni}.

>The SpikeGLX
[`Downloads Page`](https://billkarsh.github.io/SpikeGLX/#post-processing-tools)
has simple tools (MATLAB and python) demonstrating how to parse the binary
and metadata files.

--------

### Synchronization

Each stream has its own asynchronous hardware clock, hence, its own **start
time** and **sample rate**. The time at which an event occurs, for example a
spike or a TTL trigger, can be accurately mapped from one stream to another
if we can accurately measure the stream timing parameters. SpikeGLX has
several tools for that purpose:

* SpikeGLX: Clock rate calibration.
* SpikeGLX: Generate/acquire 1 Hz pulser signal.
* CatGT (offline tool): Extract event and pulser edge times.
* TPrime (offline tool): Align/map data using pulser edges.

#### Procedure to Calibrate Sample Rates

1) A pulse generator is configured to produce a square wave with period of
1 s and 50% duty cycle. You can provide your own source, or SpikeGLX can
program the NI-DAQ device to make this signal, or the imec devices can be
selected as the source.

2) You connect the output of the generator to one input channel of each
stream and name these channels in the `Sync tab` in the Configuration
dialog.

3) In the `Sync tab` you check the box to do a calibration run. This
will automatically acquire and analyze data appropriate to measuring
the sample rates of each enabled stream. These rates are stored in a
database (by device SN) for use in subsequent runs. The database is in
the `_Calibration` subfolder. Be sure to transplant this folder to the
new SpikeGLX folder when you upgrade the software.

>Full detail on the procedure is found in the help for the
Configuration dialog's [`Sync tab`](SyncTab_Help.html).

#### Running Without a Generator

You really should run the sample rate calibration procedure at least once
to have a reasonable idea of the actual sample rates of your specific
hardware. In our experience, the actual rate of an imec stream may be
30,000.60 Hz, whereas the advertised rate is 30 kHz. That's a difference
of 2160 samples or 72 msec of cumulative error per hour that is correctible
by doing this calibration.

The other required datum is the stream start time. SpikeGLX records the
wall time that each stream's hardware is commanded to begin acquiring
data. However, that doesn't account for the time it takes the command
to be transmitted to the device, to be decoded, to be responded to,
and for the first data to actually arrive at the device. This estimate
of the start time is only good to about 10 ms.

It is an option to do your data taking runs without a connected square
wave generator, and you might choose that if you only have one stream,
or if the sync hardware is malfunctioning for any reason. Under these
conditions runs will start off with time synchronization errors
of 5 to 10 ms (owing to T-zero error) and that error will slowly drift
depending upon how accurate the rate calibration is and whether the
stream has clock drift that isn't captured by a simple rate constant.
Thankfully, you can do much better than that...

#### Running With a Squarewave Generator

In this mode of operation, you've previously done a calibration run to
get good estimators of the rates, and you are dedicating a channel
in each stream to the common generator (pulser) during regular data runs.
Two things happen under these conditions:

1) When the run is starting up SpikeGLX uses the pulser to adjust the
estimated stream start times so they agree to within a millisecond.

2) During the run, the time coordinate of any event can be referenced
to the nearest pulser edge which is no more than one second away, and
that allows times to be mapped with sub-millisecond accuracy.

#### Updating the Calibration

Menu item: `Tools/Sample Rates From Run` lets you open any existing
run that was acquired with a connected generator and recalibrate the
rates for those streams. You can then elect to update the stated sample
rates within this run's metadata, and/or update the global settings
for use in the next run.

--------

## Gates and Triggers

File writing is governed by an event hierarchy:

**Run -> Gate -> Trigger -> File**

The following process works the same whether controlled manually via the
SpikeGLX GUI, or scripted using the remote MATLAB or C++ interface (API).

1. You configure experiment parameters, including:
    * Data directory (where output files will be stored),
    * Run name,
    * Gate mode,
    * Trigger mode.

<!-- -->

2. You start the run with the `Run` button in the `Configure` dialog:
    * Data acquisition devices begin collecting scans,
    * The Graphs window begins displaying streaming data.

3. Initially, the gate is low (closed, disabled) and no files can be written.
When the selected gate criterion is met the gate goes high (opens, enables),
the gate index `g` is set to zero and the trigger criteria are then evaluated.

4. Triggers are like mini programs that determine when to capture data to
files. There are several options you can read about [here](TrigTab_Help.html).
Triggers act only while the gate is high and are terminated if the gate goes
low. **Gates always override triggers**. Each time the gate goes high the
gate-index (g) is incremented and the t-index is reset to zero. The trigger
program is run again within the new gate window.

5. When the selected trigger condition is met, a new file is created. On
creation of the first file with a given `g` index, a new `run folder` is
created in the data directory to hold all the data for that gate. That is,
we create folder `data-path/run-name_gN`. Thus, the first file written for
the first trigger would be `data-path/run-name_g0/run-name_g0_t0.nidq.bin`.
When the trigger goes low the file is finalized/closed. If the selected
trigger is a repeating type and if the gate is still high then the next
trigger will begin file `data-path/run-name_g0/run-name_g0_t1.nidq.bin`,
and so on within gate zero. (For other data streams, the same naming rule
applies, with `nidq` replaced by `imec0.ap`, `imec0.lf` or `obx0.obx`).

6. If the gate is closed and then reopened, triggering resets and the
next folder/file will be named `data-path/run-name_g1/run-name_g1_t0.nidq.bin`,
and so on.

7. The run itself can be stopped manually via the GUI, or by remote command.

>Note that there is an option on the `Save Tab` called `Folder per probe`.
If this is set, there is still a run folder for each g-index `run-name_gN`.
However, inside that there is also a subfolder for each probe that contains
all the t-indices for that g-index and that probe. A probe subfolder is
named like `run-name_gN/run-name_gN_imecM`.

--------

## Console Window

The `Console` window contains the application's menu bar. The large text
field ("Log") is a running history of informative messages: errors, warnings,
current status, names of completed files, and so on. Of special note is
the status bar at the bottom edge of the window. During a run this shows
the current gate/trigger indices and the current file writing efficiency,
which is a key readout of system stability.

### Acquisition Performance

The Imec hardware buffers a small amount data per probe. A fast running
loop in SpikeGLX requests packets of probe data and marshals them into
the central stream. Every few seconds we read how full the hardware buffer
is. If it is more than 5% full we make a report in the console log like
this:

```
IMEC FIFO queue imec5 fill% 6.2
```

If the queue grows a little it's not a problem unless the percentage
exceeds 95%, at which point the run is automatically stopped.

### Disk Performance

During file writing the status bar displays a message like this:

```
FileQFill%=(ni:0.1,ob:0.0,im:0.0) MB/s=14.5 (14.2 req)
```

The streams each have an in-memory queue of data waiting to be spooled
to disk. The FileQFill% is how full each binary file queue is. The queues
may fill a little if you run other apps or copy data to/from the disk
during a run. That's not a problem as long as the percentage falls again
before hitting 95%, at which point the run is automatically stopped.

In addition, we show the overall current write speed and the minimum
speed **required** to keep up. The current write speed may fluctuate
a little but that's not a problem as long as the average is close to
the required value.

>**You are encouraged to keep this window parked where you can easily see
these very useful experiment readouts**.

### Tools

* Control report verbosity with menu item `Tools/Verbose Log`.
* Enable/disable log annotation (typing) with menu item `Tools/Edit Log`.
* Capture recent log entries to a file with menu item `Tools/Save Log File`.

--------

## Run Metrics Window

Choose menu item `Window/Run Metrics` to open a window that consolidates
the most vital health statistics from the Console log, and adds a few more:

* An overall run health summary LED.
* Stream error status flags.
* Stream FIFO filling level.
* Stream worker thread activity level.
* Stream disk writing performance.
* Stream data fetching performance.
* Errors and warnings culled from the Console log.

Click the `Help` button in the window to get a detailed description of
the metrics.

--------

## Configure Acquisition Dialog

**In brief: Configure and start a new run**.

Notes on the dialog as a whole:

* The flow of the dialog (through its tabs) mirrors the flow of data
through SpikeGLX:

**Hardware -> Memory -> Visualization -> Files**

```
   Devices tab: Select which streams/hardware to acquire.
        IM tab: Configure imec neural probe streams.
       Obx tab: Configure imec Onebox (ADC) streams.
        NI tab: Configure NI ADC streams.
      Sync tab: Cross-connect streams for precision alignment.
     Gates tab: Together with...
  Triggers tab: Define trials and how to write files.
      Save tab: Specify where to put files and how to name them.
```

* Validation (a.k.a. sanity checking) is always performed on all of the
settings on all of the tabs. General validated settings are stored in
`SpikeGLX/_Configs/daq.ini`. Probe and Onebox specific settings are stored
in the `SpikeGLX/_Calibration` folder.

* Press `Last Saved` to revert the entire dialog to the stored (validated)
values.

* Press `Verify | Save` to sanity-check the settings on all tabs, and if
valid, save them to disk **without** initiating a new run. This is
useful when trying to make the Configuration and Audio dialog settings agree
before starting a run, as audio settings are checked against the stored
values.

* Press `Run` to validate and save the settings and then start a new run.

* Press `Cancel` to end the dialog session without altering stored settings.

* You can resize this dialog (and most others) making it easier to see
big tables.

Detailed help for each tab is available here:

* [**Devices**](DevTab_Help.html)
* [**IM**](IMTab_Help.html)
* [**Obx**](OBXTab_Help.html)
* [**NI**](NITab_Help.html)
* [**Sync**](SyncTab_Help.html)
* [**Gates**](GateTab_Help.html)
* [**Triggers**](TrigTab_Help.html)
* [**Save**](SaveTab_Help.html)

--------

## Graphs Window Tools

### Second Graphs Window

In the Console window menus choose `Window/More Traces (Ctrl+T)` to open
a second Graphs window after a run has started. Only the main Graphs window
has run controls and LED indicators for gate and trigger status, but all
of the stream viewing and filtering options otherwise work the same in both
windows.

### Run Toolbar

* `Stop Acquisition`: Stops the current run and returns the software to an idle state.
You can do the same thing by clicking the `Graph Window's Close box` or by
pressing the `esc` key, or by choosing `Quit (Ctrl+Q)` from the File
menu (of course the latter also closes SpikeGLX).

* `Acquisition Clock`: The left-hand clock displays time elapsed since the run started
and first samples were read from the hardware.

* `Enable/Disable Recording`: This feature is available if you select it
on the `Gates tab`. Use this to
[pause/resume](GateTab_Help.html#gate-manual-override)
the saving of data files, to change
[which channels are being saved](#save-channel-subset)
to disk files, or to
[edit the name of the run](SaveTab_Help.html#run-name-and-run-continuation)
and its disk files.

* `Recording Clock`: The right-hand clock displays time elapsed since the current file
set was opened for recording.

* `Notes`: Edit your run notes. These are stored as `userNotes` in the
metadata.

* `Pause`: The Pause VCR-style button toggles between pause and play of
stream data in the graphs so you can inspect an interesting feature.
This does not pause any other activity.

### Stream Toolbar Controls

* `MN0C0;0`: The name of the currently selected graph. Single-click on
any graph to select it. The current graph is the target of several
controls such as the axis scaling boxes. Click on the channel-name
string in the toolbar to switch to the page containing the current selection.

* `Expand`: This button toggles between showing just the selected channel,
and the standard multi-channel view. You can also double-click a graph
to select it and expand/contract it.

* `Sec`: Enter a value in range [0.001..30.0] seconds to set the time span.

* `Yscl`: Enter a value in range [0.0..9999.0] to set a vertical magnification
for the selected graph.

* `Clr`: Click this button to get a Color picking dialog whereby you can
define an alternate color for the **data trace** in the selected graph.
This only works for analog channels; digital traces are auto-colored.

* `Apply All`: Copies Yscl from the selected graph to all other graphs
of the same category.

#### Filters Applied Only to Neural Channels

Notes:

* These filters only affect the appearance of graphs, not saved data.
* These filters are **not** applied to non-neural channels.
* If ever you are suspicious that hardware is not working, turn all the
filters off to understand what is coming out of the hardware.

##### General Filters

* `-<T>`: Time averaging. Samples the data stream per channel to calculate
and then subtract the time average value; effectively subtracting the DC
component. The value is updated every 5 seconds. This may create
artifactual steps during the initial settling phase of Imec preamps.

* `-<S>`: Spatial averaging. At each timepoint a neighborhood of electrodes
per channel is averaged; the result is subtracted from that channel. The
locations of electrodes are known from your imro table or nidq shank map.
    + Notes:
        1. Certain electrodes are omitted from the average: {Those marked
        'use=false' in your map, Imec reference electrodes, Imec electrodes that
        are turned off}.
        2. Only AP-band channels are affected.
        3. Neighborhoods never cross shank boundaries.
    + There are four choices of neighborhood:
        + `Loc 1,2`: Small annulus about the channel's electrode;
            e.g. inner radius=1, outer=2.
        + `Loc 2,8`: Larger annulus about the channel's electrode;
            e.g. inner radius=2, outer=8.
        + `Glb All`: All electrodes on probe.
        + `Glb Dmx`: All electrodes on probe that are sampled
            concurrently (same multiplexing phase).

* `BinMax`: If checked, we report the extrema in each neural channel
downsample bin. This assists spike visualization but exaggerates apparent
background noise. Uncheck the box to visualize noise more accurately.

##### Imec Stream Filters

The `imro` table determines individually for each AP channel if a 300 Hz
high pass filter is applied in hardware. The result is the `native` AP
signal. In addition, LF band signals are always acquired from the hardware.
The filter selector applies software operations to these `native` signals.

* `AP Native`: No software filter is applied.
* `300 - INF`: A software high pass filter is applied to all AP channels.
* `AP = AP+LF`: All AP channels are graphed as the simple sum of the AP
and corresponding LF band signal.

##### Nidq Stream Filters

* `Bandpass`: Applies optional bandpass filtering to neural MN
channels.

### Page Toolbar Controls

* `Acq/Usr Order`: This button toggles between acquired (standard)
channel order and that specified by your custom [channel map](#channel-map).

* `Shank View`: Opens the [Shank Viewer](ShankView_Help.html) window for
this stream.

* `NChan`: Specifies how many graphs to show per page. When the value is
changed SpikeGLX selects a page that keeps the middle graph visible.

* `1st`: Shows the index number of the first graph on the current page.

* `Slider`: Change pages.

### Right-Click on Graph

For either stream:

* `Select As L/R Audio Channel`: Listen to selected channel (immediately).

* `Edit Channel Order...`: Edit the ChanMap for this stream.

* `Edit Saved Channels...`: Edit the string describing which channels
are saved to file. Note that saved channels are marked with an `S` on the
right-hand sides of the graphs.

* `Color TTL Events...`: Watch up to 4 auxiliary (TTL) channels for
pulses. Apply color stripes to the graphs when pulses occur.

#### Imec Menu

* `Edit Channel On/Off...`: Shows editor for changing which channels are
turned off (available only if recording currently disabled).
All data and views are updated to reflect your changes.

### Other Graph Window Features

* Hover the mouse over a graph to view statistics for the data
currently shown.

--------

## Offline File Viewer

Choose `File::Open File Viewer` and then select a `*.bin` file to open.
As of version 20160701 you can open and view data files from any stream,
and you can link the files from a run so scrolling is synchronized between
multiple viewers.

In a viewer window, choose
[`Help::File Viewer Help`](FileViewer_Help.html)
for more details.

--------

## Checksum Tools

### SHA1 Checksum

Each .meta file stores the SHA1 checksum for the binary file in the field
`fileSHA1=`. Use menu item `Tools/Verify SHA1` to recalculate the current
value for any (.bin,.meta) pair and determine if either file may have been
corrupted. The SHA1 checksum, per se, does not provide any pathway to recovery.

### PAR2 Redundancy Tool

Of course, you can create a perfect backup of a file by simply copying it
whole, and that's the recommended thing to do provided you can afford the
storage space.

Alternatively, _**P**arity **AR**chive 2_ is a Usenet format for detecting and
correcting binary file corruption using only a fraction of the original
file's size. `(That fraction is called the redundancy percentage.)` The downside
is that the smaller the fraction you use for the backup set, the lower the
likelihood of being able to fully recover the original file.

To invoke the tool use menu item `Tools/PAR2 Redundancy Tool` to create a
backup set for a given data file. Subsequently, using the same tool, you
can use the backup set to verify the file and to attempt recovery in case
of corruption.


_fin_


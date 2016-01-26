# SpikeGLX User Manual

**Topics:**

* [Overview]
    + [System Requirements]
    + [Installation and Setup]
    + [Channel Ordering]
    + [FYI Architecture Notes]
* [Console Window]
* [Configure Acquisition Dialog]
* [NI Input -- Configuring NI-DAQ Devices]
    + [Sample Clocks -- Synchronizing Hardware]
    + [Input Channel Strings]
    + [MN, MA Gain]
    + [AI Range]
* [Gates -- Synchronizing Software]
    + [Run -> Gate -> Trigger]
    + [Gate Modes]
* [Triggers -- When to Write Output Files]
    + [Trigger Modes]
    + [Trigger Disable At Run Start]
    + [Trigger Pause and Resume]
* [See N' Save -- Focusing on Interesting Channels]
    + [Channel Map]
    + [Save Channel Subsets]
* [Graphs Window Tools]
* [Offline File Viewer]
* [Checksum Tools]

## Overview

### System Requirements

* Windows: XP SP3, 7, 8.1, 10.
* NI-DAQmx 9 or later (recommend latest version).
* Minimum of four cores.
* Minimum of 2.5 GHz.
* Minimum of 4 GB RAM.
* Dedicated second hard drive for data streaming.

SpikeGLX is multithreaded. More processors enable better workload
balancing with fewer bottlenecks. The OS, background tasks and most other
apps make heavy use of the C:/ drive. This is the worst destination for
high bandwidth data streaming. A second hard drive dedicated to data
streaming is strongly recommended. More cores and a separate drive are
by far the most important system specs. More RAM, clock speed, graphics
horsepower and so on are welcome but less critical.

### Installation and Setup

To install SpikeGLX on a new system, just copy a virgin SpikeGLX folder
to your C-drive and double click SpikeGLX.exe to begin.

> SpikeGLX is currently a 32-bit application. If you have any difficulty
launching it in 64-bit Windows, try:
>
> 1. Right-click on the application icon
> 2. Choose Properties
> 3. Compatibility Tab
> 4. Check : Run this program in compatibility mode for:
> 5. Select: Windows XP (Service Pack 3)

The contents of a virgin (see below) SpikeGLX folder:

```
SpikeGLX/
    platforms/
        qminimal.dll
        qwindows.dll
    icudt52.dll
    icuin52.dll
    icuuc52.dll
    libgcc_s_dw2-1.dll
    libstdc++-6.dll
    libwinpthread-1.dll
    Qt5Core.dll
    Qt5Gui.dll
    Qt5Network.dll
    Qt5OpenGL.dll
    Qt5Svg.dll
    Qt5Widgets.dll
    qt.conf
    SpikeGLX.exe
```

>Virgin: The SpikeGLX folder does not contain a `configs` subfolder.

#### Configs Folder

There are no hidden Registry settings or other components placed into your
system folders. Your personal preferences and settings will be stored in
`SpikeGLX/configs`. To back up your custom setup, just copy the configs folder
somewhere off the machine. The configs folder contains .ini style files
which are text files you can easily understand and edit if desired, though
there are GUI tools to do that for you.

>If you give the software to someone else (please do), delete the configs
folder because several settings in there are machine-dependent.

The configs folder is automatically created (as needed) when SpikeGLX
is launched.

#### Remote Command Servers

Upon first launch SpikeGLX determines the IP address of your machine
and configures the (internal) **Remote Command** server and the
**Gate/Trigger** server. You can change the default server settings using
items of those names under the `Options` menu.

>Note: For security, the Command server is disabled in a virgin setup.
You have to visit menu item `Options/Command Server Settings` to enable it.

#### Run Directory

On first startup, the software will automatically create a directory called
`C:/SGL_DATA` as a default **Run folder** (place to store data files).
Of course, the C:/ drive is the worst possible choice, but it's the only
drive we know you have. Please use menu item `Options/Choose Run Directory`
to select an appropriate folder on your data drive.

You can store your data files anywhere you want. The menu item is a
convenient way to "set it and forget it" for those who keep everything
in one place. Alternatively, each time you configure a run you can revisit
this choice on the `See N' Save` tab of the `Configure Acquisition` dialog.

### Channel Ordering

_These notes are essential to understanding:_

* How data are stored in the binary files
* How to configure the graph ordering
* How to refer to trigger channels in the GUI
* How to name subsets of channels to save.

Think about this software from the inside out. In the middle is a central
data stream: a FIFO queue holding 30 seconds of raw acquisition data.
At 25KHz sample rate, the stream holds 750K timepoints (a.k.a scans or samples).
All timepoints have a uniform **acquisition** channel order:

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
> 2. Digital channels are single bits and they are packed together into 16-bit
>	unsigned words. The first acquired digital channel occupies the lowest
>	order bit (bit#0) of the first word. Each subsequent digital channel
>	occupies the next bit position. Bits from dev2 are packed into the same
>	word if there is room. This is the tightest packing scheme. Any unused
>	bits are always zeroed. For illustration, suppose we are collecting
>	NI digital lines {3:5} from dev1 and line #0 from dev2. These will be
>	packed next to each other as the lower 4 bits of a single 16-bit word.
>
> 3. If a second device is used, each MN, MA, ... category  within the
>	central stream is seamlessly expanded as if there were a single
>	higher capacity device.


#### FYI Architecture Notes

The central stream lies in the middle. On the input side the main data
acquisition thread samples the NI-DAQ cards 1000 times per second, interleaves
and organizes the data into categories and adds the new samples to the
input end of the queue. Data older than 30 seconds are dropped from the
other end of the queue. The data acquisition thread is the only entity
that writes to the stream and it accesses the stream in serial fashion.

On the other side, stream **readers** have read-only random access. There
are several separate threads that extract samples for their special purposes:

* The graph fetcher pulls recent data from the stream and pushes it into
the graph buffers.
* The AO fetcher pulls recent data and pushes it into the NI-DAQ AO buffers.
* The trigger module scans the stream for condition you've specified and then
opens, writes and closes files accordingly.
* The Command server fetches data from the stream on behalf of remote
applications.

These and other threads run effectively concurrently, but again, more
processors enables better load balancing.

## Console Window

The `Console` window contains the application's menu bar. The large text
field ("Log") is a running history of informative messages: errors, warnings,
current status, names of completed files, and so on. Of special note is
the status bar at the bottom edge of the window. During a run this shows
the current gate/trigger states and the current file writing efficiency,
which is a key readout of system stability.

>**You are encouraged to keep this window parked where you can easily see
these very useful experiment readouts**.

* Control report verbosity with menu item `Options/Debug Mode`.
* Enable/disable log annotation with menu item `Tools/Edit Log`.
* Capture recent log entries to a file with menu item `Tools/Save Log File`.

## Configure Acquisition Dialog

Notes on the dialog as a whole:

* Settings are divided into subgroups on the various tabs. Validation
(a.k.a. sanity checking) is always performed on all of the settings on
all of the tabs. Validated settings are stored in `SpikeGLX/configs/daq.ini`.

* Press `Last Saved` to revert the entire dialog to the values in `daq.ini`.

* Press `Verify | Save` to sanity-check the settings on all tabs, and if
valid, save them to `daq.ini` **without** initiating a new run. This is
useful when trying to make the acquisition and AO dialog settings agree
before starting a run, as AO settings are checked against `daq.ini`.

* Press `OK` to validate and save the settings to `daq.ini` and then
start a new run.

* Press `Cancel` to end the dialog session without further altering `daq.ini`.

## NI Input -- Configuring NI-DAQ Devices

### Sample Clocks -- Synchronizing Hardware

For all modes:

* A second device, if used, _always needs an external clock source_, and
that source must always be the same clock that drives device1. This is the
only way to coordinate the two devices. The NI breakout boxes, like
`BNC-2110`, make this simple.

* For that matter, the AO clock needs to be the same as the device1 clock.
Unlike the choices for device2, here we provide an `Internal` option for
AO, which only makes sense if AO and AI both reside on device1 and `Internal`
is selected for both (less noisy and clumsy than running a wire).

* There is a `Sync checkbox` and a selectable digital output line. If
enabled, when the run starts, the selected line goes from low to high
and stays high until the run is stopped. This is always an option you
can use to hardware-trigger other components in your experiment.
_(Whisper systems require this signal on line0)._

**{Clock, Muxing, Sample Rate}** choices depend upon your hardware--

#### Case A: Whisper Multiplexer

If you specify any MN or MA input channels, the dialog logic assumes you
have a Whisper and automatically forces these settings:

* Device1 clock = `PFI2`.
* Start sync signal enabled on digital `line0`.

You must manually set these:

1. Set `Chans/muxer` to 16 or 32 according to your Whisper data sheet.
2. Make sure power is turned on and click `Measure ext. clock rate` button.
This will measure the actual clock pulse train from the Whisper and report
the effective `Samples/s` value (= measured rate / muxing factor). We want
an accurate estimate to translate between counts of stream samples and wall
clock time.

#### Case B: Whisper with Second Device

Follow instructions for Whisper in Case A. In addition:

1. In the device2 box, select a PFI terminal for the clock.
2. Connect the "Sample Clock" output BNC from the Whisper to the selected
PFI terminal on the NI breakout box for device2.

> Note that the BNC should be supplying the multiplexed clock rate:
`(nominal sample rate) X (muxing factor)`.

#### Case C: Non-Whisper External Master Clock

You may not have a Whisper, but are nevertheless getting a master sample
clock input from an external source. Follow these steps:

* Set device1 clock = PFI terminal.
* Connect external clock source to that terminal.
* _Optionally_ connect same external signal to a device2 PFI terminal.
* **Important**: Set `Chans/muxer` = 1.
* Make sure the external clock is running and click `Measure ext. clock rate` button.
This will measure the actual clock pulse train from the Whisper and report
the effective `Samples/s` value (= measured rate / muxing factor). We want
an accurate estimate to translate between counts of stream samples and wall
clock time.

#### Case D: Internal Clock Source

You can run without any master clock this way:

* Set device1 clock = `Internal`. In this mode we program the device1 Ctr0
to be the master clock using the sample rate you enter in `Samples/s`. The
Ctr0 signal is available as an output from device1 (see the pin-out for your
device). On the NI BNC-2110 breakout box this is usually available at terminal
`P2.4`.
* _Optionally_ connect a wire from the device1 Ctr0 output pin to a selected
`PFI` terminal on device2.
* Set a `Sample/s` value (you command a desired value rather than measure it).

### Input Channel Strings

The **{MN, MA, XA}** input fields work just like the page range field in
a print dialog. For example, "0,2:4,6" means you're using NI analog-input
(AI) channels {0,2,3,4,6}. If you are not using a particular category,
like MA, then leave that field empty.

Remember that channel ordering is important. In the central stream we want
all the MN channels (if any) to come first, followed by the MA, then XA then
XD channels. These groupings allow the software to know how to process the
channels: what gain to apply, what type of filter to use, what to name it,
and so on. When we acquire AI data from an NI card it returns data from
channel AI0 (if any) then from AI1, and so on. **Therefore**, we require
you to plug your neural multiplexers (MN's) into the lowest numbered AI
channels, then populate the next AI channels with your MA muxers, then
plug in any XA lines. For example, this is legal:

```
MN = 2,4	// don't have to start at zero, gaps are okay
MA = 5		// MA comes AFTER MN
XA = 7		// XA strictly AFTER MN and MA
```

>Note: The cabling of the Whisper system automatically routes multiplexers
to the proper AI channels, but to set up the dialog **you** still have to
know that your Whisper box has neural muxers on AI channels `0:5` and
aux multiplexers on channels `6,7` (for example).

#### Digital Strings

As with AI channel strings, the XD field takes a range string like
"1,2:4,6:11,15" but in this case the values are digital line numbers.

You have to check the data sheet for your NI device to see which digital
input lines are supported. Something that can be very confusing is that
lines are grouped into ports. For example, you may learn that your card
has two hardware-timed ports and that each is 8-bits wide. If you have
prior experience with NI programming you may already know that one can
refer to the lines by names like "Dev1/port0/line1". However, of interest
to us is that you can also name lines without reference to ports and just
use "Dev1/line1" which is implicitly assumed to be on port0 because the
line number is lower than 8. In this convention, the first line
on port1 is "Dev1/line8" and the highest line on port1 is "Dev1/line15".
That's the convention we want you to use here: just use line numbers.

### MN, MA Gain

The multiplexers in your system may have a gain factor. In a typical
Whisper box, the MN channels route through Intan chips with a gain of 200
and the MA channels are handled by a unity gain muxer (see the data sheet
for your hardware).

These gain values **do not** affect values recorded in disk files. They are
only applied in the Graph window so that familiar, unamplified voltages are
plotted and reported in the graph statistics.

>Note that some trigger modes ask you to specify a threshold voltage. The
value you enter for a threshold should always be what you read directly
from the graph of that channel. The software will make any necessary
gain adjustments.

### AI Range

NI devices let you configure an expected voltage range for an analog channel,
say [-2.5..2.5] volts. The purpose of this is to improve your dynamic range,
a.k.a. voltage resolution. If you know in advance that none of your voltages
will exceed 2.1 volts, then choosing [-2.5..2.5] is better than [-5..5] because
you'll get twice the "precision" in your measurements. However, the value 3.0V
would be pinned (saturated) at 2.5V which is bad. In that case, [-5..5] is
a safer choice. Generally, choose the smallest range compatible with your
instrument specifications.

>Note that other components in the chain may impose their own voltage
restrictions. For example, some MA channel banks on some Whisper models
saturate at 2.5V. It would be a bad idea to use such channels to read an
instrument making output in the range [0..3.3] volts.

## Gates -- Synchronizing Software

### Run -> Gate -> Trigger

Gates generalize and replace the "StimGL Integration" feature. The new
hierarchical **run/gate/trigger** scheme provides several options for carving
an experiment "run" into labeled epochs with their own data files. The
terms "gate" and "trigger" were chosen because they are "Biology neutral".
You decide if epochs are really 'windows', 'events', 'trials',
'sessions' or other relevant contexts.

In the new scheme:

1. You configure experiment parameters, including a `run folder` where all
the output files will be stored, a `run name`, a `gate` method and a `trigger`
method.

2. You start the run with the `OK` button in the Configure dialog. That means
the data acquisition devices begin collecting scans and the Graphs window
begins displaying streaming data. _(A run can also be configured and/or started
using TCP/IP from a remote application)._

3. Initially, the gate is low (closed, disabled) and no files can be written.
When the selected gate criterion is met the gate goes high (opens, enables),
the gate index is set to zero and the trigger criteria are then evaluated.

4. Triggers determine when to capture data to files. There are several
options discussed more fully in the next section. Triggers act only within
a gate-high epoch and are terminated if the gate goes low. **Gates always
override triggers**. Each time the gate goes high the trigger criterion
is evaluated anew.

5. When the selected trigger condition is met, a new file is created using
the naming pattern `run-path/run-name_g0_t0.nidq.bin`. When the trigger goes
low the file is finalized/closed. If the selected trigger is a repeating
type and if the gate is still high then the next trigger will begin file
`run-path/run-name_g0_t1.nidq.bin`, and so on within gate zero. (For IMEC
data streams, the same naming rule applies, with `nidq` replaced by `imec`).

6. If the gate is closed and then reopened, triggering resets and the
next file will be named `run-path/run-name_g1_t0.nidq.bin`, and so on.

7. The run itself is always stopped manually, either from the SpikeGLX
GUI or from a remote application.

### Gate Modes

At present we have provided two simple gate methods:

* `Immediate Start`. As soon as the run starts the gate is immediately
set high and it simply stays high ("latches high" in electronics lingo)
until the run is stopped.

* `Remote Controlled Start and Stop`. SpikeGLX contains a "Gate/Trigger"
server that listens via TCP/IP for connections from remote applications
(like StimGL) and accepts simple commands: {SETGATE 1, SETGATE 0}.

## Triggers -- When to Write Output Files

Rule 1: **A file is being written when the trigger is high**.

Rule 2: **Every binary (.bin) file has a matching (.meta) file**.

>To capture final checksum and size, the metadata are written when
the binary file is closed.

### Trigger Modes

* `Immediate Start`. As soon as the run starts the trigger is immediately
set high and it simply stays high ("latches high") until the run is stopped.
Select 'Immediate' mode for the gate and trigger modes to start recording
immediately without fussing about experiment contexts.

<!-- -->

* `Timed Start and Stop`. First, _optionally_ wait L0 seconds, **then**:
    + Latch high until the gate closes, or,
    + Perform a sequence:
        - Write for H seconds.
        - Idle for L seconds.
        - Repeat sequence N times, or until gate closes.

<!-- -->

* `TTL Controlled Start and Stop`. Watch a selected channel for a positive
going threshold crossing, **then**:
    + Latch high until the gate closes, or,
    + Write for H seconds, or,
    + Write until channel goes low.
    + _Latter 2 cases get flexible repeat options_.
    + _Threshold detection is applied to unfiltered data_.

<!-- -->

* `Spike Detection Start and Stop`. Watch a selected channel for a positive
going threshold crossing, **then**:
    + Record a given peri-event window about that to its own file.
    + Repeat as often as desired, with optional refractory period.
    + _Threshold detection is applied post 300Hz high-pass filter_.

<!-- -->

* `Remote Controlled Start and Stop`. SpikeGLX contains a "Gate/Trigger"
server that listens via TCP/IP for connections from remote applications
(like StimGL) and accepts simple commands: {SETTRIG 1, SETTRIG 0}.

>Note that some trigger modes ask you to specify a threshold voltage. The
value you enter for a threshold should always be what you read directly
from the graph of that channel. The software will make any necessary
gain adjustments.

### Trigger Disable At Run Start

Check this box on the `Configure Dialog/Triggers Tab` to start the run
with triggering disabled. This lets you look at the streaming data without
writing any files. When ready, check the `Trig Enable` box in the
`Graphs Window` to resume normal operation.

### Trigger Pause and Resume

In the `Graphs Window`, uncheck the `Trig Enable` box to block all file
writing until you re-check the box. While paused, you can:

* Change the `Save Channel Subset` using the graph checkboxes.

>_Note that you are permitted to change the saved channels without renaming
the run, but doing so is bound to cause later confusion_.

* Change the run name.

    In the text box next to `Trig Enable` enter a name different from
    the current run name. **Do not** adorn the name with gate/trigger indices
    of the form `runname_g12_t14`. Rather, the software detects that the
    run name is new and automatically resets the counters to: `_g0_t0`.

* Change the _g/t file index numbers.

    You can force the gate/trigger counters to resume at desired values
    by entering the current run name, adorned with the desired indices,
    for example `runname_g12_t14`. **Note that this feature does not check
    for pre-existing files with the resulting name**.

## See N' Save -- Focusing on Interesting Channels

### Channel Map

The Graphs window arranges the channels in the standard `acquisition` order
(MN, MA, XA, XD) or in a `user` order that you can specify using a `Channel
Map`.

A rudimentary tool is provided to create, edit and save channel maps (and
channel map (.cmp) files). Click `Edit` on the `See N' Save` tab of the
`Configure dialog` to launch the `Channel Map dialog`.

To make and use a map you must save it in a file. A Map is very simple.
A map file looks something like this:

```
0,0,6,2,32,0,1	// header (type counts): 0,0,MN,MA,C,XA,XD
MN0C0;0 256		// channel-name;acq-index sort-index
MN0C1;1 1
MN0C2;2 2
MN0C3;3 3
...
XD0;256 0		// this user put digital graph first
```

You can save and reuse channel map files in another run by loading that
file from the Channel Map dialog. However, this only makes sense if the
loaded map describes the same types and counts of channels as you've
configured in the current run, hence, the header values, which are counts
of channel types. The `C` value is the number of channels per muxer. The
extra zeros are for future channel types.

Editing the sort order simply consists of reordering the second column of
channel index values which must be in the range [0..N-1], where N is the
total channel count. For digital data we don't count individual lines.
Rather we count 16-line blocks of channels.

> You can edit these files in any text editor if you prefer. You can
change channel name strings too (shh).

### Save Channel Subsets

The hardware configuration tabs determine which channels are acquired from
the hardware and held in the central data stream. All acquired channels
are shown in the Graphs window. However, you don't have to save all of the
channels to your disk files.

You can enter a print-page-range style string for the subset of channels
that you want to save. This string is composed of index numbers in the
range [0..N-1], where N is the total channel count. To save all channels
you can use the shorthand string `all`, or just `*`.

You can also change this list from the Graphs window:

1. Uncheck `Trig Enable`.
2. Use the checkboxes on the individual graphs to mark them for saving.
3. Recheck `Trig Enable`.

## Graphs Window Tools

### Toolbar Controls

* `Acq/Usr Order`: This button toggles between acquired (standard)
channel order and that specified by your custom [channel map](#channel-map).

* `MN0C0;0`: Next to the Order button is the name of the currently selected
graph. Single-click on any graph to select it. The current graph is the
target of several controls such as the axis scaling boxes. Click on the
channel-name string in the toolbar to switch to the tab containing the
current selection.

* `Pause`: The Pause VCR-style button toggles between pause and play of
stream data in the graphs so you can inspect an interesting feature.
This does not pause any other activity.

* `Expand`: This button toggles between showing just the selected channel,
and the standard multi-channel view. You can also double-click of a graph
to select it and expand/contract it.

* `Seconds`: Enter a value in range [0.001..30.0] seconds to set the
time span of the selected graph.

* `Y-scale`: Enter a value in range [0.01..9999.0] volts to set the
span of the selected graph.

* `Color`: Click this button to get a Color picking dialog whereby you can
define an alternate color for the **data** in the selected graph. This only
works for analog channels; digital traces are auto-colored.

* `Apply All`: Copies {X,Y-scale and color} from the selected graph to
all other graphs of the same {neural, aux-analog, aux-digital} category.

* `Filter`: Applies a 300Hz high-pass filter to the neural channels. This
only affects graphing.

* `Trig Enable`: Use this to [pause/resume](#trigger-pause-and-resume) the
saving of data files, to change [which channels](#save-channel-subsets)
are being written to disk files, or to
[edit the name](#trigger-pause-and-resume) of the run and its disk files.

* `Stop Run`: Stops the current run and returns the software to an idle state.
You can do the same thing by clicking the `Graph Window's Close box` or by
pressing the `esc` key, or by choosing `Quit (control-Q)` from the File
menu (of course the latter also closes SpikeGLX).

### Other Graph Window Features

* Hover the mouse over a graph to view statistics for the data
currently shown. Note that the reported time is an average time
of the plotted data.

## Offline File Viewer

The File Viewer feature is being redesigned. It is completely disabled
at this time.

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





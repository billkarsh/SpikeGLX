## NI Tab

## NI Hardware Requirements

SpikeGLX has these requirements for NI recording devices:

1. It must be an NI device that we can talk to via DAQmx (a general
purpose device programming language for NI hardware). DAQmx allows
SpikeGLX to communicate with devices using any protocol: **PXIe, PCI
or USB**.

2. It must be an M-series (62XX), S-series (61XX), X-series (63XX),
or digital IO (653X) device.

>Note: As of version 20190305 SpikeGLX can read up to 32 digital lines
per device (previously limited to 8). Also, be aware that only a device's
'waveform' digital lines can be programmed for high sample rate input.
You'll have to look at the device spec sheet to see the count of
waveform lines. Digital device support is added as of version 20190413.

We have direct experience with these:

* PCI-based 6221 (M)
* PCI-based 6133 (S)    (16 MS FIFO tested)
* PXI-based 6133 (S)    (16 MS FIFO tested)
* PXI-based 6341 (X)
* PXI-based 6363 (X)
* USB-based 6366 (X)
* PXI-based 6535 (digital)

### Simultaneous Sampling (Fast, Expensive)

Some models (S and some X) have a feature called 'simultaneous sampling'
which means each input channel gets its own amplifier and ADC. This allows
the device to sample all its channels in parallel at the advertised maximum
sample rate, for example, 2.5 mega-samples/s/channel for the 6133. Moreover,
there is no crosstalk between the channels. That's what makes these models
very capable and very expensive. This is a must when using a Whisper
multiplexer which samples all AI channels at 800 kHz.

### Multiplexed Sampling (Slower, Budget Friendly)

When doing multichannel acquisition, non-simultaneous-sampling devices
such as the 6221 use a multiplexing scheme to connect inputs to the
single amplifier/ADC unit in quick succession. The fastest you can drive
such a device depends upon how many channels you want to sample. It's
`R0/nChans`: R0 is the advertised maximum sample rate (250 KS/s for
the 6221). Be aware that switching from channel to channel at this rate
does not allow the amplifier to fully settle before the next input is
connected to it, hence, there will be some crosstalk (charge carryover).
To avoid that issue, run at a lower maximum sample rate given by:
`1/(1/R0 + 1E-5)`. For the 6221 example, you should sample no faster
than `71428/nChans`.

### USB Devices

USB-based devices such as the 6366 can't use DMA data transfers, so have
lower effective bandwidth and higher latency than PCI or PXI devices. Go
ahead and use it if you already have one. However, don't use these for
digital input channels: The combination of low transfer rates and a very
small digital FIFO buffer make digital buffer overruns fairly common.

### Favorite All-Arounders

The X-series strike a balance between high sample rate (limited by settle
time) and high channel count. The 6363 has 32 AI and 32 waveform DI channels.
The 6341 has 32 single ended AI and 8 waveform DI channels for half the
price. Remember that AI channels can equally well read analog and TTL inputs.

### Breakout Box and Cable

Your NI module will talk to the world via a high density multi-pin connector,
so you'll also want a breakout box (connector block) and cable that works
with your module. Browse here for
[NI multifunction IO](https://www.ni.com/en-us/shop/select/pxi-multifunction-io-module)
devices. Click on a table entry and a `View Accessories` button will appear.
There are easier to use options like the BNC-2110 that provide BNCs for the
most often accessed channels, and the SCB-68A that offers only screw terminals
but is more versatile because you can access all channels.

--------

## Primary and Secondary Devices

SpikeGLX can operate two cards **provided they have identical model
numbers**. SpikeGLX treats such a pair as a single device with double
the channel capacity.

If using just one NI card, it is always the `primary` device.

If enabling a second identical card, it is the `secondary` device.

--------

## Primary Device

* Select the device name in the combo-box menu.
* Select the sample clock source, either internal, or an external terminal.
* List the NI channels to acquire as instructed here.

```
MN = 'multiplexed neural'    (requires Whisper multiplexer)
MA = 'multiplexed auxiliary' (requires Whisper multiplexer)
XA = 'non-multiplexed auxiliary analog'
XD = 'non-multiplexed digital'
```

### Analog Channel Strings

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
MN = 2,4  // don't have to start at zero, gaps are okay
MA = 5    // MA comes AFTER MN
XA = 7    // XA strictly AFTER MN and MA
```

>Note: The cabling of the Whisper system automatically routes multiplexers
to the proper AI channels, but to set up the dialog **you** still have to
know that your Whisper box has neural muxers on AI channels `0:5` and
aux multiplexers on channels `6,7` (for example).

### Digital Strings

As with AI channel strings, the XD field takes a range string like
"0,2:4,6:7" but in this case the values are digital line numbers.

We support up to 32 lines from each device, so legal line numbers are
[0..31]. Note that Whisper systems reserve line #0 as an output line that
commands the Whisper to start.

--------

## Secondary Device

A second card (identical model to the first) can be used to add additional
channels. SpikeGLX will treat all the specified channels as if they came
from one device. For that to work, these two cards must run in synchrony
(be time-locked to each other). To ensure that, you will connect the sample
clock that is driving the primary card to a sample clock input terminal
on the secondary card.

To use two cards:

* Check the `Secondary device` box.
* Select the device name.
* Select the sample clock terminal that will get a driving signal from the
primary device.
* List the channels to acquire. These need not match the channels listed
for the primary device.

--------

## Common Analog Params

These apply to both primary and secondary devices.

### Chans / Muxer

See the data sheet for your external multiplexing device (muxer). The
Whisper acquires 32 channels for each NI channel.

### MN, MA Gain

The multiplexers in your system may have a gain factor. In a typical
Whisper box, the MN channels route through Intan chips with a gain of 200
and the MA channels are handled by a unity gain muxer (see the data sheet
for your hardware).

The gain values you enter here **do not** affect values recorded in disk
files. They are only applied in the Graph window so that familiar,
unamplified voltages are plotted and reported in the graph statistics.

>Note that some trigger modes ask you to specify a threshold voltage. The
value you enter for a threshold should always be what you read directly
from the graph of that channel. The software will make any necessary
gain adjustments.

### AI range (V)

NI devices let you configure an expected voltage range for analog channels,
say [-2.5..2.5] volts. The purpose of this is to balance dynamic range
versus voltage resolution. If you know in advance that none of your voltages
will exceed 2.1 volts, then choosing [-2.5..2.5] is better than [-5..5] because
you'll get twice the resolution in your measurements. However, the value 3.0V
would be pinned (saturated) at 2.5V which is bad. In that case, [-5..5] is
a safer choice. Generally, choose the smallest range compatible with your
instrument specifications.

>Note that other components in the chain may impose their own voltage
restrictions. For example, some MA channel banks on some Whisper models
saturate at 2.5V. It would be a bad idea to use such channels to read an
instrument making output in the range [0..3.3] volts.

### Termination

Depending upon the NI device capabilities, voltage measurements can be:

* Single-ended: Referenced to ground.
* Differential: Referenced to another channel.

Consult NI documentation; wire inputs accordingly.

--------

## Timing

### Clock Source

* **Clock source : set rate**: Use the combo-box to select an existing
sample clock source and its set (programmed) rate. The `meas` box to its
right shows the measured (actual) rate of this clock if it has been
calibrated, or the set rate if not yet calibrated.

* **New Source**: To create new entries for the `Clock Source` combo-box
menu, click `New Source` and follow the [Help](NIClock_Help.html) in
that dialog.

### Optional Start Signal

There is a `Start checkbox` and a selectable `digital output line`. If
enabled, when the run starts the selected line goes from low to high
and stays high until the run is stopped. This is always an option you
can use to hardware-trigger other components in your experiment.
_(Whisper systems require this signal on line0)._ If this feature
is enabled, that line cannot also be listed as a digital input
(**XD**) line.

--------

## Maps

### Shank Map

This describes two things for each (**MN**) channel:

1. Where that channel's electrode is on the probe. The location is a set
of three indices for the site's (shank, column, row), each zero-based.

2. Whether the channel is "used": shown in displays and included in CAR.

The default shank map is a probe with 1 shank, 2 columns and a row count
equal to MN/2 (neural channel count / [2 columns]).

>The ability to manually edit the locations of sites is mainly useful
when using third-party probes with the NI/Whisper recording system.

### Channel Map

This lets you group/sort/order the channels in SpikeGLX graph windows.
It has no effect on how binary data are stored.

### Save Channels

You can save all of the channels being acquired by setting the list to
any of:

* blank
* '*'
* 'all'

You can save any arbitrary subset of channels using a printer-page-like
list of individual channels and/or ranges, like: 1:3,5,100:127.

--------

--------

## Examples of Sample Clock Configuration

* A secondary device, if used, _always needs an external clock source_, and
that source must always be the same clock that drives the primary. This is
the only way to coordinate the two devices. The NI breakout boxes, like
`BNC-2110` or `SCB-68A`, make this simple.

* You always need to select a clock source in the `Timing` group box.
Your selection specifies two key values. (1) It specifies a
`Set_sample_rate` (read directly in the menu control) that names the
nominal rate for the source, and is used to program your NI device if
`Primary clock line` is set to `Internal`. (2) Your selection looks up
the `Measured_sample_rate` for this device and enters that in the
`meas` box.

* You will get the best possible alignment of data across your files
if you use the calibration features on the `Sync tab` to measure the
true sample rates of your devices.

**{Clock, Muxing, Sample Rate}** choices depend upon your hardware--

### Case A: Internal Clock Source, No Multiplexing

In this simple case, there is no external sample clock. Rather, you
can set the NI device to generate its own sample clock waveform.
Note that an NI device can achieve a precise value only if it evenly
divides the master clock rate. The master rate for a 6133 is 20 MHz
which is divisible by 40000 but not by 30000, for example. Although
generated internally, the clock source is also routed to an external
terminal so you can share it with other hardware:

* On multifunction IO devices, the output is available at
Ctr0Output/pin-2/PFI-12.
* On digital IO devices like the 6535, the output is at the 1st PFI terminal
listed in the primary clock line menu, which is usually pin-67/PFI-5.

Settings:

* Set primary clock line = `Internal`.
* _Optionally_ connect a wire from the primary clock terminal to a selected
`PFI` terminal on the secondary device.
* Leave the MN and MA channel boxes blank.
* Specify input channels in the XA and XD boxes.
* `Chans/muxer` is ignored for these channels.

### Case B: External Clock Source, No Multiplexing

In this case, the sample clock is being driven by some component in
your setup, other than the primary NI device. Follow these steps:

* Set primary clock line = PFI terminal.
* Connect external clock source to that terminal.
* _Optionally_ connect same external clock to a secondary device PFI terminal.
* Leave the MN and MA channel boxes blank.
* Specify input channels in the XA and XD boxes.
* `Chans/muxer` is ignored for these channels.

### Case C: Whisper Multiplexer

If you specify any MN or MA input channels, the dialog logic assumes you
have a Whisper and automatically forces these settings:

* Primary clock line = `PFI2`.
* Start sync signal enabled on digital `line0`.

You must manually set these:

* Set `Chans/muxer` to 16 or 32 according to your Whisper data sheet.

### Case D: Whisper with Second Device

Follow instructions for Whisper in Case C. In addition:

1. In the secondary box, select a PFI terminal for the clock.
2. Connect the "Sample Clock" output BNC from the Whisper to the selected
PFI terminal on the NI breakout box for the secondary device.

> Note that the BNC should be supplying the multiplexed clock rate:
`(nominal sample rate) X (muxing factor)`.

--------

--------

## Sharing Channels With Other Programs

### Digital Lines (XD)

*Non-waveform lines are not clocked. If SpikeGLX is using that line, it
owns that line; otherwise the line is available to external software.*

*Waveform (clocked) digital lines are subject to sharing limitations
that derive from use of clock resources as discussed below.*

#### SpikeGLX Clock Source External

The digital lines that are not being used in SpikeGLX should be available
in other software on a line-by-line basis.

#### SpikeGLX Clock Source Internal

All digital lines are owned exclusively by SpikeGLX.

--------

### Analog Channels (XA)

#### Listed XA Channels

If any XA channels are listed, then all analog channels are owned
exclusively by SpikeGLX.

#### Only Digital XD Lines; No XA Channels Listed

**SpikeGLX Clock Source External**

All analog channels are available to other software.

**SpikeGLX Clock Source Internal**

This case depends upon the type of chassis and the type of NI device.

For NI chassis, we can clock digital acquisition directly from the
internal clock without involving analog input, so in this case all
analog channels remain available to external software.

For ADLink chassis, some devices can clock digital lines directly from
the internal clock, but other devices piggyback digital timing off of
an intermediary (hidden) analog input task and in those cases the analog
channels are exclusively owned by SpikeGLX. The only device we are
currently aware of with this issue is the **6133**.

*Note1: We don't have experience with other chassis at this time.*

*Note2: PCI-based and USB-based NI devices operate the same as if the PXI
version of that device were pugged into an NI chassis.*


_fin_


## Sync Tab

**In brief: Mapping Time Across Streams**

--------

## Square Wave Source

Choose `Disable sync waveform` to run without active alignment to edges
of a common square wave. The software will still apply the measured sample
rates for streams that have previously been calibrated.

Otherwise, a generator source should be programmed to form a simple
square wave with a 1 second period and 50% duty cycle. You have three
choices for the generator:

* `Separate high precision pulser` allows you to provide any waveform
generator you like for the purpose.

* `Current NI acquisition device` will program your multifunction NI device
to produce the required waveform at terminal `Ctr1Out (pin-40/PFI-13)`.
(Brian Barbarits will make a simple breakout connector available to allow
Whisper users to access this signal.) Digital devices like the 6535 are
programmed to make the waveform on `line-0`.

* `Imec slot N` will program the indicated BS (PXI or Onebox) to produce
the sync waveform and make it available at its SMA connector.

Whichever pulser source you select, the programmed (set) period is 1
second, but the actual period may differ from that. If you have measured
the actual period of the generator's output with a high precision
frequency/period analyzer, then enter that value in the `Measured period`
box. If you have not measured it, enter **1** in the box.

--------

## Imec SMA Connector

Each imec BS cards has a single SMA connector on its front panel.

In a PXI chassis you might have more than one imec BS module. However,
the modules share the (input/output) sync signal on chassis backplane
line <7>. Hence, in all conditions, you should connect the entire set
of PXI modules to the rest of the world via **one and only one**
SMA connector.

**Example**: If an Imec PXI BS is selected as the source, all Imec BS
modules in the chassis will automatically get the signal via the chassis
backplane. Connect only the designated source SMA to other devices.

**Example**: Whenever the selected source is **NOT** an imec PXI slot,
then all imec PXI modules are set to input mode, and will share whatever
signal you connect to the specified **input** `Imec PXI SMA slot`.
Connect only the designated input SMA to other devices.

Whenever any imec BS (PXI or Onebox) is not being a source, e.g.
`Disable sync waveform` is selected, it is placed in input mode.
In this mode it can record one TTL signal of your choice as bit #6
of the SY word. Each Onebox can separately record up to one TTL
signal (at its sync SMA). A chassis with one or more BS modules will,
as a whole, record up to one TTL signal at the designated input SMA,
which is propagated on the backplane to all PXI streams.

All sync SMA connectors are compatible with 0-5V TTL signals.

--------

## Nidq Input

If using an NI device with sync, you will always have to connect a wire
to one of its analog or digital input terminals. *Yes, even if that
device is selected as the sync source!!*

At this time, Whisper boxes do not allow access to digital inputs, so you
must connect the square wave to one of the multiplexed auxiliary analog
inputs (MA).

--------

## Calibration Run

To do a run that is automatically customized for sample rate calibration,
check the box in this item group and select a run duration. We can't tell
you how long is optimal. That depends upon how stable the clocks are and
you can see that for yourself by repeating this measurement to see how it
changes over time. It's probably a good idea to turn on power and let the
devices approach a stable operating temperature before calibrating. We
can't tell you how long that should be either. Our typical practice is
30 minutes of warm up and 30 minutes of measurement time.

A calibration run will save data files to the current run directory
specified on the `Save tab`. The files will automatically be named
`CalSRate_date&time_g0_t0...`

When the run finishes SpikeGLX will analyze the measured sample rates of
all the selected devices and report the results in a dialog where you can
adopt or reject them.

--------

## Measured Samples/s

When you do a calibration run (and it is successful) the results are
stored into your `_Calibration` folder. The next time you configure
a run the results will automatically appear in the `meas` boxes on the
{IM, OBX, NI} dialog tab appropriate to that stream.

Tabulated device calibration values are stored in the files:

* `_Calibration\calibrated_sample_rates_imec.ini`
* `_Calibration\calibrated_sample_rates_nidq.ini`

You can manually edit these if needed, say, if you've swapped equipment
and already know correct rates from previous measurements.

To give you a sense of how much measured values differ from the nominal
rates, here's what we typically get:

* IM: 30000.083871
* NI: 25000.127240.


_fin_


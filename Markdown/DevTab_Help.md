## Devices Tab

**In brief: Select which components (data streams) to enable.**

1. Describe which Neuropixels components you've installed.
2. Enable the subset of those you intend to run.
3. Enable National Instruments components, if using.
4. Click `Detect` to discover if enabled streams are present and working.

Following that:

* Work through the other tabs to further configure those streams.
* Start the run.

>Settings Memory:
>
>SpikeGLX remembers everything about your installed components and
run settings in its database. If you didn't want to change anything
from the last time you ran, you'd only have to do these three things:
>
>(1) Click `Detect`, (2) type a runName (`Save tab`), (3) click `Run`.
>
>You only need to make changes if needed.
>
>All the run settings are also stored in the `.meta` files that accompany
your `.bin` file output. This is a complete record of that experiment.

### SpikeGLX vs Open-Ephys

Selecting Imec components is quite different between these two.

Open-Ephys will try to discover, configure and run all the hardware it
can find. If you don't want to run a given probe, you have to unplug it
and therefore hide it from Open-Ephys.

SpikeGLX doesn't guess what you want to do. Rather, at least once, you tell
it what hardware you installed. For each run, you tell it which of those
devices to use (just check a box). By asking about your explicit
intent, we ensure precisely those devices are indeed present and working.

--------

## Dialog Notes

* The `probe table` scrolls!
* Resize columns by clicking on the dividers between column headers.
* Double-click on a divider to auto-size that column.
* Drag the `Configure` dialog's outer edges or corners to resize all content.

--------

## Imec

* Check the `Enable Imec` box if you intend to use imec probes or OneBoxes.

### Populate the Probe Table

Initially, your `probe table` is empty.

Before you can enable imec parts for a run, you need to tell SpikeGLX
which base station (BS) modules and/or OneBoxes you have, and give each
a slot assignment using the `Configure Slots` dialog.

### Configure Slots

Click `Configure Slots`, above the `probe table`, to get the dialog.

The [Help](Slot_Help.html) there, discusses **slots, ports and docks**, and
how to populate the `probe table`.

### Enable Checkboxes

In the `probe table` check the `Enab` boxes for each probe or OneBox you
want to include in this run.

The message box shows how many streams are checked for each slot.

* Probes are identified by their **(slot,port,dock)**.
* OneBoxes are identified by **(slot,"XIO")**.

### Enable Probe Shortcuts

There are some one-click shortcuts for making multiple boxes match the
one you are clicking:

* **Alt-click** to make all boxes with the same (slot, dock) match this one.

* **Ctrl-click** to make all boxes with the same (slot, port) match this one.

* **Shift-click** to make all boxes in the table match this one.

### Include Probe Checks

**(Connection Failures, Broken Shanks)**

Imec probes have two common failure modes. These and many other issues
can be checked using the built-in self test (**BIST**) dialog, under the
`Tools` menu.

Nevertheless, because these two issues are so common, and the tests run
quickly, we recommend you check this box to automatically catch problems
before every run:

1. Are the probes correctly plugged into their headstages? If not, you'll
get a "parallel-serial bus error" reported here.

2. Are the internal shank shift register chains intact and functioning? If
not, you'll get a warning here.

>**IMPORTANT:**
>
>The shift register chains are the means by which the IMRO table data
(electrode and ref selections) are programmed. If there is a break in the
chains the probe may run, but you'll have no idea which electrode sites are
actually connected and read out. How does this break? Snapping the shank
off would certainly cause this. Unfortunately, the internal wiring in a
shank can develop breaks from only modest bending with no outward sign
of damage. It isn't necessarily your fault. It's a technology limitation.

### Detect

Clicking `Detect` is always required to run. It forces SpikeGLX to talk
to each selected device, to check if it is working, to get its model and
serial numbers, and enables SpikeGLX to sanity check settings against device
capabilities.

Devices are checked slot by slot, port by port. If there is any fault,
the checking is halted and that issue is reported in the message box,
along with the final status "**FAIL**."

If all is well, the final text is "**OK**."

### Logical Index Numbers

These numbers are input recording stream identifiers.

After you specify which physical slots, ports and docks are enabled and
click `Detect`, the table will assign each probe a `logical ID` number
in a simple manner, slot by slot, then port by port, then dock by dock.

For example, if you enabled (slot,port,dock): (2,2,2), (2,3,1), (4,1,1),
(4,1,2), they would get logical probe #s: (2,2,2)=0, (2,3,1)=1, (4,1,1)=2,
(4,1,2)=3.

Likewise, slot by slot, the table assigns each OneBox (that is being used
for ADC input) a zero-based logical ID number, independently of the probes.
Said another way, if a OneBox has been given any combination of analog or
digital input channels {XA, XD} it will get a logical ID (input stream index).
If you only configured the output AO (DAC) channels, that OneBox will not
get a logical ID because it is not an input recorder.

>Note: *In the SDK remote interface for talking to OneBoxes, functions
relating to recording operations take (js,ip) indices, where js is the
stream-type (js=1 for OneBox) and ip is the logical ID. Functions relating
to the output (DAC) features refer to a OneBox by its slot number.

>**SpikeGLX uses the zero-based logical stream ID's of probes and OneBoxes
in numerous places:**
>
>* Setting up Sync.
>* Setting up triggers.
>* Names of files, e.g., "imec3", "obx2".
>* Remote {MATLAB,C++} API commands to address recording streams.

--------

## National Instruments

* Check the `Enable NI-DAQ` box if you intend to use any NI devices.

Upon `Detect` SpikeGLX will list:

- Which NI devices can be used for **input**, and for each device...
- How many **high-sample-rate** analog (**AI**) and digital (**DI**) channels
are available.

- Which NI devices can be used for **output**, and for each device...
- How many **high-sample-rate** analog (**AO**) and digital (**DO**) channels
are available.

If any **input** devices are available, the `NI Tab` will be enabled, so you
can select one and configure it.


_fin_


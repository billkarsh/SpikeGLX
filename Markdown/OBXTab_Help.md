## OneBox (Obx) Tab

**In brief: Detailed settings for each OneBox.**

On the `Devices` tab you checked the `enable` boxes for the OneBoxes
you want to run and then clicked `Detect`.

Use this tab to configure parameters for each OneBox, individually.

--------

## OneBox Database

SpikeGLX maintains a database of OneBox settings in the _Calibration folder.
The settings are keyed to the OneBox's serial number. There is one entry per
device, hence, the software remembers the last thing you did with that OneBox.
The entries are each time-stamped. Individual entries are discarded after
three years of disuse.

When you click `Detect` we look for each selected OneBox in the database.
If found, the stored settings are used. If no entry is found, the OneBox
gets default settings.

You can always see and edit those settings using the `Each OneBox` table
on the `Obx Setup` tab.

Upon clicking `Verify` or `Run` the table values are transferred to the
database for future runs.

--------

## Table Viewing and Sizing

* The OneBox table scrolls!
* Resize columns by clicking on the dividers between column headers.
* Double-click on a divider to auto-size that column.
* Drag the `Configure` dialog's outer edges or corners to resize all content.

--------

## Editing

* Select a table row, then click `Default`, or,
* **Double-click** in a table cell to edit that item.

For each OneBox, you can edit its:

* XA list of input analog (AI) channels.
* XD whether to acquire digital lines.
* AI voltage range: +/- {2, 5, 10} volts.
* AO list of output analog (DAC) channels.
* Channel map.
* Save channels.

More details below.

--------

## Measured Sample Rate

Not editable.

This is the OneBox's measured input sample rate as determined by
the calibration procedures on the `Sync` tab (See User Manual).

--------

## XA Analog Channels

Edit directly in the table cell by specifying a channel list,
just as you do for saved channels. E.g., 0:3,7.

Each OneBox can record from up to 12 16-bit analog channels.

If desired, the XA box can be blank (specfying **NO** analog channels).

--------

## XD Digital Lines

If you enable the XD (digital) option, then, regardless of your XA channel
list, **ALL 12** analog inputs are digitized using a fixed threshold value
equal to 10% of: {2.5, 5, 10} as determined by your AI Range selection.
The 12 resulting digital lines are read out together as the lowest 12 bits
of a single 16-bit `XD` word.

In other words, your Obx data stream either has no digital word, or it has
a single digital word containing 12 digital lines.

A OneBox Obx stream is configured for recording if at least one XA channel
is listed, or if XD is enabled. In this case, the OneBox is assigned a
logical recording stream ID index that you can see in the big Probes table
of the Devices tab. In SDK remote interface functions you can access this
device using indices: `(js,ip) = (1,ID)`.

--------

## AI Range

The selected scale +/- {2.5,5,10} volts is applied to all input analog
channels.

--------

## AO Analog (DAC) Channels

Edit directly in the table cell by specifying a channel list,
just as you do for saved channels. E.g., 0:3,7.

Each OneBox can output voltages on up to 12 analog channels.

Each of the 12 channels can only be an input or an output.

>*It is flagged as an error if the same channel is in the XA and the AO list.*

>**CAUTION: You might damage the OneBox or other equipment if you
cross-connect inputs to outputs.**

>Yes it is safe and permitted, to enable XD and specify AO channels.
The digital lines for AO channels are disabled and read as zeros.

A OneBox is configured for output if at least one AO channel is listed.
In SDK remote interface functions you can access the output features for
this device using its assigned `slot number`.

### AO Voltages

* Use the SDK during a run to set constant AO voltage levels: range +/- 5V.
* All listed AO channels are initialized to zero when a run starts.

--------

## Channel Map

This lets you group/sort/order the channels in SpikeGLX graph windows.
It has no effect on how binary data are stored.

--------

## Save Channels

Edit directly in the table cell by specifying a channel list.

You can save all of the channels being acquired by setting the list to
any of:

* blank
* '*'
* 'all'

You can save any arbitrary subset of channels using a printer-page-like
list of individual channels and/or ranges, like: 1:4,6,8:10.

>The SY channel is always added to your list because it carries error flags
and the sync channel.

--------

## Default

This is a handy way to set all of the editable fields:

* XA: 0:11.
* XD: true.
* AI range: 5V.
* AO: blank.
* Channel map: acquisition channel order.
* Save chans: all.

--------

## Copy

Select a OneBox and copy its settings to a specified other OneBox, or to all
other OneBoxes.

* This does NOT copy `measured sample rates`.


_fin_


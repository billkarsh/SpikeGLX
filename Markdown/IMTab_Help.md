## Imec Probe (IM) Tab

**In brief: Detailed settings for each probe.**

On the `Devices` tab you checked the `enable` boxes for the probes you want
to run and then clicked `Detect`.

Use this tab to configure parameters for each probe, individually.

--------

## All Probes

A few settings are common to all the probes...

### Calibration Policy

Select how the imec-provided ADC and gain calibration files for each
probe are handled:

* Absolutely required.
* Used if you have that probe's files, skipped otherwise.
* Skipped for all probes.

### Acquisition Start Signal

Currently we are only supporting software-based initiation of imec data
streaming. That is, the data acquisition starts manually when you click
the `Run` button in the configuration dialog.

--------

## Probe Database

SpikeGLX maintains a database of probe settings in the _Calibration folder.
The settings are keyed to the probe's serial number. There is one entry per
probe, hence, the software remembers last thing you did with that probe.
The entries are each time-stamped. Individual entries are discarded after
three years of disuse.

When you click `Detect` we look for each selected probe in the database.
If found, the stored settings are used. If no entry is found, the probe
gets default settings.

You can always see and edit those settings using the `Each probe` table
on the `IM Setup` tab.

Upon clicking `Verify` or `Run` the table values are transferred to the
database for future runs.

--------

## Table Viewing and Sizing

* The probe table scrolls!
* Resize columns by clicking on the dividers between column headers.
* Double-click on a divider to auto-size that column.
* Drag the `Configure` dialog's outer edges or corners to resize all content.

--------

## Editing

* Select a table row, then click `Default`, or,
* **Double-click** in a table cell to edit that item.

For each probe, you can edit its:

* LED (really, for each headstage).
* IMRO table.
* Bad (stand-by) channels.
* Shank map.
* Channel map.
* Save channels.

More details below.

--------

## Measured Sample Rate

Not editable.

This is the probe's (headstage's) measured sample rate as determined by the
calibration procedures on the `Sync` tab (See User Manual).

--------

## LED

One checkbox per probe, but really the LEDs are on headstages...

* NP 1.0-type probes: One probe per headstage: no conflicts.
* NP 2.0-type probes: Two probes per headstage: requires your thought.

When you click `Verify` or `Run` you will be warned if your LED
settings for the same headstage conflict.

--------

## IMRO Table

Each probe `type` (1.0, 2.0 single-shank, 2.0 4-shank, UHD, 128-channel, ...)
has its own type of tools to edit its `imec readout` table. These are the
probe's channel-by-channel settings, for example:

* Which electrode is connected (its bank).
* Which reference is being used (external, tip, on-shank).
* AP and LF gains and filters (where applicable).

--------

## Bad (Stand-by) Channels

Edit directly in the table cell by specifying a channel list,
just as you do for saved channels. E.g. 0:3,200,301:305.

The probe API lets you turn off the amplifiers for selected channels; what
imec terms setting the channel to stand-by mode. The channel is still read
out but its amplitude will effectively be zero. This makes graph windows
easier to view. There is another more important way we use the bad channel
list...

SpikeGLX and CatGT observe the shank map that's stored in every recording's
metadata file. A shank map describes:

1. Where that channel's electrode is on the probe.
2. Whether that channel should be `used` in spatial averaging (CAR).

When you click `Verify` or `Run`, SpikeGLX updates the probe's shank map.
These channels are set to (used=0):

* Those marked (used=0) directly in the shank map editor or file.
* On-shank references.
* Listed bad channels.

--------

## Shank Map

This describes two things for each channel:

1. Where that channel's electrode is on the probe. The location is a set
of three indices for the site's (shank, column, row), each zero-based.

2. Whether the channel is "used": shown in displays and included in CAR.

The default shank map is automatically generated from the IMRO table.
**That's really what you want all the time, so don't change that**.
However, you *can* set the used flags in the provided editor.

>The ability to manually edit the locations of sites is mainly useful
when using third-party probes with the NI/Whisper recording system.

--------

## Channel Map

This lets you group/sort/order the channels in SpikeGLX graph windows.
It has no effect on how binary data are stored.

--------

## Save Channels

You can save all of the channels being acquired by setting the list to
any of:

* blank
* '*'
* 'all'

You can save any arbitrary subset of channels using a printer-page-like
list of individual channels and/or ranges, like: 1:200,205,207,350:360.

>The SY channel is always added to your list because it carries error flags
and the sync channel.

### Force LF to Complement Listed AP Channels

(If checked) AND (if this probe type has separate LF channels) THEN your
list of saved channels is edited so that there is a saved LF channel for
each listed AP channel. It's just a convenience.

>The checkbox actually applies to all probes, not just the one being edited.

--------

## Fix Probe ID

This tool allows you to alter the stored serial number and part number
of the selected probe. It is only used for faulty EEPROM chips.

--------

## Default

This is a handy way to set all of the editable fields:

* LED: off.
* IMRO: bank 0, external referencing, medium gain, AP filter on.
* Bad chans: read from the probe database.
* Shank map: layout derived from IMRO, used flags set as described above.
* Channel map: ordered from probe tip to base.
* Save chans: all.

--------

## Copy

Select a probe and copy its settings to a specified other probe, or to all
other probes.

* This only acts between probes of the same `type`.
* This does NOT copy `measured sample rates`.
* This does NOT copy `bad chans`.


_fin_

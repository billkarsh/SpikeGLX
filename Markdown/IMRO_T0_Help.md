## IMRO Table (Type-0)

## Overview

Edit the probe's channel-by-channel settings, for example:

* Which electrode is connected (its bank).
* Which reference is being used (external, tip, on-shank).
* AP and LF gains and filters.

--------

## Default vs Custom Settings

### Defaults

`Default` sets all channels to these values:

* Bank-0.
* External referencing.
* Medium gain (AP = 500, LF = 250).
* AP filter on.

No file is needed with defaults; they are built-in.

### Custom Settings

Customized settings always have to be saved to a file. It has extension
`(.imro) = Imec readout`.

>In all cases, default or custom, the active settings are also
stored in the run-time meta file with tag: `~imroTbl`.

--------

## Big Table

The big table on the left shows a row for each readout channel. You can
type directly into this table to make choices just for that channel.

--------

## Set-All Quick Controls

These are located to the right of the table. These helpers provide a
convenient way to assign a given value to all channels.

--------

## Center Here Quick Control

This positions a 384-channel readout block (region of interest) anywhere
along the length of the shank. You can use it to capture observed activity.
Follow these steps:

1. First decide where the center of interest/activity is on the probe.
You can set all banks to zero, one or two, or load an interesting
long column pattern available from the
[download site here.](https://billkarsh.github.io/SpikeGLX/#interesting-map-files)
Start a run and open the Shank Viewer to see activity. Hover the cursor
over the shank to read out a row number at the center of your region
of interest. Write it down.

2. Open this dialog. Either configure a new run, or right-click in the
Graph Window traces for this probe and select `Edit Banks, Refs, Gains`.
In the dialog, type the row number into the `Row` box and click `Center Here`.
This will select a continuous block of electrodes centered at that row by
assigning the correct bank numbers for you.

3. Optionally, to make your graph ordering reflect the new electrode
assignments, open the `Channel mapping` dialog for this probe. Either
configure a new run, or right-click in the Graph Window traces for this
probe and select `Edit Channel Order`. In the dialog, select
`Shank: Tip to Base` in the `Auto-arrange` menu and click `Apply`.
Now save this channel map. Remember that a channel map is only used to
order the graphs in the viewers, and that to make it take effect, you
have to click the sort button in the Graphs window panel so it reads
`Usr Order`.

--------

## Parameter Details

### Bank

The probes use switches to select the bank that each readout
channel is connected to. The relationship is this:

* electrode = (channel+1) + bank*384; electrode <= 960

![](Probe.png)

### Refid

There are five reference choices [0..4] for each channel.

```
Refid  Referenced site
0      external
1      tip electrode
2      electrode 192
3      electrode 576
4      electrode 960
```

>In practice you should never select the on-shank references (choices
2,3,4) because they don't work well: In saline amplitudes referenced
to these sites are not effectively cancelled.

### Gain

Each readout channel can be assigned a gain factor for the AP and the LF
band. The choices are:

```
50, 125, 250, 500, 1000, 1500, 2000, 3000
```

### AP Filter

Imec channels are separated into two filtered bands as follows:

* LF: [0.5..1k]Hz (fixed).
* AP: [{0,300}..10k]Hz (selectable high pass).

NP 1.0 probes read out both an AP and an LF band.


_fin_


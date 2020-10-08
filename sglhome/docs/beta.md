# Test Versions

* Get test versions on this page.

>This site supports prototype and currently shipping commercial products.</BR>
>The latest commercial PXI-based components are:
>
>* Hardware: `Neuropixels 1.0 probes` (also called phase3B2).
>* Software: `SpikeGLX 3.0` (also called phase30).

------

*There are no test versions at this time*

------

## PXI Module Firmware

A module can only run one flavor of SpikeGLX at a time because it can only
contain firmware of one flavor at a time. To run SpikeGLX-phase3B2 the
hardware module must contain Neuropixels 1.0 (phase3B2) firmware. To run
SpikeGLX-phase20 the module must contain Neuropixels 2.0 firmware. Likewise
for the newest SpikeGLX-phase30.

You can always change the firmware, either to get the latest bug fixes, or
to switch SpikeGLX flavors. SpikeGLX-phase30 ships with its required
firmware package. Older firmware packages are available below: download
the appropriate one and unzip it to your hard drive. Note that
each package contains two files, one for the FPGA on the basestation (BS)
and one for the FPGA on the basestation connect card (BSC). The filenames
contain either 'BS' or 'BSC' so you can tell them apart.

Run SpikeGLX and select menu item `Tools/Update Imec Firmware`. Click
the **?** button in the dialog titlebar to get further help.

* Neuropixels 3.0 (Phase30) Firmware is included in SpikeGLX downloads.

* [Neuropixels 2.0 (Phase20) Firmware](../Support/NP20ModuleFirmware.zip)

* [Neuropixels 1.0 (Phase3B2) Firmware](../Support/NP10ModuleFirmware.zip)


_fin_


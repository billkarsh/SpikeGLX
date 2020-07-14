# Test Versions

* Get test versions on this page.

>The current commercial version is PXI-based `Neuropixels 1.0`, also called `3B2`.

------

## Neuropixels 2.0 (Phase20)

**Latest**:

*Temporarily unavailable*
* [Release 20200520-phase20](../App/Release_v20200520-phase20.zip)...[Readme](../Readme/Readme_v20200520-phase20.txt) : New 2.0 probe names, Imec v2.14

**Previous**:

* [Release 20200309-phase20](../App/Release_v20200309-phase20.zip)...[Readme](../Readme/Readme_v20200309-phase20.txt) : Multidrive output, Imec v2.11

------

## PXI Module Firmware

A module can only run one flavor of SpikeGLX at a time because it can only
contain firmware of one flavor at a time. To run SpikeGLX-phase3B2 the
hardware module must contain Neuropixels 1.0 (phase3B2) firmware. To run
SpikeGLX-phase20 the module must contain Neuropixels 2.0 firmware.

You can always change the firmware, either to get the latest bug fixes, or
to switch SpikeGLX flavors. To do that, get the firmware package you want
to install from this web page and unzip it to your hard drive. Note that
each package contains two files, one for the FPGA on the basestation (BS)
and one for the FPGA on the basestation connect card (BSC). The filenames
contain either 'BS' or 'BSC' so you can tell them apart.

Next run the flavor of SpikeGLX matching the firmware now contained in the
module. In SpikeGLX select menu item `Tools/Update Imec Firmware`. This
dialog lets you detect the version currently installed, and lets you select
new files to load/install. Click the **?** button in the dialog title bar
to get further help.

* [Neuropixels 2.0 (Phase20) Firmware](../Support/NP20ModuleFirmware.zip)

* [Neuropixels 1.0 (Phase3B2) Firmware](../Support/NP10ModuleFirmware.zip)


_fin_


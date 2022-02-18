# Firmware

## FIX: Imec Module Doesn't Work In My Chassis

You've powered everything off as instructed, installed your imec basestation
module into a chassis slot and powered everything on again. You get a red
module status light; the module isn't recognized at all...

It could be a 1-lane (**X1**) slot. Recently we've encountered the fact
that not all PXIe chassis slots are the same with respect to bandwidth.
They come in X1, X2, X4, X8, X16, X32. X1 is sufficient for up to 250 MB/s,
which is completely adequate for an imec module. However, imec originally
designed the module firmware for X4 and higher slots.

As of February 2022, no modules work (as shipped) in X1 slots. Imec plans
to start shipping with X1 capable firmware preinstalled starting late
March 2022.

For now, what if you have a newer NI chassis like the 1088 with five (X1)
and three (X4) slots, or a Keysight M9005A that has only X1 slots? There's
a firmware update on this page to fix the issue on existing modules, but
note the following:

* Get the Phase30 software listed below. *It's only compatible with
SpikeGLX Phase30!!*.

* To update the firmware for X1 compatibility, you first have to install
it into an X4 or higher slot so you can talk to it...

* Check if your chassis design includes at least some X4+ slots. It can be
truly difficult to find that out. NI doesn't mention anything about that
in most chassis 'Datasheets' or 'Specifications.' Rather, it's in the
'User Guide' for some models.

* If you don't have any X4+ slots, maybe a neighbor lab can let you
reprogram a module in their chassis.

>*The NI 1088 is a popular NI choice; the X1 slots are {2,3,5,7,9}, the
X4 slots are {4,6,8}.*

------

## PXI Module Firmware

A module can only run one flavor of SpikeGLX at a time because it can only
contain firmware of one flavor at a time. To run SpikeGLX-phase3B2 the
hardware module must contain Neuropixels 1.0 (phase3B2) firmware. To run
SpikeGLX-phase20 the module must contain Neuropixels 2.0 firmware. Likewise
for the newest SpikeGLX-phase30.

You can always change the firmware, either to get the latest bug fixes, or
to switch SpikeGLX flavors. Download the desired package (below) and unzip
it to your hard drive. Note that each package contains two files, one for
the FPGA on the basestation (BS) and one for the FPGA on the basestation
connect card (BSC). The filenames contain either 'BS' or 'BSC' so you can
tell them apart.

Run SpikeGLX and select menu item `Tools/Update Imec Firmware`. Click
the **?** button in the dialog titlebar to get further help.

>Regardless of the currently installed firmware, we recommend that you
do *ANY* update using the latest SpikeGLX Phase30 application because
that updater works reliably. Yes, even if for example, you are currently
at Phase20 firmware and want to drop back to Phase3B2, we still advise
you to run Phase30 software, select `Tools/Update Imec Firmware`, then
power everything down. After rebooting, run the SpikeGLX application that
pairs with the newly loaded firmware.

* [Neuropixels 3.0 (Phase30) Firmware](../Support/NP30ModuleFirmware.zip)

* [Neuropixels 2.0 (Phase20) Firmware](../Support/NP20ModuleFirmware.zip)

* [Neuropixels 1.0 (Phase3B2) Firmware](../Support/NP10ModuleFirmware.zip)


_fin_


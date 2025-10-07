# Firmware

## FIX: Imec Module Doesn't Work In My Chassis

You've powered everything off as instructed, installed your imec basestation
module into a chassis slot and powered everything on again. You get a red
module status light; the module isn't recognized at all...

It could be a 1-lane (**X1**) slot. In February 2022 we've encountered the
fact that not all PXIe chassis slots are the same with respect to bandwidth.
They come in X1, X2, X4, X8, X16, X32. X1 is sufficient for up to 250 MB/s,
which is completely adequate for an imec module. However, imec originally
designed the module firmware for X4 and higher slots.

Modules have two copies of the firmware. The normal/working copy is loaded,
unless you power-up with the ISP button depressed, in which case the golden
image is loaded. Note the golden image is a backup copy in case the working
copy is corrupted.

In roughly Q2 2022, imec began shipping modules with an X1-compatible
working image, but not an X1-compatible golden image. By Q2 2023, both
images were X1-compatible.

What if you have an older module and a newer NI chassis, for example, a 1083
with five (X1) slots, a 1088 with five (X1) and three (X4) slots, or a
Keysight M9005A that has only X1 slots? There's a firmware solution to fix
the issue, but note the following:

* You have to upgrade to an api4 release; get the latest.

* To update the module firmware for X1 compatibility, you first have to
install it into an X4 or higher slot so you can talk to it...

* Check if your chassis design includes at least some X4+ slots. It can be
truly difficult to find that out. NI doesn't mention anything about that
in most chassis 'Datasheets' or 'Specifications.' Rather, it's in the
'User Guide' for some models.

* If you don't have any X4+ slots, maybe a neighbor lab can let you
reprogram a module in their chassis.

>*The NI 1088 is a popular NI choice; the X1 slots are {2,3,5,7,9}, the
X4 slots are {4,6,8}.*

------

## SpikeGLX Downloads Include Matched Firmware

Each download of SpikeGLX comes with matching firmware.

You can revert to earlier SpikeGLX versions if desired, and of course you
update to newer SpikeGLX versions...

In all cases, when you change SpikeGLX, you should be using the firmware
that was included with that SpikeGLX download.

Note that more recent SpikeGLX versions will tell you if your firmware is
currently correct or not.


_fin_


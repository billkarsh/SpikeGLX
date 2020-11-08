## Updating Firmware

### Dual Boot Images

The flash memory on the BS and BSC each contain two images of boot code;
the **golden** image (used for recovery) and the **normal** image (the
working copy).

* The normal image is loaded in a standard power-on procedure.
* This tool updates only the normal images.

### Power Cycling

Power cycling does not mean restart. Rather, we mean `shut down` the
computer and make sure the chassis power is switched `fully off`,
wait a few seconds, then power them on again. A full shutdown is
required to complete a BS update.

### Typical Update Procedure

1. Select which slot you want to update.

2. Check the box and select the file(s) for a BS and/or a BSC update. The
files for this version of SpikeGLX are in the folder named `Firmware` that
is included in your SpikeGLX release.

3. Click `Update`.

4. Check the Console (Log) window for any error messages.

5. If you updated the BS you need to power cycle everything for the
change to take effect. If you updated only the BSC you need to quit
and restart SpikeGLX for the change to take effect.

6. After cycling power, you'll be able to see which versions are installed
when you click `Detect` in the `Configure Acquisition` dialog.

### Corruption

If anything goes wrong during an update procedure, the normal image may
be corrupted. Symptoms include:

* Detection attempts cause the main status LED on the BS front panel to
switch from green to black (off).

* The module responds slowly or not at all.

* Detection returns error code: `openBS(slot#) error 30 'PCIE_IO_ERROR'`.

* Additional detection attempts may return:
`openBS(slot#) error 2 'ALREADY_OPEN'`.

### Recovery

To get the BS working again you have to update the firmware. To update the
firmware, you need a green light on the front panel of the module.

1. First, try power cycling everything. This won't fix the firmware but it
will often recover enough function to allow the Update operation to access
the module.

2. After the restart, the first thing you should do is use the Update
dialog to select the replacement firmware file(s) and click the Update
button. Do not press `Detect` or try to run before fixing the firmware.

>**Important:**: When the firmware is in a bad state, do not use the
`Detect` button. Doing so will only lock up the BS again and require
another power cycle.

### Secret Button

If that didn't get the update started, power cycle again while depressing
the tiny button labeled `ISP` on the front of the module. This tells the
module to boot the golden image, which should then allow using the Update
dialog as in step (2) above. However, when you use the `ISP` switch like
this, the safest thing to do is to **update both the BS and BSC**.


_fin_


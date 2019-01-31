## Updating Firmware

### Dual Boot Images

The flash memory on the BS and BSC each contain two images of boot code;
the **golden** image (used for recovery) and the **normal** image (the
working copy).

* The normal image is loaded in a standard power-on procedure.
* This tool updates only the normal images.

### Typical Update Procedure

1. In the Update dialog, click 'Detect' to see what versions are currently
installed.

2. Check the box and select the file(s) for a BS and/or a BSC update.

3. Click Update.

4. Check the Console (Log) window for any error messages.

5. If you updated the BS you need to power cycle everything for the
change to take effect. If you updated only the BSC you need to quit
and restart SpikeGLX for the change to take effect.

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

2. After the restart, use the Update dialog to select the replacement
firmware file(s) and click the Update button. Do not press 'Detect'
first (see note below). If the updater is running successfully the
progress bar should be advancing.

>**Important:**: When the firmware is in a bad state, do not use the
'Detect' button either in the configure dialog or in the Update dialog.
Doing so will only lock up the BS again and require another power cycle.

### Secret Button

If that didn't get the update started, power cycle again while depressing
the tiny button labeled `ISP` on the front of the module. This tells the
module to boot the golden image, which should then allow using the Update
dialog as in step (2) above.


_fin_


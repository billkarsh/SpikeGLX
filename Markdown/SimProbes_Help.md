## Simulated Probes

**In brief: Replay one or more files back through online runs.**

Enable a simulated probe by:

1. Selecting an existing imecNNN.ap.bin file.
2. Assigning a (slot, port, dock) address.
3. Enabling it in **both** the `Simulated Probes` dialog and `probe table`.

At run time, SpikeGLX fetches data for this stream from the named file
instead of from hardware. The data then flow through the system in the
same way, and at the same pace, as real probe data would.

--------

## How File Data Are Used

The selected binary data file must have a paired meta file living in the
same directory. The meta data explain how the binary data are organized.
If the .lf. files for a 1.0 probe are also present those are automatically
used to recreate LFP-band data.

Simulation gets hardware version and part number data from the meta file
and shows these values on the `Devices tab` when you click `Detect`. These
version data will be saved into any new meta data that are written during
the run.

Simulation uses the binary data to substitute voltages that would be
fetched from the hardware. However, other parameters such as the IMRO
table, are not adopted from the file. Those are adopted from the real
probe that is selected in the `probe table`. For example, you can use
a 2.0 4-shank probe file to provide simulation data for a 2.0 1-shank
probe because both record 384 output channels. Simulation does not care
which sites were used.

If you wanted to run with the IMRO table from the simulation file,
follow these steps:

1. Open the file in the offline `File Viewer`.
2. Open the `Shank Viewer`.
3. On the Shank Viewer `Edit tab` select `Save`.

--------

## Mix and Match

You can:

* Assign simulated probes to any real hardware probe address.
* Assign simulated probes to a sim slot (see `Configure Slots` dialog).
* Run arbitrary combinations of real and simulated probes.

--------

## Editing

Double-click in table cells to edit the values.

You can enter any integers for (slot, port, dock) that you want. However,
a given table entry will only be enabled/applied in simulation if:

* The (slot, port, dock) address matches an address that is enabled in
the `Devices tab probe table`.

* The entry is also enabled in the `Simulated Probes` table.

>Tip: This lets you keep a catalog of favorite simulation files without
forcing you to apply them now: either uncheck `Enable` or assign an unused
(slot, port, dock) address.


_fin_


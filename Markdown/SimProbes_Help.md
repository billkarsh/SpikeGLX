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


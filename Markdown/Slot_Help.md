## Configure Slots Overview

**In brief: Where did you plug in your probes?**

```
Each probe has a physical address in the system given by its

    (slot, port, dock)
```

1. Use this dialog to select which addresses to show in the `probe table`.
2. Use the `probe table` to select `which probes` to enable in the run.

This document defines terminology, and then covers details of
the `Configure Slots` dialog.

--------

## Important Hardware Note

You can power a Onebox on or off at any time, and you can plug it into
a USB port at any time.

**However, never add/remove physical PXI modules unless the power is OFF.**

--------

## Background: Slots, Ports, Docks and Probes

### Slots

Imec currently makes two kinds of devices that can operate probes:

* PXI "modules," a.k.a. "base stations," a.k.a. "cards."
* USB "Oneboxes," a.k.a. "base stations."

Both types of devices are also called "**slots**," and that is the name you
will see most often in the software. For example, error messages will refer
to a probe by its (slot, port, dock) address.

#### Slot index ranges

* PXI: **[2..18]** inclusive.
* Onebox: **[20..31]** inclusive.

#### Slot index assignment

* PXI: Fixed by position in chassis.
* Onebox: Assigned by you in `Configure Slots` dialog.

### Ports

Each slot has either 2 or 4 USB-C connectors, called "**ports**," where you
can attach the five-meter cables from your probe headstages:

* PXI: **4 ports each**.
* Oneboxes: **2 ports each**.

>PXI and Onebox ports can be used for NP 1.0 or NP 2.0 components.

>Each Onebox can also record 12 non-neural analog and digital channels, and
>these are treated as their own data streams. Onebox streams each:
>
>* Are listed/selected in the `probe table` with port = `ADC`.
>* Are configured on their own `Obx Tab`.
>* Get their own file, e.g., **run_g0_t0.obx3.obx.bin/meta**.

### Docks

A headstage has either 1 or 2 ZIF connectors, called "**docks**," where you
can plug in a probe's FLEX cable:

* NP 1.0 headstages: **1 dock each**.
* NP 2.0 headstages: **2 docks each**.

>Neuropixels 2.0:
>
>You can use just dock #1, or just dock #2, or both, on a NP 2.0 headstage.
>Dock #1 is on the side of the HS populated by two large capacitors and
>the Omnetics connector. Dock #2 is on the backside with the HS label.

### Probe Table

The probe table is on the `Devices tab` of the main `Configure Acquisition`
dialog.

Each row is a (slot, port, dock) address where you can plug in a probe.

Plug probes into one or more of these locations and then check the
`Enable` box for each probe you want to run.

* If you check a box but there isn't a probe there, you'll get an error.
* If you plug in a probe but don't check the box, it won't run.

--------

## Configure Slots Dialog

You can run any desired combination of PXI modules and/or Oneboxes, that
is, any combination of slots.

Use the `Configure Slots` dialog to decide which slots to show in
the probe table and how many docks to show.

### Slot Table

Each row in the table is one slot.

When you open the dialog, it automatically scans the busses for all
detectable devices (plugged in and powered on).

* PXI: The table shows the union of scanned devices and whatever is already
defined in the `probe table` (_Configs/improbetable.ini).

* Oneboxes: The table shows the union of scanned devices and any Oneboxes
you've used before (_Configs/imslottable.ini).

#### Actions

Click on a table row to select it. You can then:

* Edit its slot number (if a Onebox).
* Set how it is shown in the `probe table`.
* Delete it.

### 1 or 2 Docks

If you only use NP 1.0 headstages, which only have one dock, then you
can keep your probe table simple by selecting `1 Dock/port`.

If you need to plug any probes into dock #2 on an NP 2.0 headstage, you
should select `2 Dock/port` to make those addresses visible in the probe
table.

### Detect Button

While the dialog is open you can turn Oneboxes on/off or un/plug them
and then click `Detect` to rescan the bus.


_fin_


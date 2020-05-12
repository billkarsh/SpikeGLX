# Parsing Data Files

## Directory and file names

### Imec probe filenames

* 3A probe filename has no index number after `.imec.`, e.g. `myrun_g0_t0.imec.ap.meta`.
* All later phases have a probe index, e.g. `myrun_g0_t0.imec0.ap.meta`.

***CatGT Tip**: Use option `-prb_3A` for 3A data, and DON'T use `-prb=list`!*

### Run folder

* Prior to SpikeGLX version `20190214` there were no run folders. Rather,
all datafiles output from a run were placed directly into the current
`Data Directory`.

* Version `20190214` and later first create a `Run Folder` within the
data directory. The run folder holds all output from that run, and is
named with a g-index, e.g. `myrun_g0`.

***CatGT Tip**: Use option `-no_run_fld` for runs before version 20190214.*

### Probe folders

**Xilinx-based 3A, 3B1**

These phases are single-probe. No option for probe folders is provided.

**PXI-based 3B2, 2.0**

Version `20190214` and later provide a `Folder per probe` option on the
`Save` tab:

* If not selected, no probe folders are created. Rather, all output files
for a run are saved into its `Run Folder`.

* If selected (checked):
    + All nidq datafiles are saved directly into the `Run Folder`.
    + A separate `Probe Folder` for each imec probe is first created
    in the run folder. The probe folder holds all output from that probe,
    and is named with both g- and probe-indices, e.g. `myrun_g0_imec6`.

***CatGT Tip**: Use option `-prb_fld` if option selected.*

## Multidrive output

### PXI-based 3B2, 2.0

Before the `multidrive` option was introduced SpikeGLX let you specify
at most one `data directory` to hold all your run output folders and files.
As of version `20200309`, there is still a main data directory, but now
you can check a box to enable the splitting of runs across additional
drives/directories to access greater storage capacity when doing very
long runs or when using many probes.

**Modulo rule**

The rule for distributing run output into several data directories is this:

* As always, the (N) probes have logical indices `[0..N-1]`.
* The main data directory is `dir-0`.
* The set of all (M) directories, including the main one are enumerated `[dir-0..dir-M-1]`.
* NI output is always sent to dir-0.
* Probe-j output is sent to `dir-(j modulo M)`.
* Within each dir-k SpikeGLX will create a `run folder` and applicable `probe folders`
according to the same rules that apply for a single main data directory.

>`A Modulo B` means "take the remainder of dividing integers `A/B`". Modulo
is often written `mod` and is written `%` in C/C++ and Python code.

For example, suppose these parameters:

* dir-0 = `D:\Data`
* dir-1 = `E:\DataB`
* dir-2 = `G:\DataC`
* runname = `myRun`
* folder per run is enabled

Example output would go here:

* NI: `D:\Data\myrun\myrun_g0_t0.ni.bin(.meta)`
* Imec0: `D:\Data\myrun\myrun_g0_imec0\myrun_g0_t0.imec0.ap.bin(.meta)`
* Imec3: `D:\Data\myrun\myrun_g0_imec3\myrun_g0_t0.imec3.ap.bin(.meta)`
* Imec4: `E:\DataB\myrun\myrun_g0_imec4\myrun_g0_t0.imec4.ap.bin(.meta)`
* Imec8: `G:\DataC\myrun\myrun_g0_imec8\myrun_g0_t0.imec8.ap.bin(.meta)`

### New offline behaviors

When multidrive output is enabled, a run's files are distributed over
several directories. The SpikeGLX features and tools that work on sets
of run files are affected as follows:

* **FileViewer/Link**: To link files in the same run, that is, to show them
and scroll them together, you first open any binary file from a run, then
choose `File/Link` in the Viewer window. In the Link dialog you list which
other streams in this run to open and link together. This feature only knows
how to search for and link other streams that reside in the same parent
`data-dir/run-folder` as the open file. If the multidrive option had split
the run, you will only be able to link the streams that live together in
one of the split run folders. You'll have to separately view and link data
that live in separate data directories.

* **Tools/Sample Rates From Run**: This dialog still looks the same. You
select any binary file from a run and you have an option to calibrate just
that data stream, or all of the streams it can find in the same run folder.
If the multidrive option had split the run, this tool will only be able to
locate and calibrate probes it can find in the same split run folder as the
selected binary. That is, you'll have to repeat this process once for each
separate data directory.

* **CatGT**: Tell CatGT which probes to process by pointing to a data
directory `-dir=myDir` and listing the probes you want to process, say
`-prb=0:3`. In CatGT version 1.2.7 and earlier, if any probe is missing
from that directory, the run stops and logs a missing file error. As of
CatGT version `1.2.8` you can add option `-prb_miss_ok` telling CatGT
to skip missing probes and continue with the next one it finds in that
directory. Note that you will have to make a separate command line for
each data directory, but each command line can conveniently list all of
the probes in the run. You don't have to figure out the modulo rule for
each directory.

* **TPrime**:
Unaffected. All of the input and outfile files {tostream, fromstream, events}
already have independently specified paths.

## Metadata differences by phase

> This isn't an exhaustive list of differences. Rather, this is what
we parse in our own code.

### Phase 3A vs later:

**Filename**

* 3A probe filename has no index number after `.imec.`, e.g. `myrun_g0_t0.imec.ap.meta`.
* All later phases have a probe index, e.g. `myrun_g0_t0.imec0.ap.meta`.

**Metadata**

* 3A contains key `typeEnabled` with string values from list `{imec, nidq}`.
* All later phases instead contain keys `{typeIMEnabled, typeNIEnabled}` with
integer counts of those streams.

### Phase 3B1 vs later:

**Metadata**

* 3B1 does not have keys `{imDatPrb_port, imDatPrb_slot, syncImInputSlot}`.

### Phase 2.0:

**Metadata**

* 2.0 introduces keys `{imDatPrb_dock, imMaxInt}`

## Probe type

* 3A contains key `imProbeOpt`, an integer prototype probe `option code {1,2,3,4}`.
* All later phases instead contain key `imDatPrb_type`, an integer
encoding a production probe's unique feature set.


_fin_


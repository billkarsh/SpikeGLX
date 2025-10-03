# SpikeGLX Output Directories and Files

## Filenames

Filenames are decorated with several indices.

### Gate and trigger indices

All files are created under the control of SpikeGLX gate and trigger
modes, which determine when to start writing files. When file writing
starts, all files for all streams will get tagged by the current g- and
t-indices so you know they go together as a set. They will also be grouped
together inside a folder that is named with the run name and the current
g-index.

An important characteristic of all of the files in the same `Run_g*` folder
is that they start writing at the same time. Said another way, they are
time-aligned at their starts if you had sync enabled.

All of the files from the same run have a special relationship to each other.
During a run the settings are held constant and the data continue to stream.
The files that are written are just snapshots from the continuous stream.
The files contain a metadata item `firstSample` that identifies where in
that continuum this file was snapped. This allow the files saved from the
same run to be concatenated in a way that reconstructs the true timeline.
The [CatGT](https://billkarsh.github.io/SpikeGLX/#catgt) tool can perform
time-preserving join operations.

### Probe index

* 3A probe filename has no index number after `.imec.`, e.g. `myrun_g0_t0.imec.ap.meta`.
* All later phases have a probe index, e.g. `myrun_g0_t0.imec0.ap.meta`.

***CatGT Tip**: Use option `-prb_3A` for 3A data, and DON'T use `-prb=list`!*

## Directories

### Data directory

At the highest level is the `data directory` which you set in SpikeGLX
using menu item `Options::Choose Data Directory`. All output data will
be stored into this directory.

>Note: You can split data into several top-level directories using the
[`Multidrive Feature`.](#multidrive-output)

### Run folder

* Version `20190214` and later first create a `Run Folder` within the
data directory. The run folder holds all output from that run, and is
named with a g-index, e.g. `myrun_g0`.

* Prior to SpikeGLX version `20190214` there were no run folders. Rather,
all datafiles output from a run were placed directly into the current
`Data Directory`.

***CatGT Tip**: Use option `-no_run_fld` for runs before version 20190214.*

### Probe folders

**Xilinx-based 3A, 3B1**

These phases are single-probe. No option for probe folders is provided.

**Everything else (3B2, 1.0, 2.0, ...)**

Version `20190214` and later provide a `Folder per probe` option on the
`Save` tab:

* If not selected, no probe folders are created. Rather, all output files
for a run are saved into the `Run Folder`.

* If selected (checked):
    + All nidq and obx datafiles are saved directly into the `Run Folder`.
    + A separate `Probe Folder` for each imec probe is first created
    in the run folder. The probe folder holds all output from that probe,
    and is named with both g- and probe-indices, e.g. `myrun_g0_imec6`.

***CatGT Tip**: Use option `-prb_fld` if option selected.*

## Multidrive output

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
* NI and obx output is always sent to dir-0.
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
* obx0: `D:\Data\myrun\myrun_g0_t0.obx0.obx.bin(.meta)`
* obx1: `D:\Data\myrun\myrun_g0_t0.obx1.obx.bin(.meta)`
* Imec0: `D:\Data\myrun\myrun_g0_imec0\myrun_g0_t0.imec0.ap.bin(.meta)`
* Imec3: `D:\Data\myrun\myrun_g0_imec3\myrun_g0_t0.imec3.ap.bin(.meta)`
* Imec4: `E:\DataB\myrun\myrun_g0_imec4\myrun_g0_t0.imec4.ap.bin(.meta)`
* Imec8: `G:\DataC\myrun\myrun_g0_imec8\myrun_g0_t0.imec8.ap.bin(.meta)`

>Note: You can run several OneBox ADC streams, which would be named 'obx0,'
'obx1,' etc. All obx\* data are tiny so we put all obx files in the main
dir-0 directory.

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

## Parsing data by probe

The safest and most reliable way to process your data is to use our tools:

* [CatGT](https://billkarsh.github.io/SpikeGLX/#catgt)
* [Other post-processing tools](https://billkarsh.github.io/SpikeGLX/#post-processing-tools)

If you are determined to roll your own parsing code, you will need to
identify the type of probe used. Once identified, you can look up probe
characteristics and parameters in the published
[**probe table**](https://github.com/billkarsh/ProbeTable).

The metadata key that identifies a probe is its part number `imDatPrb_pn`.
Virtually all meta files later than phase 3A will contain this value.


_fin_


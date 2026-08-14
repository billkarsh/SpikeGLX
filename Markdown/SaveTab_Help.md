## Save Tab

Use this tab to:

* Save annotation notes into your metadata files.
* Set where to save and what to name the output data files.
* Check the available disk space, hence, maximum possible recording time.

--------

## Run Name and Run Continuation

You can set the name for your run either in this tab's `Run name` box or
in the run toolbar at the top of the Graphs window. In the Graphs window
you must first pause writing via the optional `Enable/Disable Recording`
button as described [here](GateTab_Help.html#gate-manual-override).

Both of these inputs accept either an undecorated base-name for a run,
or a decorated name of the form `run-name_gN_tM`. The decorated form tells
SpikeGLX you wish to continue writing additional files into an existing
run, starting at the specified g/t indices. This is very useful if a run
had to be interrupted to repair a problem.

Note too, that you can change to a different run name without stopping the
run. Use the Graphs window `Disable` button to pause writing, then type in
a new undecorated name (no g/t indices). This will be treated as a request
for a brand new run name that will start at _g0_t0.

### Unique Name Rules

If the run name is **un**decorated, the provided base-name must be unused
in all of the current `Data Directories`.

If the name is decorated, then those indices must be unused. That is,
`run-name_gN_tM` must not exist in any of the current `Data Directories`.

### Folder per probe

Create a run subfolder for each probe.

Suppose you have two 2.0 probes {0,1} in myrun...

Box unchecked, flat file organization in run folder:

```
data_dir/
    myrun_g0/
        myrun_g0_t0.imec0.ap.bin
        myrun_g0_t0.imec0.ap.meta
        myrun_g0_t0.imec1.ap.bin
        myrun_g0_t0.imec1.ap.meta
        // other non-probe stream files
```

Box checked, probe subfolders:

```
data_dir/
    myrun_g0/
        myrun_g0_imec0/
            myrun_g0_t0.imec0.ap.bin
            myrun_g0_t0.imec0.ap.meta
        myrun_g0_imec1/
            myrun_g0_t0.imec1.ap.bin
            myrun_g0_t0.imec1.ap.meta
        // other non-probe stream files
```

### Separate shanks

Write separate bin/meta for each shank of your 4-shank probes...

- This only affects 4-shank probes.
- Each 4-shank probe has an original probe-ip.
- Multidisk directory selection is based on original ip.
- Folder per probe option is based on original ip.
- The filename for {ip,shank-s} is: `1000 + 10*ip + s`.
- Only non-empty files are written.
- Adjusted metadata reflect intersection of {original saved chans, chans on that shank}.
- The original probe-ip meta file is also written as a settings record.

Suppose you have two 2.0 probes {0,1} in myrun, and probe-1 is 4-shank...

- Folder per probe: checked.
- Separate shanks: checked.

```
data_dir/
    myrun_g0/
        myrun_g0_imec0/
            myrun_g0_t0.imec0.ap.bin
            myrun_g0_t0.imec0.ap.meta
        myrun_g0_imec1/
            myrun_g0_t0.imec1.ap.meta       // ref meta named per original ip
            myrun_g0_t0.imec1010.ap.bin
            myrun_g0_t0.imec1010.ap.meta
            myrun_g0_t0.imec1011.ap.bin
            myrun_g0_t0.imec1011.ap.meta
            myrun_g0_t0.imec1012.ap.bin
            myrun_g0_t0.imec1012.ap.meta
            myrun_g0_t0.imec1013.ap.bin
            myrun_g0_t0.imec1013.ap.meta
        // other non-probe stream files
```


_fin_


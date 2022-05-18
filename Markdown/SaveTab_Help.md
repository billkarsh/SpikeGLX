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


_fin_


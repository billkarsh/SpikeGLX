## Graphs Window Tools

### Second Graphs Window

In the Console window menus choose `Window/More Traces (Ctrl+T)` to open
a second Graphs window after a run has started. Only the main Graphs window
has run controls and LED indicators for gate and trigger status, but all
of the stream viewing and filtering options otherwise work the same in both
windows.

### Run Toolbar

* `Stop Acquisition`: Stops the current run and returns the software to an idle state.
You can do the same thing by clicking the `Graph Window's Close box` or by
pressing the `esc` key, or by choosing `Quit (Ctrl+Q)` from the File
menu (of course the latter also closes SpikeGLX).

* `Acquisition Clock`: The left-hand clock displays time elapsed since the run started
and first samples were read from the hardware.

* `Enable/Disable Recording`: This feature is available if you select it
on the `Gates tab`. Use this to
[pause/resume](GateTab_Help.html#gate-manual-override)
the saving of data files, to change
[which channels are being saved](#save-channel-subset)
to disk files, or to
[edit the name of the run](SaveTab_Help.html#run-name-and-run-continuation)
and its disk files.

* `Recording Clock`: The right-hand clock displays time elapsed since the current file
set was opened for recording.

* `Notes`: Edit your run notes. These are stored as `userNotes` in the
metadata.

* `Pause`: The Pause VCR-style button toggles between pause and play of
stream data in the graphs so you can inspect an interesting feature.
This does not pause any other activity.

### Stream Toolbar Controls

* `AP0;0`: The name of the currently selected graph. Single-click on
any graph to select it. The current graph is the target of several
controls such as the axis scaling boxes. Click on the channel-name
string in the toolbar to switch to the page containing the current selection.

* `Expand`: This button toggles between showing just the selected channel,
and the standard multi-channel view. You can also double-click a graph
to select it and expand/contract it.

* `Sec`: Enter a value in range [0.001..30.0] seconds to set the time span.

* `Yscl`: Enter a value in range [0.0..9999.0] to set a vertical magnification
for the selected graph.

* `Clr`: Click this button to get a Color picking dialog whereby you can
define an alternate color for the **data trace** in the selected graph.
This only works for analog channels; digital traces are auto-colored.

* `Apply All`: Copies Yscl from the selected graph to all other graphs
of the same category.

#### Filters Applied Only to Neural Channels

Notes:

* These filters only affect the appearance of graphs, not saved data.
* These filters are **not** applied to non-neural channels.
* If ever you are suspicious that hardware is not working, turn all the
filters off to understand what is coming out of the hardware.

##### General Filters

* `-<Tn>` (`-<Tx>`): Neural (auxiliary analog) time averaging. Samples the
data stream per channel to calculate and then subtract the time average
value; effectively subtracting the DC component. The value is updated every
5 seconds. This may create artifactual steps at update boundaries.

* `-<S>`: Spatial averaging. At each timepoint a neighborhood of electrodes
per channel is averaged; the result is subtracted from that channel. The
locations of electrodes are known from your imro table or nidq shank map.
    + Notes:
        1. Certain electrodes are omitted from the average: {Those marked
        'use=false' in your map, imec reference electrodes, imec electrodes that
        are turned off}.
        2. Only AP-band channels are affected.
        3. Neighborhoods never cross shank boundaries.
    + There are four choices of neighborhood:
        + `Loc 1,2`: Small annulus about the channel's electrode;
            e.g., inner radius=1, outer=2.
        + `Loc 2,8`: Larger annulus about the channel's electrode;
            e.g., inner radius=2, outer=8.
        + `Glb All`: All electrodes on probe.
        + `Glb Dmx`: All electrodes on probe that are sampled
            concurrently (same multiplexing phase).

* `BinMax`: If checked, we report the extrema in each neural channel
downsample bin. This assists spike visualization but exaggerates apparent
background noise. Uncheck the box to visualize noise more accurately.

##### Imec Stream Filters

The `imro` table determines individually for each AP channel if a 300 Hz
high pass filter is applied in hardware. The result is the `native` AP
signal. In addition, LF band signals are always acquired from the hardware.
The filter selector applies software operations to these `native` signals.

* `AP Native`: No software filter is applied.
* `300 - INF`: A software high pass filter is applied to all AP channels.
* `0.5 - 500`: A software low pass filter is applied to all AP channels.
* `AP = AP+LF`: All AP channels are graphed as the simple sum of the AP
and corresponding LF band signal.

##### Nidq Stream Filters

* `Bandpass`: Applies optional bandpass filtering to neural MN
channels.

### Page Toolbar Controls

* `Acq/Usr Order`: This button toggles between acquired (standard)
channel order and that specified by your custom [channel map](#channel-map).

* `Shank View`: Opens the [Shank Viewer](ShankView_Help.html) window for
this stream.

* `NChan`: Specifies how many graphs to show per page. When the value is
changed SpikeGLX selects a page that keeps the middle graph visible.

* `1st`: Shows the index number of the first graph on the current page.

* `Slider`: Change pages.

### Right-Click on Graph

For either stream:

* `Listen Left/Right/Both Ears`: Listen to selected channel (immediately).

* `Edit Channel Order...`: Edit the ChanMap for this stream.

* `Edit Saved Channels...`: Edit the string describing which channels
are saved to file. Note that saved channels are marked with an `S` on the
right-hand sides of the graphs.

* `Refresh Graphs`: Clear and restart sweeping for all graphs.

* `Color TTL Events...`: Watch up to 4 auxiliary (TTL) channels for
pulses. Apply color stripes to the graphs when pulses occur.

#### Imec Menu

* `Spike Viewer 1..4`: Display spiking in selected channel (immediately).

* `Edit Channel On/Off...`: Shows editor for changing which channels are
turned off (available only if recording currently disabled).
All data and views are updated to reflect your changes.

### Other Graph Window Features

* Hover the mouse over a graph to view statistics for the data
currently shown.

* Use the `L` and `R` controls in the lower right of the window to
display just one stream in the window `L`, or two. You can place two
streams either side-by-side `L | R` or above and below `L / R`.


_fin_


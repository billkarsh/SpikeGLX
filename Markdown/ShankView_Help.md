## Shank Viewer

* See activity directly on the probe.
* Navigate between probe and graphs.
* Edit IMRO tables.

**Topics:**

* [Edit Tab](#edit-tab-quick-reference)
* [View Tab](#view-tab-quick-reference)

### Edit Tab Quick Reference

* The editor is resizable.
* `Load` and view an existing file without applying.
* `Default` to {bank-0, ext ref, std gains, std filter} without applying.
* `Save` as a file without applying.
* `OK` applies changes to new configuration.
* `Cancel` reverts to prior configuration.
* `Apply` changes to current run.
* `Revert` to current settings.
* {Reference, Gains, Filter} are applied to all channels.
* Change the magnification with the `Pix/pad` box.
* You can tab among the items in the editor.
* When the scrollbar has focus you can navigate along the probe with the
arrow or page keys.

### Boxes

Define your region(s) of interest:

1. Set the `Boxes` menu to the number of desired regions.
2. Click on the probe to place the centers of boxes.

* Dark outlines indicate excluded sites.
* Boxes snap into place next to existing boxes or forbidden zones.
* Click in an existing box to delete it.
* `Clear` removes all boxes.
* Reducing the `Boxes` count will remove all existing boxes.
* Increasing the `Boxes` count will evenly split all existing boxes.

>Note: Each probe type may limit the maximum number of boxes available,
and may impose an internal grid that prevents placing boxes at illegal
locations.

>Tip: It might become tedious placing 16 boxes on some probe types due
to their complicated selection rules. Make your life easier by using the
smallest `Boxes` count that adequately samples the activity. Also, this
strategy can help: Use the splitting feature. Set `Boxes` small to start
and place a full set (perhaps, 4) over the activity. Next, increase
`Boxes`, which splits those into 8. Finally move some of the smaller
boxes to better locations. Split again to get to 16 if you need to.

--------

### View Tab Quick Reference

* The viewer is resizable.
* The viewer stays in front of its Graphs window.
* Change the magnification with the `Pixels/pad` box.
* Click on a graph to highlight/select that pad in the viewer.
* Click on a viewer pad to select that graph.
* `For dual-band probes, like NP 1.0,` shift+click on a viewer pad to select LF.
* The spike (negative going) threshold `T` and antibounce `Stay low` settings
only apply to spike detection.
* Increase the sampling/display `Update (s)` interval to visualize infrequent activity.
* Click on the button with the name of the currently selected channel
to scroll that pad into view.
* You can tab among the items in the viewer.
* When the scrollbar has focus you can navigate along the probe with the
arrow or page keys.

### Online Data Flow

`Raw data -> Fetcher -> Graphs -> Shank`

Every tenth of a second a process thread called the 'graph fetcher' grabs
the next chunk of raw data from the main data stream and pushes it to the
Graphs window. The stepping of the visible time cursor across the screen
heralds these events.

If the corresponding Shank Viewer is visible, the Graphs window pushes a
copy of the unmodified raw data chunk to the Shank Viewer. Importantly,
the filter choices in the Graphs window do not affect the Shank Viewer;
each does its own filtering/processing.

Note that clicking `Pause` in the Graphs window pauses/resumes both the
graphs and the shank activity mapping.

By default, both the online and offline Shank Viewers apply a
`300Hz highpass` and `global CAR` filter to AP-band channels and a
`0.2Hz - 300Hz bandpass` filter to the LFP channels. These filters
reduce out-of-band electrical noise, including DC offsets, which allows
better comparison to your requested spike threshold voltage.

As of SpikeGLX version 20230425, the Configuration dialog/IM Setup tab has
a box to set up a filtered probe AP-band stream that runs in parallel with
the raw stream. You can set the bandpass edges for this stream, and the
stream automatically gets global `demux` CAR. These data, when present,
are used to improve signal-to-background in both audio output and Shank
Viewers (for {spike, AP pk-pk} calculations). There is a checkbox to disable
this feature just in case you are running out of RAM or CPU, but we don't
think this will be necessary unless you are running 12 probes or more at the
same time.

### File Viewer Data Flow

`Average current window` applies a 300Hz highpass and global CAR filer,
then counts spikes and tallies peak-to-peak voltages over the data currently
displayed in the File Viewer window. The Shank Viewer updates as you scroll.

`Sample whole survey` is specifically for viewing survey result files. It
averages up to four half-second time chunks drawn from each of the banks
on the probe to build a whole-probe activity map. You have to click `Update
whole survey` to trigger the calculation. If you change the spike threshold
or 'stay low' values you need to click `Update` again. Peak-to-peak calculation
has no parameters so only one `Update` click each is needed.

### Setting a Spike Threshold

You can read an appropriate threshold level from a graph:

1. Select a graph and double-click to blow it up.
2. Turn off all filters except `300 - INF` (approximates Shank View filtering).
3. Mouse over the graph; read the voltage in the status bar.
4. Be conservative, set a value ~75% of the peak.

### Setting 'Stay low'

This value gives you a rough spike width filter. Our spike detection
logic requires that the signal cross the threshold (from high to low)
and continuously stay low for at least this many samples.

Unfortunately, if you've got a lot of electrical noise, the signal could
cross back and forth rapidly across the threshold. The detector thinks
such spikes are narrow and they are rejected if 'stay low' is too high.

If your spike rates seem too low, try lowering 'stay low'.

To directly examine the noise in a selected graph:

1. Double-click to blow it up.
2. Set a very fast time scale like 0.01s for better resolution.
3. Use the `Pause` button to freeze the display for a better look.

### Anatomy Colors

This item group interacts with the Pinpoint and Trajectory Explorer
real-time anatomy programs. The box receives a list of color-coded
region labels. The checkboxes let you apply anatomy color separately
to the shanks and to the traces in the Graphs Window.

### Right-click

Right-click on a viewer pad to select more channel-specific options.


_fin_


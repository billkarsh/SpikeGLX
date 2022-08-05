## Shank Viewer

* See activity directly on the probe.
* Navigate between probe and graphs.
* Edit IMRO tables.

**Topics:**

* [Edit Tab](#edit-tab-quick-reference)
* [View Tab](#view-tab-quick-reference)

### View Tab Quick Reference

* The viewer is resizable.
* The viewer stays in front of its Graphs window.
* Change the magnification with the `Pixels/pad` box.
* Click on a graph to highlight/select that pad in the viewer.
* Click on a viewer pad to select that graph.
* `For imec probes,` shift+click or right-click on a viewer pad to select
its LF graph.
* The spike (negative going) threshold `T` and antibounce `Stay low` settings
only apply to spike detection.
* Increase the sampling/display `Update (s)` interval to visualize infrequent activity.
* Click on the button with the name of the currently selected channel
to scroll that pad into view.
* You can tab among the items in the viewer.
* When the scrollbar has focus you can navigate along the probe with the
arrow or page keys.

### Data Flow

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

The Shank Viewer applies a `300Hz highpass` filter to AP band channels and
a `0.2Hz - 300Hz bandpass` filter to the LFP channels. These filters reduce
out-of-band electrical noise, including DC offsets, which allows better
comparison to your requested spike threshold voltage.

### Setting a Spike Threshold

You can read an appropriate threshold level from a graph:

1. Select a graph and double-click to blow it up.
2. Turn off all filters except `-<T>` (approximates Shank View filtering).
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

### Edit Tab Quick Reference

* The editor is resizable.
* `Load` an existing file.
* `Default` sets bank zero with standard reference, gains and filters.
* `Save` a non-default table as a file.
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

_fin_


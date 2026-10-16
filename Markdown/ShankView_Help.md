## Shank Viewer

* See activity directly on the probe.
* Navigate between probe and graphs.
* Edit IMRO tables.

[**New: 20260901**](#new-20260901)

**Topics:**

* [Edit Tab](#edit-tab-quick-reference)
    + [Quick Ref](#edit-tab-quick-reference)
    + [Editing Basics](#editing-basics)
    + [Boxes](#boxes)
        + [Readouts](#readouts)
        + [Making New Boxes](#making-new-boxes)
        + [Resizing Boxes](#resizing-boxes)
        + [Clearing/Erasing Boxes](#clearingerasing-boxes)
        + [Boxes => file chans](#boxes-file-chans)
    + [Third Party Tools](#third-party-tools)
* [View Tab](#view-tab-quick-reference)
    + [Quick Ref](#view-tab-quick-reference)
    + [Online Data Flow](#online-data-flow)
    + [File Viewer Data Flow](#file-viewer-data-flow)
    + [Setting a Spike Threshold 'T(-uV)'](#setting-a-spike-threshold-t-uv)
    + [Setting 'Stay low'](#setting-stay-low)
    + [Anatomy Colors](#anatomy-colors)
    + [Right-click](#right-click)

--------

## New: 20260901

(1) Bug-fix: Easier to place half-width boxes on the right.
(2) New `Method` menu adds capabilities...

Set the `method` to `Lclk Center Green Here` and the behavior is the same
as before. Placement of green ROI boxes works the same, and blue conflict
boxes work the same.

Set `method` to `Lclk Where => Rclk What` to make a cyan box, then right-click
the cyan box for a pop-up menu of options:

**Half- or Full-width cyan box**

- **Max Box Length**: Extend cyan box to full shank length.
- **Fill All Rows**: Place green at all available sites.
- **Fill Even Rows**: Ditto, only on even rows.
- **Fill Odd Rows**: Ditto, only on odd rows.
- **Delete Green**: Erase any green inside the cyan box.

**Smart fills, full-width only**

- **Linear; Row Step 1**: Single linear "column" as long as possible.
- **Linear; Row Step 2**: Ditto, every other row.
- **Staggered; Row Step 1**: Checkerboard as long as possible.
- **Staggered; Row Step 2**: Ditto, every other row.

Filling works from tip to top and observes the `Rqd` maximum.

## Edit Tab Quick Reference

* The editor is resizable.
* `Load` and view an existing file without applying.
* `Default` to {bank-0, ext ref, std gains, std filter} without applying.
* `Save` as a file without applying.
* `OK` applies saved changes to new configuration.
* `Cancel` reverts to prior configuration.
* `Apply` saved changes to current run.
* `Revert` to current settings.
* {Reference, Gains, Filter} are applied to all channels.
* Change the magnification with the `Pix/pad` box.
* You can tab among the items in the editor.
* When the scrollbar has focus you can navigate along the probe with the
arrow or page keys.

### Editing Basics

A typical probe has 1000+ electrode sites, but, to keep the probe thin,
compact, lightweight, and preserve high sample rate, you must select a
smaller subset (typically 384) for readout.

The editor helps you visually select the sites you want: your region of
interest (ROI). The ROI, your differential referencing choice, and a few
other channel parameters are stored in an `imro table` data structure,
and these are made portable by storing them in `imro files.`

The editor can be accessed from many situations:

- When configuring a run, [here](IMTAB_Help.html#editing).
- During an online run, [here](Graphs_Help.html#page-toolbar-controls).
- In the offline [FileViewer](FileViewer_Help.html#shank-viewer).

> *You can use the included **SpikeGLX_NISIM.exe** to access the offline
editor from any Windows PC. No need to install NI drivers or any additional
environments or tool/library packages.*

> *A strictly offline, **Linux version of SpikeGLX_NISIM** is available
[here](https://billkarsh.github.io/SpikeGLX/#post-processing-tools).*

How to choose sites?

**Looking at overlays is best...**

The best way is to (1) Do a survey run to sample activity everywhere
on the probe. (2) Review the survey in the FileViewer which paints an
activity heatmap on the probe. Use that context to select sites having
desired levels of activity, or desired responses to stimuli. Learn about
that [here](IMTAB_Help.html#survey-all-probe-banks).

You can overlay the ShankViewer with [anatomy data](UserManual.html#anatomy-pinpoint-trajectory-explorer). 
Use that context to select sites in relevant brain regions. If using Pinpoint,
anatomy can be applied in online or offline ShankViewers. Anatomy colors the
space outside of shanks, so you can see anatomy and activity at the same time.

During an online run, before you start recording files, you can open the editor
tab and try several different ROIs to hunt for desired activity. This affords
some degree of feedback.

If you don't have a clue yet, use a standard guess:

- Use the predefined `Default` imro, which is random, but super easy.
- Try a compact region on just one shank.
- Try a compact region near each shank tip.
- Try a sparse set of sites spanning the full probe length.

### Boxes

In the editor you don't need to click on 384 individual sites, though you can.
Rather, you work with larger boxes of sites. There are several types of boxes:

- **Green**: These are your selected ROI boxes. You'll create as many green boxes
as needed to lasso interesting clusters of sites. The editor guides you so you
won't select more than the required site count (usually 384), and you won't put
boxes in forbidden places.

- **Heavy Black**: Every probe has selection rules: If you choose this site, you
can't select those. As you make green boxes, the corresponding excluded sites are
automatically outlined with heavy black boxes. This shows you which areas of the
probe remain available for new green boxes. When your required site count is
reached all the black boxes disappear, meaning "you're done."

- **Cyan**: The recently added `Where/What` method uses cyan to indicate a
potential action box. Right-click in a cyan box to get the pop-up menu. Fill
the available space with new green boxes (several fill patterns available),
or, delete the overlapped green space, so you can make alternate green boxes
elsewhere. The filling actions work from tip toward the top and will not go
over the `Rqd` limit. The cyan box is erased when an action completes.

- **Blue**: These are visual aids that do not affect your site count. They help
with conflict resolution. For example, you've created a number of green boxes
and you see that a key activity hot spot is now in a forbidden area but you really
want to include it...you'll have to undo some of the green, but which green is
involved? Control-click a blue box over the hot spot and all related sites are
outlined with heavy blue boxes. You'll be able to see what needs adjustment.

#### Readouts

The `Rqd` readout reminds you how many total channels/sites you must
select to make a valid table. Most probes have 384 channels that are
shared over the whole probe surface. Some probes have fewer than 384
readout channels, NP3 probes have 912 or 1536 channels, and quad-probes
have 384 channels on each of the four shanks. In all cases, Rqd tells
you the required total channel count.

`Sum` is the channel count summed over your current set of boxes. You are
done making a valid ROI when **Sum = Rqd**.

When you click in an existing box `Sel` reads out its size, in channels.

#### Making New Boxes

**All Boxes**

1. `Nrows` sets the height (in rows) of new boxes.
2. Set the `Width` either to full- or half-shank.
3. Click where you want to place the **center** of the new box.

> *You can left- or right-click to make the new box.*

**Green: Lclk Center Green Here**

Clicking directly makes green boxes:

* If a box overlaps a forbidden zone or another box, the box slides to available space.
* If a box goes off the top or bottom of the shank, the box slides to available space.
* If a box won't fit where you clicked, the box is downsized.

**Cyan: Lclk Where => Rclk What**

Clicking makes a cyan action box.

* If a box goes off the top or bottom of the shank, the box slides to available space.
* The box can overlap all other boxes.
* You can only make one cyan box at a time.

> *Left-click to make a box (handy for resizing), right-click to make the box and
immediately show the menu (fewer steps).*

**Blue: Control-click**

* If a box goes off the top or bottom of the shank, the box slides to available space.
* The box can overlap {green, cyan, black} boxes; not blue.
* You can make more than one blue, but the view gets crowded.

#### Resizing Boxes

**All Boxes**

* `Click-and-drag` in the upper-half of a box to adjust its top edge.
* `Click-and-drag` in the lower-half of a box to adjust its bottom edge.
* The view will scroll as you drag beyond the top/bottom edge.
* Drag off the side of the shank to cancel and revert to the original size.
* Release the mouse button to finish the operation.

**Green: Lclk Center Green Here**

* The size is limited by other objects, and by Rqd.

**Cyan: Lclk Where => Rclk What**

* Menu item `Max Box Length` extends the box to the full shank height.

#### Clearing/Erasing Boxes

* The `Clear All` button erases green and cyan boxes; not blue.
* `Shift-click` in any box to clear it.

>*Note: Blue boxes only go away when you manually shift-click in them.
Otherwise they stay put even if you load alternative IMRO files.*

#### Boxes => file chans

* Click this button to turn the current set of boxes into a selective 'save
channel subset' string. That is, change the set of channels to save in data
files. You can use this to lasso activity hotspots when you use the feature
either online or from the offline File Viewer.
* You can do this as long as the current box `Sum` is nonzero.
* It's the same 'Save Channels' dialog you'd see from the `IM Setup` tab,
but the current list of channels reflects your current boxes.
* If you use this from the Configure dialog or during a run, the result is
applied to current run settings. If you use this from the offline File Viewer,
the resulting string is merely copied to the clipboard.

### Third Party Tools

Recently, other developers are making imro editor tools:

- [PixelMap](https://github.com/m-beau/pixelmap)
- [NeuroCarto](https://neurocarto.readthedocs.io/en/latest/)

> *We do not test, endorse, or attest to the fitness of third party tools.*

--------

## View Tab Quick Reference

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
arrow or page keys, or the mouse wheel. Hold the shift key down to wheel
faster.

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

When data are drawn from the raw streams, the Shank Viewers apply a 
`300Hz high-pass` filter to AP-band data, and a `0.2Hz - 300Hz bandpass`
to LF-band data. No CAR filtering is done on raw online streams.

Use the `Configuration dialog/IM Setup tab` to enable prefiltered AP-band
streams that run in parallel with the raw streams. You can select the
AP-bandpass edges. Global demux is always applied. Using this filtered
stream improves signal-to-background in both audio output and Shank
Viewers (for {spike, AP pk-pk} calculations). There is a checkbox to disable
this feature just in case you are running out of RAM or CPU, but we don't
think this will be necessary unless you are running 12 probes or more at the
same time.

### File Viewer Data Flow

Colorizing by spike rate or AP peak-to-peak voltage first applies a 300Hz
high-pass filter then optionally applies a global CAR filer. Turn the CAR
filter on or off using the `Gbl dmx` checkbox. Turning CAR off can make it
much easier to visually identify the boundary between tissue and outside.
Point the cursor at the boundary to read out the row. Enter that row in the
`Max row` box to exclude electrodes outside the brain when 'Gbl Dmx' is on.
Setting `-1` turns the Max-row feature off ('Gbl Dmx' uses all channels when
it is on).

`Average current window` applies a 300Hz high-pass and optional global
CAR filter, then counts spikes and tallies peak-to-peak voltages over the
data currently displayed in the File Viewer window. The Shank Viewer updates
as you scroll.

`Sample whole survey` is specifically for viewing survey result files. It
averages up to four half-second time chunks drawn from each of the banks
on the probe to build a whole-probe activity map. You have to click `Update
whole survey` to trigger the calculation. If you change {`Gbl dmx`, `Max row`,
`T(-uV)`, `Stay low`} you need to click `Update` again.

### Setting a Spike Threshold 'T(-uV)'

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

If your spike rates seem too low, try lowering 'Stay low'.

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


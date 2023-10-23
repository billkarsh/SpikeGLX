## SpikeViewer: Display spikes on up to 4 independent channels

### Detection

Spikes are detected on the given {probe, channel} when the voltage
crosses from above to below the given threshold (negative microvolts)
and stays below threshold for at least **3 consecutive samples**.

Spikes are each 64 samples long.

Up to 32 time-stamped spikes at a time are maintained in a FIFO buffer.

New spikes replace the oldest spikes. Each spike is retained no longer
than 30 seconds after it was detected.

### Waveforms

Individual spikes are displayed (yellow) and are used to calculate their
mean waveform (green) and average spike rate (blue bar on a log scale).

Spike waveforms are aligned to their minima.

The vertical full-scale voltage is set by the `yscl` spinner control in
microvolts. The baseline (y-axis) is adjusted automatically.

### Showing the window

* Open window and apply last settings with menu item: `Window/SpikeViewer...`

### Picking channels (Also opens window)

* Right-click on ShankViewer site; select `SpikeViewer N` from popup menu.

* Right-click on Graph window trace; select `SpikeViewer N` from popup menu.

* Use the `probe` and `channel` spinners for that viewer.


_fin_


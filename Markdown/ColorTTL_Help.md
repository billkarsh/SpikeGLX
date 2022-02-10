## ColorTTL: Visually Link Neural Activity and TTL Inputs

### Overview

You can monitor voltage levels in up to 4 auxiliary channels (either
analog or digital). Each channel has a unique color it applies to
the online graphs as a vertical stripe across all traces. The left edge
of the stripe shows when the monitored channel went high (crossing its
voltage threshold) and the right edge shows when the channel went below
threshold again. That is, it marks when a pulse occurred.

You can monitor channels in any stream.

### Limitations

* The coloring is communicated only among the graph panels in the same
Graphs window. That is, only among a pair of streams.

* The data that are scanned for TTL edges come from a graph panel. If you
set up ColorTTL to scan NI channels, for example, but you aren't displaying
the NI graph panel, you won't detect/see any edges.

### Color item group

We've preset the colors: green, magenta, cyan, orange.

- Each color has a checkbox to enable it.
- You can select which stream to watch.
- You must select either an analog voltage channel or a digital bit to watch.
- Digital bits are either {0,1} so no threshold is used.
- In streams with multiple digital words, bits are numbered: [0..15], [16..31], etc.
- Analog channels need a threshold to accommodate hardware noise.

### Stay high count

Set this to `1` if there is no noise in your monitored signals. If there
is a chance your signals are noisy or may bounce during transitions,
increase this value. The value is the number of samples in a row that
have to be high to be sure the transition is real.

### Minimum width (s)

If the actual pulse width is very short, then the painted stripe might
be so narrow you can't see it at the current graph time span. Set this
value to the minimum stripe width you can usefully see (in seconds).


_fin_


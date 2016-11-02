SpikeGLX
=========

<br>
#### *SpikeGLX is a new and improved SpikeGL*
<br>

### What

SpikeGLX is a recording system for extracellular neural probes. The emphasis
is on concurrent synchronous recording from high channel count probes together
with numerous auxiliary analog and digital channels:

* Concurrent, synchronized acquisition from IMEC and NI-DAQ devices.
* HHMI/Whisper System support.
* Flexible visualization, filtering and sorting tools.
* Programmable triggering.
* Remote control via MATLAB.
* Powerful offline viewing and editing.

### Who

SpikeGLX is developed by [Bill Karsh](https://www.janelia.org/people/bill-karsh)
of the [Tim Harris Lab](https://www.janelia.org/lab/harris-lab-apig) at
HHMI/Janelia Research Campus.

SpikeGLX is based upon [SpikeGL](https://github.com/cculianu/SpikeGL.git),
written by independent developer **Calin A. Culianu** under contract to the
[Anthony Leonardo Lab](https://www.janelia.org/lab/leonardo-lab) at Janelia.
There are significant changes to support IMEC technology and higher bandwidth.

### Compiled Software

Download official release software and support materials here:
[http://billkarsh.github.io/SpikeGLX.](http://billkarsh.github.io/SpikeGLX)

### System Requirements

* Windows: XP SP3, 7, 8.1, 10.
* NI-DAQmx 9 or later (recommend latest version).
* Minimum of four cores.
* Minimum of 2.5 GHz.
* Minimum of 4 GB RAM.
* Dedicated second hard drive for data streaming.

SpikeGLX is multithreaded. More processors enable better workload
balancing with fewer bottlenecks. The OS, background tasks and most other
apps make heavy use of the C:/ drive. This is the worst destination for
high bandwidth data streaming. A second hard drive dedicated to data
streaming is strongly recommended. More cores and a separate drive are
by far the most important system specs. More RAM, clock speed, graphics
horsepower and so on are welcome but less critical.

#### Imec

The high channel count of Imec probes places addition demands on the
system:

* You must have a dedicated network interface card (NIC) and cable
rated for Gigabit Ethernet. (That's a card, not a dongle).

* Data collection requires an SSD (solid state drive) with sustained
write speed of at least 500 MB/s. Fortunately these are readily available
and affordable.

### Licensing

Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:
[http://license.janelia.org/license](http://license.janelia.org/license).

<br>
_fin_

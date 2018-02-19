SpikeGLX
=========

#### *SpikeGLX is a new and improved SpikeGL*

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
[http://billkarsh.github.io/SpikeGLX](http://billkarsh.github.io/SpikeGLX).

### System Requirements

#### General

* Windows: XP SP3, 7, 8.1, 10.
* NI-DAQmx 9 or later (recommend latest version).
* Minimum of four cores.
* Minimum of 2.5 GHz.
* Minimum of 4 GB RAM for 32-bit OS.
* Minimum of 8 GB RAM for 64-bit OS.
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
rated for Gigabit Ethernet (category 6 or better).

> We find that Ethernet dongles typically have much lower real world
bandwidth than an actual card, so plugin adapters are discouraged.
Note too, that you will configure your Ethernet device with static
IP address (10.2.0.xx) and subnet mask (255.0.0.0). This device can
not be used for other network activity while configured for Imec data
transfer. SpikeGLX incorporates TCP/IP servers to interface with other
applications, like MATLAB, and can even stream live data during a run.
This continues to work fine, but now requires two NIC cards: one for
Imec and a separate one that can be assigned a different address.

* Data collection requires an SSD (solid state drive) with sustained
write speed of at least 500 MB/s (check manufacturer's specs). These
are readily available and affordable.

### Frequently Asked Questions

[SpikeGLX FAQ](Markdown/SpikeGLX_FAQ.md).

### Licensing

Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:
[http://license.janelia.org/license](http://license.janelia.org/license).


_fin_


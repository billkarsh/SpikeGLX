SpikeGLX
=========

<br>
#### *SpikeGLX is a new and improved SpikeGL*
<br>

[SpikeGL](https://github.com/cculianu/SpikeGL.git) is a mature
feature-rich **neural recording system**, originally written by independent
developer **Calin A. Culianu** under contract to the
[Anthony Leonardo Lab](https://www.janelia.org/lab/leonardo-lab)
at HHMI/Janelia Research Campus.

SpikeGLX is a complete rewrite by
[Bill Karsh](https://www.janelia.org/people/bill-karsh) of the
[Tim Harris Lab](https://www.janelia.org/lab/harris-lab-apig), also
at Janelia. Our lab goes by the brief name "**APIG**," an undignified way
of saying "_Applied Physics and Instrumentation Group_." We develop
a variety of imaging and electrophysiology tools supporting state of
the art neuroscience.

APIG has now initiated the **IMEC** project with several other institutions.
Its purpose is to create extracellular neural probes with far higher
channel counts than ever before (several hundred per shank). The system
must include reliable data acquisition software, and I selected
SpikeGL as the basis for the new system.

My development plan has these phases:

1. Fix up SpikeGL to make it the best possible version of itself.
2. Publish that to github.
3. Add to that new support for IMEC probes.
4. Add feature enhancements.

### What's the Same

SpikeGLX retains strong data compatibility and user experience with the original:

* Feature set frozen at SpikeGL version 20140814.
* Compatible binary file format.
* Same basic metadata format with minor ini-tag name changes.
* Enhanced TCP/IP API with only minor command name changes.
* Largely same windows, menus, displays, layout.

In short, I wanted to ensure that current users of SpikeGL could migrate to
SpikeGLX with negligible impact/resistance.

### What's New

I must emphasize that Calin provided an excellent starting point. He's
a creative and talented guy who taught me a great deal about Qt, OpenGL
and ephys concepts. I owe a sincere debt of gratitude for getting me going.
That being said, there were stability, performance and structural issues that
got in the way of my ability to understand and extend the original.

The overhaul is significant. Some highlights:

* More efficient threading supports:
    * Higher bandwidth, hence, channel counts
    * Fewer (if any) buffer overruns
    * Better tolerance of background activity
    * Lower latency (e.g. 12 ms analog out)
* Lower latency TCP/IP interface.
* Cleaner, expandable configuration dialog.
* Refactored and more modular code.
* Improved decoupling of features.
* Improved project file organization.
* More idiomatic and best-practice Qt usage.
* Simplified and streamlined algorithms.

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

### Compiled Software

Download official release software and support materials here:
[http://billkarsh.github.io/SpikeGLX.](http://billkarsh.github.io/SpikeGLX)

<br>
_fin_

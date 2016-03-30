SpikeGLX Neural Recording System
==========================

This is your official download source for compiled
[SpikeGLX](https://github.com/billkarsh/SpikeGLX.git)
software and support materials.

### Application Downloads

* [Release 20160304](App/Release_v20160304.zip)...[Readme](Readme/Readme_v20160304.txt) : Quick startup, quick changes, Imec v3.4
* [Release 20160302](App/Release_v20160302.zip)...[Readme](Readme/Readme_v20160302.txt) : Fix Bank 0 configuration, uses Imec v3.0
* [Release 20160101](App/Release_v20160101.zip)...[Readme](Readme/Readme_v20160101.txt)
* [Release 20151231](App/Release_v20151231.zip)

### Support Downloads

* [Channel mapping for IMEC Phase II probes](Support/PhaseII_Mapping.zip)

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

### Installation and Setup

To install SpikeGLX on a new system, just copy a virgin SpikeGLX folder
to your C-drive and double click SpikeGLX.exe to begin.

> SpikeGLX is currently a 32-bit application. If you have any difficulty
launching it in 64-bit Windows, try:
>
> 1. Right-click on the application icon
> 2. Choose Properties
> 3. Compatibility Tab
> 4. Check : Run this program in compatibility mode for:
> 5. Select: Windows XP (Service Pack 3)

The contents of a virgin (see below) SpikeGLX v20151231 folder:

```
SpikeGLX/
    platforms/
        qminimal.dll
        qwindows.dll
    icudt52.dll
    icuin52.dll
    icuuc52.dll
    libgcc_s_dw2-1.dll
    libNeuropix_basestation_api.dll
    libstdc++-6.dll
    libwinpthread-1.dll
    qt.conf
    Qt5Core.dll
    Qt5Gui.dll
    Qt5Network.dll
    Qt5OpenGL.dll
    Qt5Svg.dll
    Qt5Widgets.dll
    SpikeGLX.exe
    SpikeGLX_NISIM.exe
```

>Virgin: The SpikeGLX folder does not contain a `configs` subfolder.

### Licensing

Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:
[http://license.janelia.org/license](http://license.janelia.org/license).

<br/>
<br/>

_fin_



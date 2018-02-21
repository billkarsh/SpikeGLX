SpikeGLX Neural Recording System
==========================

This is your official download source for compiled
[SpikeGLX](https://github.com/billkarsh/SpikeGLX.git)
software and support materials.

### Application Downloads

* [Release 20171101-phase3A](App/Release_v20171101-phase3A.zip)...[Readme](Readme/Readme_v20171101-phase3A.txt) : Synchronization, Imec v4.3
* [Release 20170901-phase3A](App/Release_v20170901-phase3A.zip)...[Readme](Readme/Readme_v20170901-phase3A.txt) : Fix imec FIFO filling, Imec v4.3
* [Release 20170814](App/Release_v20170814.zip)...[Readme](Readme/Readme_v20170814.txt) : Fix nidq-only run starts, Imec v4.3
* [Release 20170808](App/Release_v20170808.zip)...[Readme](Readme/Readme_v20170808.txt) : TTL trigger fix, Imec v4.3
* [Release 20170724](App/Release_v20170724.zip)...[Readme](Readme/Readme_v20170724.txt) : MATLAB:SetMetaData, timed trigger fix, Imec v4.3
* [Release 20170501](App/Release_v20170501.zip)...[Readme](Readme/Readme_v20170501.txt) : Audio, more MATLAB tools, Imec v4.3
* [Release 20170315](App/Release_v20170315.zip)...[Readme](Readme/Readme_v20170315.txt) : Clearer spikes, MATLAB tools, Imec v4.3
* [Release 20161201](App/Release_v20161201.zip)...[Readme](Readme/Readme_v20161201.txt) : More GUI features, Imec v4.3
* [Release 20161101](App/Release_v20161101.zip)...[Readme](Readme/Readme_v20161101.txt) : Shank viewers, Imec v4.3
* [Release 20160806](App/Release_v20160806.zip)...[Readme](Readme/Readme_v20160806.txt) : Internal refs, spatial ave, Imec v4.3
* [Release 20160703](App/Release_v20160703.zip)...[Readme](Readme/Readme_v20160703.txt) : Even better file viewers, Imec v4.3
* [Release 20160701](App/Release_v20160701.zip)...[Readme](Readme/Readme_v20160701.txt) : Real offline file viewers, Imec v4.2
* [Release 20160601](App/Release_v20160601.zip)...[Readme](Readme/Readme_v20160601.txt) : MATLAB:GetFileStartCt, TTL fix, Imec v4.2
* [Release 20160515](App/Release_v20160515.zip)...[Readme](Readme/Readme_v20160515.txt) : BIST, digital graphs, better AO, Imec v4.2
* [Release 20160511](App/Release_v20160511.zip)...[Readme](Readme/Readme_v20160511.txt) : Split files, better timing, Imec v4.1
* [Release 20160502](App/Release_v20160502.zip)...[Readme](Readme/Readme_v20160502.txt) : ADC+gain cal, sync, force ID, Imec v4.1
* [Release 20160404](App/Release_v20160404.zip)...[Readme](Readme/Readme_v20160404.txt) : Gate/trigger improvements, Imec v3.5
* [Release 20160401](App/Release_v20160401.zip)...[Readme](Readme/Readme_v20160401.txt) : Disk calc, NI(USB) bug fixes, Imec v3.5
* [Release 20160305](App/Release_v20160305.zip)...[Readme](Readme/Readme_v20160305.txt) : Imec and NI(USB) bug fixes, Imec v3.4
* [Release 20160304](App/Release_v20160304.zip)...[Readme](Readme/Readme_v20160304.txt) : Quick startup, quick changes, Imec v3.4
* [Release 20160302](App/Release_v20160302.zip)...[Readme](Readme/Readme_v20160302.txt) : Fix Bank 0 configuration, uses Imec v3.0
* [Release 20151231](App/Release_v20151231.zip)

### Support Downloads

* [Channel mapping for IMEC Phase II probes (Pre  20160301)](Support/PhaseII_Mapping_Pre20160301.zip)
* [Channel mapping for IMEC Phase II probes (Post 20160301)](Support/PhaseII_Mapping_Post20160301.zip)

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
IP address (10.2.0.123) and subnet mask (255.0.0.0). This device can
not be used for other network activity while configured for Imec data
transfer. SpikeGLX incorporates TCP/IP servers to interface with other
applications, like MATLAB, and can even stream live data during a run.
This continues to work fine, but now requires two NIC cards: one for
Imec and a separate one that can be assigned a different address.

* Data collection requires an SSD (solid state drive) with sustained
write speed of at least 500 MB/s (check manufacturer's specs). These
are readily available and affordable.

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

The contents of a virgin (see below) SpikeGLX folder:

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

### Frequently Asked Questions

[SpikeGLX FAQ](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/SpikeGLX_FAQ.md).

### Licensing

Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:
[http://license.janelia.org/license](http://license.janelia.org/license).


_fin_


SpikeGLX Neural Recording System
==========================

This is your official download source for compiled
[SpikeGLX](https://github.com/billkarsh/SpikeGLX.git)
software and support materials.

#### Imec Project Phases

There are three branches in this repo separately supporting the three Imec
Neuropixels project development phases. The three phases have mutually
incompatible hardware and software, so obtain the appropriate materials
for your needs. Release software packages are labeled as phase3A, phase3B1,
or phase3B2. All early releases having no phase label are actually phase3A.
All releases support NI-DAQ based acquisition.

* **Phase3A**: Initially, 4 flavors of prototype probe (option 1, 2, 3, 4)
were created enabling consortium members to choose the most useful
architecture and feature set. Phase3A probes are connected one at a time to
a Xilinx Kintex 7 FPGA board, and accessed via Ethernet. The probes, cables,
HS and BSC parts are specific to phase3A.

* **Phase3B1**: At this phase, option 3 probes had won and were further
developed for commercial production. The same Xilinx board is retained for
one at a time probe operation over Ethernet (but reprogrammed for 3B1).
The cables, HS and BSC parts are all specific to 3B1. The 3B probes can be
run with either 3B1 or 3B2 setups.

* **Phase3B2 (master code branch)**: This phase replaces the Xilinx board
with PXIe based modules, each of which connects up to 4 probes. Several
modules can be operated together from one PXI chassis and one application.
The probes are the same as the phase3B1, but all other hardware parts are
specific to the PXIe implementation.

### Latest Application Downloads

> The first PXI version is not yet posted, expected by January 18, 2019.

* [Release 20180515-phase3B1](App/Release_v20180515-phase3B1.zip)...[Readme](Readme/Readme_v20180515-phase3B1.txt) : Fix FileViewer, Imec v5.1
* [Release 20180525-phase3A](App/Release_v20180525-phase3A.zip)...[Readme](Readme/Readme_v20180525-phase3A.txt) : Add MATLAB features, minor fixes, Imec v4.3

### Older Versions

* [Release 20180325-phase3B1](App/Release_v20180325-phase3B1.zip)...[Readme](Readme/Readme_v20180325-phase3B1.txt) : Fixed-up 3B1, Imec v5.1
* [Release 20180515-phase3A](App/Release_v20180515-phase3A.zip)...[Readme](Readme/Readme_v20180515-phase3A.txt) : Handle broken EEPROMs, fix FileViewer, Imec v4.3
* [Release 20180325-phase3A](App/Release_v20180325-phase3A.zip)...[Readme](Readme/Readme_v20180325-phase3A.txt) : MATLAB fixes, new filters, Imec v4.3
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

These channel mapping files allowed early Imec probes to be read out using
SpikeGLX's NI-DAQ/Whisper feature set.

* [Channel mapping for IMEC Phase II probes (Pre  20160301)](Support/PhaseII_Mapping_Pre20160301.zip)
* [Channel mapping for IMEC Phase II probes (Post 20160301)](Support/PhaseII_Mapping_Post20160301.zip)

### System Requirements

Requirements differ according to platform:

* [For PXI-based setups](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/SystemRequirements_PXI.md).
* [For all earlier setups](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/SystemRequirements_Xilinx.md).

### Installation and Setup

To install SpikeGLX on a new system, just unzip a SpikeGLX release folder
to your C-drive, read the included README, then double click
SpikeGLX/SpikeGLX.exe to begin.

> Alternatively, run SpikeGLX/SpikeGLX_NISIM.exe if you don't need/want
any NI-DAQ dependencies.

### Frequently Asked Questions

[SpikeGLX FAQ](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/SpikeGLX_FAQ.md).

### Licensing

Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:
[http://license.janelia.org/license](http://license.janelia.org/license).

QLed components are subject to GNU Library GPL v2.0 terms, which are described here:
[https://github.com/billkarsh/SpikeGLX/blob/master/QLed-LGPLv2-LICENSE.txt](https://github.com/billkarsh/SpikeGLX/blob/master/QLed-LGPLv2-LICENSE.txt).


_fin_


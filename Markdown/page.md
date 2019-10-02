SpikeGLX Neural Recording System
=================================

**Concurrent, synchronized acquisition from Imec and NI-DAQ devices.**

## Download Page

* Get compiled apps, support and help stuff on this page.
* The source code repo is [here](https://github.com/billkarsh/SpikeGLX.git).

**Topics:**

* [Imec Project Phases](#imec-project-phases)
* [System Requirements](#system-requirements)
* Downloads
    * [Latest Application Downloads](#latest-application-downloads)
    * [Older Versions](#older-versions)
    * [Support Downloads](#support-downloads)
        * [PXI Enclustra Drivers](#pxi-enclustra-drivers)
        * [Interesting Map Files](#interesting-map-files)
        * [Offline Analysis Tools](#offline-analysis-tools)
* [Help](#help)
* [How-to Videos](#how-to-videos)
* [Licensing](#licensing)

#### Imec Project Phases

There are four branches in this repo separately supporting the four Imec
Neuropixels project development phases. The four phases have mutually
incompatible hardware and software, so obtain the appropriate materials
for your needs. Release software packages are labeled as phase3A, phase3B1,
phase3B2, or phase20. All early releases having no phase label are actually
phase3A. All releases support NI-DAQ based acquisition.

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

* **Phase3B2**: This phase replaces the Xilinx board with PXIe based
modules, each of which connects up to 4 probes. Several modules can
be operated together from one PXI chassis and one application.
The probes are the same as the phase3B1, but all other hardware parts
are specific to the PXIe implementation.

* **Phase20**: Neuropixels 2.0 introduces specific probes and HS with a
more compact design. The HS each support two probes. The same PXIe modules
can be used with a firmware update. Initially, SpikeGLX for this phase is
specific to 2.0 hardware. In future one app will run 2.0 and 3B2 hardware.

### System Requirements

Requirements differ according to platform:

* [For PXI-based setups](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/SystemRequirements_PXI.md).
* [For all earlier setups](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/SystemRequirements_Xilinx.md).

### Latest Application Downloads

* [Release 20190919-phase20](App/Release_v20190919-phase20.zip)...[Readme](Readme/Readme_v20190919-phase20.txt) : Initial NP 2.0, Imec v2.8
* [Release 20190413-phase3B2](App/Release_v20190413-phase3B2.zip)...[Readme](Readme/Readme_v20190413-phase3B2.txt) : Flexible bank selection, Imec v1.20
* [Release 20190413-phase3B1](App/Release_v20190413-phase3B1.zip)...[Readme](Readme/Readme_v20190413-phase3B1.txt) : Flexible bank selection, Imec v5.1
* [Release 20190413-phase3A](App/Release_v20190413-phase3A.zip)...[Readme](Readme/Readme_v20190413-phase3A.txt) : Flexible bank selection, Imec v4.3

>Suggested organization: Create folder 'SpikeGLX' on your desktop or `C:\`
then download/unzip associated stuff into it:

```
SpikeGLX\
    Release_v20190919-phase20.zip
    Release_v20190413-phase3B2.zip
    Release_v20190919-phase20\
    Release_v20190413-phase3B2\
    Drivers\
        Enclustra_Windows_10\
    Tools\
    Etc\
```

### Older Versions

* [Release 20190327-phase3B2](App/Release_v20190327-phase3B2.zip)...[Readme](Readme/Readme_v20190327-phase3B2.txt) : Fix names with dots, Imec v1.15
* [Release 20190305-phase3B2](App/Release_v20190305-phase3B2.zip)...[Readme](Readme/Readme_v20190305-phase3B2.txt) : 32 NI channels, Imec v1.15
* [Release 20190214-phase3B2](App/Release_v20190214-phase3B2.zip)...[Readme](Readme/Readme_v20190214-phase3B2.txt) : First PXI, Imec v1.15

* [Release 20190327-phase3B1](App/Release_v20190327-phase3B1.zip)...[Readme](Readme/Readme_v20190327-phase3B1.txt) : Fix names with dots, Imec v5.1
* [Release 20190305-phase3B1](App/Release_v20190305-phase3B1.zip)...[Readme](Readme/Readme_v20190305-phase3B1.txt) : 32 NI channels, Imec v5.1
* [Release 20190214-phase3B1](App/Release_v20190214-phase3B1.zip)...[Readme](Readme/Readme_v20190214-phase3B1.txt) : Run folders, Imec v5.1
* [Release 20180829-phase3B1](App/Release_v20180829-phase3B1.zip)...[Readme](Readme/Readme_v20180829-phase3B1.txt) : Fix TTL trigger, Imec v5.1
* [Release 20180515-phase3B1](App/Release_v20180515-phase3B1.zip)...[Readme](Readme/Readme_v20180515-phase3B1.txt) : Fix FileViewer, Imec v5.1
* [Release 20180325-phase3B1](App/Release_v20180325-phase3B1.zip)...[Readme](Readme/Readme_v20180325-phase3B1.txt) : Fixed-up 3B1, Imec v5.1

* [Release 20190327-phase3A](App/Release_v20190327-phase3A.zip)...[Readme](Readme/Readme_v20190327-phase3A.txt) : Fix names with dots, Imec v4.3
* [Release 20190305-phase3A](App/Release_v20190305-phase3A.zip)...[Readme](Readme/Readme_v20190305-phase3A.txt) : 32 NI channels, Imec v4.3
* [Release 20190214-phase3A](App/Release_v20190214-phase3A.zip)...[Readme](Readme/Readme_v20190214-phase3A.txt) : Run folders, Imec v4.3
* [Release 20180829-phase3A](App/Release_v20180829-phase3A.zip)...[Readme](Readme/Readme_v20180829-phase3A.txt) : Fix TTL trigger, Imec v4.3
* [Release 20180525-phase3A](App/Release_v20180525-phase3A.zip)...[Readme](Readme/Readme_v20180525-phase3A.txt) : Add MATLAB features, minor fixes, Imec v4.3
* [Release 20180515-phase3A](App/Release_v20180515-phase3A.zip)...[Readme](Readme/Readme_v20180515-phase3A.txt) : Handle broken EEPROMs, fix FileViewer, Imec v4.3
* [Release 20180325-phase3A](App/Release_v20180325-phase3A.zip)...[Readme](Readme/Readme_v20180325-phase3A.txt) : MATLAB fixes, new filters, Imec v4.3
* [Release 20171101-phase3A](App/Release_v20171101-phase3A.zip)...[Readme](Readme/Readme_v20171101-phase3A.txt) : Synchronization, Imec v4.3

### Support Downloads

#### PXI Enclustra Drivers

These required driver files are not included in your SpikeGLX release.

1. Click the link below for your Windows OS version.
2. Unzip the folder into your local folder of SpikeGLX-related stuff.
3. Follow the installation ReadMe in the download.

>Terminology: 'Enclustra' is a company that markets other vendor's FPGAs
along with development and support tools. There are several FPGAs used in
the Neuropixels hardware, including a Xilinx Zynq model, often referred to
as 'the enclustra'.

* [Windows 7 & 8](Support/Enclustra_Win7&8.zip)
* [Windows 10](Support/Enclustra_Win10.zip)

#### Interesting Map Files

* [Checkerboard Bank Selection 3B](Support/CheckPattern_3B.zip) (interleaves banks zero and one)
* [Long Column Bank Selection 3B](Support/LongColPattern_3B.zip) (one column through banks zero and one)

#### Offline Analysis Tools

Descriptions of metafile items for each phase:

* [Metadata_20](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Support/Metadata_20.md)
* [Metadata_3B2](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Support/Metadata_3B2.md)
* [Metadata_3B1](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Support/Metadata_3B1.md)
* [Metadata_3A](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Support/Metadata_3A.md)

MATLAB and Python tools for parsing meta and binary datafiles (supports 3A, 3B1, 3B2, 20).

* [SpikeGLX_Datafile_Tools](Support/SpikeGLX_Datafile_Tools.zip)

CatGT is a command-line tool that does the following offline operations (all imec phases):

>+ Join trials with given run_name and g-index in t-index range [ta,tb]...
>+ ...Or run on any individual file.
>+ Optionally apply bandpass and global demux CAR filters.
>+ Optionally edit out saturation artifacts.
>+ Optionally extract tables of TTL event times.
>+ [CatGT: Global Demuxed CAR](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Help/dmx_vs_gbl/dmx_vs_gbl.md)

TPrime is a command line tool that maps event times (all imec phases):

>+ Map time from one SpikeGLX data stream to any other.
>+ Translate all events to a single common timeline.
>+ Uses sync edges to achieve 1 to 2 sample accuracy.
>+ Maps TTL events extracted using CatGT.
>+ Maps spike times from any sorter.
>+ [Sync: Aligning with Edges.](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Help/SyncEdges/Sync_edges.md)

* [CatGT 1.1](Support/CatGTApp.zip)
* [TPrime 1.0](Support/TPrimeApp.zip)

### Help

* [Neuropixels Support Page](https://www.neuropixels.org/support)
* [Sharpening Apparatus (MS.Word.docx)](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Help/NPix_sharpening.docx)
* [Installing NI Drivers](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Help/NI_driver_installation.md)
* [SpikeGLX UserManual](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/UserManual.md)
* [SpikeGLX FAQ](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/SpikeGLX_FAQ.md)
* [Sync: Aligning with Edges](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Help/SyncEdges/Sync_edges.md)
* [CatGT: Global Demuxed CAR](https://github.com/billkarsh/SpikeGLX/blob/gh-pages/Help/dmx_vs_gbl/dmx_vs_gbl.md)

### How-to Videos

* [Handle & Solder](https://vimeo.com/315542037)
* [Sharpen Probe](https://vimeo.com/359133527)
* [Install Software](https://vimeo.com/316017791)
* [First Run](https://vimeo.com/322145679)
* [Sync](https://vimeo.com/322974285)

### Licensing

Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:
[http://license.janelia.org/license](http://license.janelia.org/license).

QLed components are subject to GNU Library GPL v2.0 terms, which are described here:
[https://github.com/billkarsh/SpikeGLX/blob/master/QLed-LGPLv2-LICENSE.txt](https://github.com/billkarsh/SpikeGLX/blob/master/QLed-LGPLv2-LICENSE.txt).


_fin_


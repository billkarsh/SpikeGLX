# History / Older versions

## Imec Project Phases

>This site supports prototype and currently shipping commercial products.</BR>
>The latest commercial PXI-based components are:
>
>* Hardware: `Neuropixels 1.0 probes` (also called phase3B2).
>* Software: `SpikeGLX 3.0` (also called phase30).

There are five branches in this repo separately supporting the five Imec
Neuropixels project development phases. The five phases have mutually
incompatible hardware and software, so obtain the appropriate materials
for your needs. Release software packages are labeled as phase3A, phase3B1,
phase3B2, phase20, or phase30. All early releases having no phase label are
actually phase3A. All releases support NI-DAQ based acquisition.

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

* **Phase3B2**: A.k.a. Neuropixels 1.0. This phase replaces the Xilinx
board with PXIe based modules, each of which connects up to 4 probes.
Several modules can be operated together from one PXI chassis and one
application. The probes are the same as phase3B1, but all other hardware
parts are specific to the PXIe implementation.

* **Phase20**: A.k.a. Neuropixels 2.0, introduces specific probes and HS
with a more compact design. The HS each support two probes. The same PXIe
modules can be used with a firmware update. SpikeGLX for this phase is
specific to 2.0 hardware.

* **Phase30**: This is unified software to support Neuropixels 1.0, 2.0 and
all other probe hardware going forward. Again, the same base station modules
can be used after updating their firmware, which can be done via SpikeGLX.

------

## Older Versions

**30**:

* [Release 20240129-phase30](../App/Release_v20240129-phase30.zip)...[Readme](../Readme/Readme_v20240129-phase30.txt) : New IMRO editor, imec v3.62.1
* [Release 20231207-phase30](../App/Release_v20231207-phase30.zip)...[Readme](../Readme/Readme_v20231207-phase30.txt) : NP1016, imec v3.62.1
* [Release 20230905-phase30](../App/Release_v20230905-phase30.zip)...[Readme](../Readme/Readme_v20230905-phase30.txt) : Spike Viewer, bug fixes, imec v3.62
* [Release 20230815-phase30](../App/Release_v20230815-phase30.zip)...[Readme](../Readme/Readme_v20230815-phase30.txt) : NP2014, low latency mode, imec v3.62
* [Release 20230425-phase30](../App/Release_v20230425-phase30.zip)...[Readme](../Readme/Readme_v20230425-phase30.txt) : Anatomy data, better SNR, imec v3.60
* [Release 20230411-phase30](../App/Release_v20230411-phase30.zip)...[Readme](../Readme/Readme_v20230411-phase30.txt) : 2.0 probes, Fix PH2B headstage, imec v3.60
* [Release 20230323-phase30](../App/Release_v20230323-phase30.zip)...[Readme](../Readme/Readme_v20230323-phase30.txt) : Realistic geometry, ~snsGeomMap, Imec v3.57
* [Release 20230120-phase30](../App/Release_v20230120-phase30.zip)...[Readme](../Readme/Readme_v20230120-phase30.txt) : Fix TTL trigger, add UHD3 probes, Imec v3.57
* [Release 20221212-phase30](../App/Release_v20221212-phase30.zip)...[Readme](../Readme/Readme_v20221212-phase30.txt) : Fix sync SMA input, Imec v3.52
* [Release 20221012-phase30](../App/Release_v20221012-phase30.zip)...[Readme](../Readme/Readme_v20221012-phase30.txt) : Fix HS detection issue in 20220101, Imec v3.52
* [Release 20201103-phase30](../App/Release_v20201103-phase30.zip)...[Readme](../Readme/Readme_v20201103-phase30.txt) : Fix firmware update dialog, Imec v3.31

**20**:

* [Release 20200309-phase20](../App/Release_v20200309-phase20.zip)...[Readme](../Readme/Readme_v20200309-phase20.txt) : Multidrive output, Imec v2.11

**3B2 (NP 1.0)**:

* [Release 20200520-phase3B2](../App/Release_v20200520-phase3B2.zip)...[Readme](../Readme/Readme_v20200520-phase3B2.txt) : Supports UHD-1, NHP, Imec v1.20
* [Release 20200309-phase3B2](../App/Release_v20200309-phase3B2.zip)...[Readme](../Readme/Readme_v20200309-phase3B2.txt) : Multidrive output, Imec v1.20
* [Release 20190724-phase3B2](../App/Release_v20190724-phase3B2.zip)...[Readme](../Readme/Readme_v20190724-phase3B2.txt) : Minor bug fixes, Imec v1.20
* [Release 20190413-phase3B2](../App/Release_v20190413-phase3B2.zip)...[Readme](../Readme/Readme_v20190413-phase3B2.txt) : Flexible bank selection, Imec v1.20
* [Release 20190327-phase3B2](../App/Release_v20190327-phase3B2.zip)...[Readme](../Readme/Readme_v20190327-phase3B2.txt) : Fix names with dots, Imec v1.15

**3B1**:

* [Release 20190413-phase3B1](../App/Release_v20190413-phase3B1.zip)...[Readme](../Readme/Readme_v20190413-phase3B1.txt) : Flexible bank selection, Imec v5.1
* [Release 20190327-phase3B1](../App/Release_v20190327-phase3B1.zip)...[Readme](../Readme/Readme_v20190327-phase3B1.txt) : Fix names with dots, Imec v5.1
* [Release 20190305-phase3B1](../App/Release_v20190305-phase3B1.zip)...[Readme](../Readme/Readme_v20190305-phase3B1.txt) : 32 NI channels, Imec v5.1
* [Release 20190214-phase3B1](../App/Release_v20190214-phase3B1.zip)...[Readme](../Readme/Readme_v20190214-phase3B1.txt) : Run folders, Imec v5.1
* [Release 20180829-phase3B1](../App/Release_v20180829-phase3B1.zip)...[Readme](../Readme/Readme_v20180829-phase3B1.txt) : Fix TTL trigger, Imec v5.1
* [Release 20180515-phase3B1](../App/Release_v20180515-phase3B1.zip)...[Readme](../Readme/Readme_v20180515-phase3B1.txt) : Fix FileViewer, Imec v5.1
* [Release 20180325-phase3B1](../App/Release_v20180325-phase3B1.zip)...[Readme](../Readme/Readme_v20180325-phase3B1.txt) : Fixed-up 3B1, Imec v5.1

**3A**:

* [Release 20190413-phase3A](../App/Release_v20190413-phase3A.zip)...[Readme](../Readme/Readme_v20190413-phase3A.txt) : Flexible bank selection, Imec v4.3
* [Release 20190327-phase3A](../App/Release_v20190327-phase3A.zip)...[Readme](../Readme/Readme_v20190327-phase3A.txt) : Fix names with dots, Imec v4.3
* [Release 20190305-phase3A](../App/Release_v20190305-phase3A.zip)...[Readme](../Readme/Readme_v20190305-phase3A.txt) : 32 NI channels, Imec v4.3
* [Release 20190214-phase3A](../App/Release_v20190214-phase3A.zip)...[Readme](../Readme/Readme_v20190214-phase3A.txt) : Run folders, Imec v4.3
* [Release 20180829-phase3A](../App/Release_v20180829-phase3A.zip)...[Readme](../Readme/Readme_v20180829-phase3A.txt) : Fix TTL trigger, Imec v4.3
* [Release 20180525-phase3A](../App/Release_v20180525-phase3A.zip)...[Readme](../Readme/Readme_v20180525-phase3A.txt) : Add MATLAB features, minor fixes, Imec v4.3
* [Release 20180515-phase3A](../App/Release_v20180515-phase3A.zip)...[Readme](../Readme/Readme_v20180515-phase3A.txt) : Handle broken EEPROMs, fix FileViewer, Imec v4.3


_fin_


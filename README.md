SpikeGLX
=========

### What

SpikeGLX is a recording system for extracellular neural probes. The emphasis
is on concurrent synchronous recording from high channel count probes together
with numerous auxiliary analog and digital channels:

* Concurrent, synchronized acquisition from Imec probes, OneBoxes and NI-DAQ devices.
* Supports all Neuropixels probe generations and types.
* Supports HHMI/Whisper system.
* Integration with live anatomical tracking:
[Pinpoint](https://github.com/VirtualBrainLab/Pinpoint);
[Trajectory Explorer](https://github.com/petersaj/neuropixels_trajectory_explorer).
* Flexible visualization, filtering and sorting tools.
* Programmable triggering.
* Remote control and extensibility via MATLAB, C++, C, C#, Python.
* Powerful offline viewing and editing.

#### Imec Project Phases

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

### Who

SpikeGLX is developed by [Bill Karsh](https://www.janelia.org/people/bill-karsh)
of the [Tim Harris Lab](https://www.janelia.org/lab/harris-lab-apig) at
HHMI/Janelia Research Campus.

>*Based on the SpikeGL extracellular data acquisition system originally
developed by Calin A. Culianu.*

### Compiled Software

Official release software and support materials are here:
[**SpikeGLX Download Page**](http://billkarsh.github.io/SpikeGLX).

### System Requirements

Requirements differ according to platform:

* [For PXI-based setups](Markdown/SystemRequirements_PXI.md).
* [For all earlier setups](Markdown/SystemRequirements_Xilinx.md).

### Help

* Find help videos, docs and tools on the [**SpikeGLX Download Page**](http://billkarsh.github.io/SpikeGLX).
* Get help from expert users on the [**Neuropixels Slack Channel**](https://join.slack.com/t/neuropixelsgroup/shared_invite/zt-2c8u0k21u-CC4wSkb8~U_Fkrf3HHf_kw).

### Frequently Asked Questions

[SpikeGLX FAQ](Markdown/SpikeGLX_FAQ.md).

### Licensing

Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:
[http://license.janelia.org/license](http://license.janelia.org/license).

QLed components are subject to GNU Library GPL v2.0 terms, which are described here:
[QLed-LGPLv2-LICENSE.txt](QLed-LGPLv2-LICENSE.txt).


_fin_


## System Requirements for Legacy Xilinx

### Overview

These specifications are appropriate for the following cases:

* Xilinx based imec acquisition (Phase3A, Phase3B1).
* NI-only acquisition using traditional non-imec probes.

### Basic Requirements

* Phase3A Windows: XP SP3, 7, 8.1, 10
* Phase3B1 Windows: 7, 8.1, 10
* NI-DAQmx 9 or later
* Minimum of four cores
* Minimum of 2.5 GHz
* Minimum of 4 GB RAM for 32-bit OS
* Minimum of 8 GB RAM for 64-bit OS
* Dedicated second hard drive for data streaming

General Notes

* SpikeGLX is multithreaded. More processors enable better workload
balancing with fewer bottlenecks.

* The OS, background tasks and most other apps make heavy use of the
C:/ drive. This is the worst destination for high bandwidth data streaming.
A second hard drive dedicated to data streaming is strongly recommended.

### Imec Notes

The high channel count of Imec probes places addition demands on the
system:

* Data collection requires an SSD (solid state drive) with sustained
write speed of at least 500 MB/s (check manufacturer's specs). These
are readily available and affordable.

* For phase3A and phase3B1 Xilinx/Ethernet based implementations you must
have a dedicated network interface card (NIC) and cable rated for Gigabit
Ethernet (category 6 or better).

> We find that Ethernet dongles typically have much lower real world
bandwidth than an actual card, so plugin adapters are discouraged.
Note too, that you will configure your Ethernet device with static
IP address [phase3A=(10.2.0.123), phase3B1=(10.1.1.1)] and subnet mask
(255.0.0.0). This device can not be used for other network activity
while configured for Imec data transfer. SpikeGLX incorporates TCP/IP
servers to interface with other applications, like MATLAB, and can even
stream live data during a run. This continues to work fine, but now
requires two NIC cards: one for Imec and a separate one that can be
assigned a different address.


_fin_


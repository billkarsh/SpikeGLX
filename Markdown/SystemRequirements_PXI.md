## System Requirements for Neuropixels

**>> Updated: April 03, 2023 <<**

What's new:

* [Spike sorting hardware section.](#spike-sorting-requirements)
* [Thunderbolt 3 and 4 pass our tests, with caveats.](#thunderbolt)
* [250 MB/s controller handles 8 probes.](#acquisition-pxi-chassis)
* [System testing up to 20 probes.](#requirements)
* [Configure RAM for 2X speed boost!](#ram)
* Simplified purchasing guidelines.

**Table of Contents:**

* [Overview](#overview)
    * [Two steps, two computers](#two-steps-two-computers)
    * [Acquisition parts overview](#acquisition-parts-overview)
* [Acquisition: PXI Chassis](#acquisition-pxi-chassis)
    * [PXI Controller](#pxi-controller)
* [Acquisition: Computer](#acquisition-computer)
    * [Requirements](#requirements)
    * [RAM](#ram)
    * [Thunderbolt](#thunderbolt)
    * [Test performance](#test-performance)
* [Acquisition: NI multifunction IO](#acquisition-ni-multifunction-io)
    * [Breakout box and cable](#breakout-box-and-cable)
* [Acquisition: Simplified shopping list](#acquisition-simplified-shopping-list)
* [Spike sorting requirements](#spike-sorting-requirements)
    * [CPU](#cpu)
    * [Graphics card (GPU)](#graphics-card-gpu)

## Overview

### Two steps, two computers

The ideal setup will have two separate computers for the two main steps of
an experiment {acquisition, analysis}:

* **Online Data Acquisition** computer: attached to your experiment rig to
run probes and record data.

* **Offline Data Analysis/Spike Sorting** computer: located in a comfortable
setting to process and review the data. Importantly this computer must be able
to run the spike sorting program(s) of your choice.

It's fine to save money and use just one computer for both of these steps,
simply make sure the computer meets the combined requirements for both.

### Acquisition parts overview

**What parts do I need for imec/Neuropixels data acquisition experiments?**

From Neuropixels.org, you need:

* Probe(s)
* Head stage(s)
* 5m cable(s)
* PXI base-station module(s)

You also need a PXI chassis setup:

* Chassis
* Controller
* Computer
* Controller-computer link

You also probably want to collect non-neural data:

* NI multifunction IO module

## Acquisition: PXI Chassis

>BTW: Plural of chassis is also chassis.

We have tested a variety of **NI** and **ADLink** components at Janelia,
running up to 20 probes + 8 NI channels to discover any limitations. Other
users have reported success with **Keysight** models.

Bandwidth is the most important PXI specification:

* 2 GB/s handles **at least** 20 probes + NI.
* 250 MB/s handles **a maximum** of 8 probes + NI.

### PXI Controller

We've tested these remote-control modules (Chassis <-> PC) links:

* NI PXIe-8398 (16 GB/s)
* NI PXIe-8381 (4 GB/s)
* NI PXIe-8301 (2.3 GB/s Thunderbolt 3)
* ADLink 8638  (4 GB/s)

These mix and match in in our chassis without compatibility issues. At this
time we can offer these additional remarks:

* The 8398 may well be overkill. It did everything asked of it, but it is
very costly, uses up a PCIe Gen 3 x16 slot, and its thick cable is very
unwieldy. In fact, the cable is heavy enough to worry that it might not
hold securely in the device connectors which are somewhat flimsy.

* The 8381 and 8638 also performed perfectly in all tests out to 20 probes.
This is what we recommend at present. They are more affordable, small, and
need only PCIe Gen 2 x8 slots.

* The 1073 chassis has a built-in 250 MB/s remote controller (link) which
we've tested and works fine for at most 8 probes. You can't (and don't need
to) buy any other controller. This requires a PCIe Gen 1 1x slot.

* The 1083 chassis has a built-in Thunderbolt 3 remote controller (link)
which we've tested and works fine at 20 probes. You can't (and don't
need to) buy any other controller.

## Acquisition: Computer

### Requirements

**General:**

* 64-bit Windows {7, 10, 11}
* Minimum 2.5 GHz CPU
* Minimum 32 GB RAM
* Graphics Card: [PassMark G3D score](https://www.videocardbenchmark.net/high_end_gpus.html) > 10000.
* Dedicated data drive (SSD or NVMe, 500+ MB/s)
* PCIe 8X slot for PXIe controller (or Thunderbolt port).

Notes:

* Imec believes that AMD CPUs are not compatible with the required enclustra
drivers. We have not tested this ourselves, but we are aware that one or two
users have had success with AMD systems. We need more data about this. If you
have an AMD system, please try it and report whether it works for you on the
slack channel. Until thorough testing shows that AMD systems work reliably
in this application, we will continue to recommend using only intel systems.
* CPU frequency is not as important as the number of cores and the RAM
configuration (below).
* High-end GPU cards require workstations with 400W power supplies.
* The data drive should be distinct from the system drive.

**To run N probes:**

| CPU Cores | RAM Channels | Max Probes |
| --------- | ------------ | ---------- |
| 4         | 1            | 4          |
| 6         | 1            | 8          |
| 6         | 2            | 16         |
| 8         | 1            | 8          |
| 8         | 2            | 20         |

### RAM

SpikeGLX needs only 7 GB of total RAM to run 20 probes plus 8 NI channels.
Rather than memory size, what matters most are more CPU cores and *faster
memory access*. It makes a huge difference how the RAM DIMMS (memory sticks)
are populated into the slots on the motherboard, that is, how many RAM
channels are operational (see table above).

Computers can be designed with {single, dual, quad, octa}-channel memory.
For example, a dual-channel setup has twice the bandwidth (speed) of a
single-channel setup, and so on. However, to enable dual-channel mode,
you need at least two properly matched memory sticks placed into the
correct slots. If you can buy the same dual-channel laptop configured
either with one 32 GB stick or with two 16 GB sticks, they will both have
32 GB of RAM, but the two-stick setup is likely twice as fast at computation
and multithreading applications. Not all vendors make this detail available.
Try looking for an option to do a custom build.

If you are putting RAM in yourself, the user manual for the motherboard
will explain which slots should be populated, and you should buy your add-in
RAM as a kit designed to support N-channel setups, so the memory modules
are properly matched to each other and to your hardware.

> You can see how the RAM slots are populated in the Memory panel of
the Windows Task Manager/Performance tab.

Check out ["Guide to RAM Memory Channels as Fast As Possible."](https://www.youtube.com/watch?v=-D8fhsXqq4o)

### Thunderbolt

HHMI and the open ephys group independently tested laptops and workstations
with Thunderbolt 3.0 and 4.0. The successful setups all came preconfigured
for Thunderbolt from the factory. To add Thunderbolt after the fact, you'll
need to get a card specifically matched to your motherboard. Merely matching
the manufacturer does not guarantee success.

You should have reliable connectivity if you power up the chassis first,
and then start the PC, which is the usual start-up sequence for any other
PXIe controller. Hot-plugging can work if you are lucky but it is not
reliable. If you unplug and replug the Thunderbolt cable without doing
the orderly power cycle, the enclustra drivers may fail to load or fail
to start; you won't be able to connect to the imec base stations. If this
happens, fix it by going into the Windows Device Manager. Select each
enclustra device in the list, and choose `Uninstall device` under the
`Actions` menu. Finish by choosing `Scan for hardware changes` under the
`Actions` menu. If that doesn't work, try a complete power cycle, again,
starting the chassis first, then the PC.

### Test performance

In SpikeGLX select menu item `Window\Run Metrics` to display a window
of performance measurements that provide some insight on whether the system
is running comfortably or struggling to keep up.

You can also use the `Windows Task Manager` to monitor performance. In
particular, the average CPU utilization percentage should remain below
70%. If high activity levels persist SpikeGLX will stop a run.

## Acquisition: NI multifunction IO

Imec BS cards have no non-neural input channels, except for a single SMA
connector that SpikeGLX uses to synchronize the card with other devices.
However, SpikeGLX can record concurrently from the imec cards and from
an additional multifunction or digital IO device to cover physiological
data and trial marking signals. Neural and non-neural data are tightly
synchronized.

>SpikeGLX can actually operate two cards **provided they have identical
model numbers**. We are treating such a pair as a single device with
double the channel capacity.

SpikeGLX has these requirements for the non-neural device:

1. It must be an NI device that we can talk to via DAQmx (a general
purpose device programming language for NI hardware).

2. It must be an M-series (62XX), S-series (61XX), X-series (63XX),
or digital IO (653X) device.

>Note: As of version 20190305 SpikeGLX can read up to 32 digital lines
per device (previously limited to 8). Also, be aware that only a device's
'waveform' digital lines can be programmed for high sample rate input.
You'll have to look at the device spec sheet to see the count of
waveform lines. Digital device support is added as of version 20190413.

We have direct experience with these:

* PCI-based 6221 (M)
* PCI-based 6133 (S)    (16 MS FIFO tested)
* PXI-based 6133 (S)    (16 MS FIFO tested)
* PXI-based 6341 (X)
* PXI-based 6363 (X)
* USB-based 6366 (X)
* PXI-based 6535 (digital)

Some models (S and some X) have a feature called 'simultaneous sampling'
which means each input channel gets its own amplifier and ADC. This allows
the device to sample all its channels in parallel at the advertised maximum
sample rate, for example, 2.5 mega-samples/s/channel for the 6133. Moreover,
there is no crosstalk between the channels. That's what makes these models
very capable and very expensive. This is a must when using a Whisper
multiplexer which samples all AI channels at 800 kHz.

When doing multichannel acquisition, non-simultaneous-sampling devices
such as the 6221 use a multiplexing scheme to connect inputs to the
single amplifier/ADC unit in quick succession. The fastest you can drive
such a device depends upon how many channels you want to sample. It's
`R0/nChans`: R0 is the advertised maximum sample rate (250 KS/s for
the 6221). Be aware that switching from channel to channel at this rate
does not allow the amplifier to fully settle before the next input is
connected to it, hence, there will be some crosstalk (charge carryover).
To avoid that issue, run at a lower maximum sample rate given by:
`1/(1/R0 + 1E-5)`. For the 6221 example, you should sample no faster
than `71428/nChans`.

USB-based devices such as the 6366 can't use DMA data transfers, so have
lower effective bandwidth and higher latency than PCI or PXI devices. Go
ahead and use it if you already have one. However, don't use these for
digital input channels: The combination of low transfer rates and a very
small digital FIFO buffer make digital buffer overruns fairly common.

The X-series strike a balance between high sample rate (limited by settle
time) and high channel count. The 6363 has 32 AI and 32 waveform DI channels.
The 6341 has 32 single ended AI and 8 waveform DI channels for half the
price. Remember that AI channels can equally well read analog and TTL inputs.

### Breakout box and cable

Your NI module will talk to the world via a high-density multi-pin connector,
so you'll also want a breakout box (connector block) and cable that works
with your module. Browse here for
[NI multifunction IO](https://www.ni.com/en-us/shop/select/pxi-multifunction-io-module)
devices. Click on a table entry and a `View Accessories` button will appear.
There are easier to use options like the BNC-2110 that provide BNCs for the
most often accessed channels, and the SCB-68A that offers only screw terminals
but is more versatile because you can access all channels.

## Acquisition: Simplified Shopping List

We resisted recommending specific parts as long as possible, but people
keep asking what to buy, so here it is, the basic NI parts list to run
16 headstages plus NI.

| Item                                   | Part Number   | $US     |
| -------------------------------------- | ------------- | ------- |
| **Chassis:**                           |               |         |
| PXIe-1083, 5-slot integrated chassis   | [787026-01](https://www.ni.com/en-us/search.html?pg=1&ps=10&q=787026-01)     | 2365.00 |
| Thunderbolt 3 type-C cable 2m          | 785607-02     | 129.00  |
| US power cord                          | 763000-01     | 19.00   |
| **Multifunction IO:**                  |               |         |
| PXIe-6341 module                       | [781052-01](https://www.ni.com/en-us/support/model.pxie-6341.html)     | 1829.00 |
| BNC-2110 breakout box                  | [777643-01](https://www.ni.com/en-us/shop/accessories/products/terminal-block.html?skuId=33800)     | 545.00 |
| SHC68-68-EPM shielded cable 2m         | [192061-02](https://www.ni.com/en-us/support/model.shc68-68-epm.html)     | 204.00 |

* Available (but we haven't tested): [ADLink 4-slot integrated Thunderbolt 3 chassis](https://www.adlinktech.com/Products/PXI_PXIe_platform/PXIChassis/PXES-2314T?lang=en).
* You can buy the Thunderbolt 3 cable and power cord anywhere; they're not special.
* Prices do not include service contracts.
* All required software is free.

## Spike sorting requirements

You'll want to make sure your computer meets the requirements for all of the
offline analysis software you plan to use. Here we discuss our recommendations
for running Kilosort, the most commonly used spike sorting application.

### CPU

The general computer requirements for spike sorting are just a little
different than those for acquisition. In particular, you want at least
64 GB RAM which is enough to process recordings that are < 2 hours in
length. However, for longer recordings (or if concatenating shorter
recordings into a long one) then go for 128 GB of RAM.

The local data storage drive (in your workstation) should use fast solid state
technology. You will want to process runs on your computer and then move them
to long-term storage. Cheaper spinning disk technology is adequate for that.
But you will continually need more and more long term space. We find, for example,
[LaCie 'Rugged' 2 or 4 TB portable drives](https://www.lacie.com/products/rugged/)
work well for this.

Specs for a spike sorting machine:

* 64-bit Windows {10, 11}
* Minimum 4 cores, 2.5 GHz CPU
* Minimum 64 GB RAM
* Data drive 1 TB (SSD or NVMe)
* Graphics card: Nvidia CUDA-compliant, compute cability 6+, 8 GB RAM

### Graphics card (GPU)

The most important spec for your GPU card is that it be compatible with
MATLAB (Kilosort) and with the PyTorch machine learning package used by
both Kilosort 4, and the latest YASS replacement from Julien Boussard in
the Paninski lab. That means you want a card that uses
[Nvidia CUDA toolkit,](https://en.wikipedia.org/wiki/CUDA)
version 11.6 or 11.7. To be on the safe side, we recommend a graphics card
that is labeled as a 'GeForce card.' Such cards incorporate a chipset made
by Nvidia and definitely run CUDA. So, we recommend:

* A GeForce card; made by Nvidia, MSI or Gigabyte. We have tested
all of these.

* The card should have a `Compute Capability` rating of 6.0 or greater.
These ratings can be obtained on [this Nvidia page.](https://developer.nvidia.com/cuda-gpus)

* The GPU card should have at least 8 GB of onboard RAM if your unit counts
are in the hundreds. For unit counts in the thousands get 12 GB of GPU RAM.
Note that you can adjust Kilosort's RAM usage via its `ops.NT` (batch size)
parameter if you are running out of GPU RAM.


_fin_


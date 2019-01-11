## System Requirements for PXI

### Overview

Computer requirements depend upon the number of probes and other signals
to be acquired in an experiment. Heavy use of the visualization tools
during acquisition are an additional burden. Since a single computer
specification won't cover all uses of SpikeGLX, we give some rough
guidelines and examples of systems we have tested.

This document also covers options for acquisition of non-neural data
channels and what we know about different chassis options.

#### New Computer Purchase Guidelines

The following are what we think you should purchase if buying a new
desktop machine for this application.

__Up to 8 probes, plus NI channels:__

* Windows 7 or 10, 64-bit
* NI Platform Services 18.5 or later
* NI-DAQmx 18.6 or later
* Minimum 6 cores (Passmark rating > 13000)
* Minimum 3.5 GHz
* Minimum 16GB memory
* Graphics Card: Nvidia GeForce 1060 or better (see, e.g. Passmark ratings for video cards)
* Dedicated data drive (SSD or NVMe, 500+ MB/s)
* PCIe 8X slot for the PXIe controller

__Up to 16 probes, plus NI channels:__

* Windows 7 or 10, 64-bit
* NI Platform Services 18.5 or later
* NI-DAQmx 18.6 or later
* Minimum 8 cores (Passmark rating > 18000)
* Minimum 3.5 GHz
* Minimum 32GB memory
* Graphics Card: Nvidia GeForce 1060 or better (see, e.g. Passmark ratings for video cards)
* Dedicated data drive (NVMe 2000+ MB/s write rate)
* PCIe 8X slot for the PXIe controller

Notes

* GeForce cards require PCs with 400W power supplies
* Google '_model_name_ passmark' (e.g. W-2145 passmark)
* The data drive should be distinct from the system drive

#### Testing Existing Hardware

In SpikeGLX select menu item `Window\Run Metrics` to display a window
of performance measurements that provide some insight on whether the system
is running comfortably or struggling to keep up.

#### Our Test Computer Systems

The following are systems we've tested in our lab. These are not
recommendations (most of the hardware is no longer available);
they are illustrations of working systems.

More work needs to be done to better understand which system attributes
are the most significant.

![Test Systems](SysReqTblPXI.png)

#### Non-neural auxiliary channels

Imec BS cards have no non-neural input channels, except for a single SMA
connector that SpikeGLX uses to synchronize the card with other devices.
However, SpikeGLX can record concurrently from the imec cards and from
one or two additional multifunction IO devices to cover recording of
physiological data and trial marking signals, tightly synchronized with
the neural probe data.

SpikeGLX has two requirements for a multifunction IO device.

1. It must be an NI device that we can talk to via DAQmx (a general
purpose device programming language for NI hardware).

2. The device must support simultaneous sampling, which means that its
analog and digital inputs get sampled together in time instead of the
round-robin multiplexing sweep that would otherwise be used.

NI offers the following devices, which we have some experience with:

* PCI-based 6133
* PXI-based 6133
* USB-based 6366
* PXI-based 6366

The 6133 models have 8 analog and 8 digital inputs. We've tested the PCI
and PXI versions with NI chasses and with an ADLink chassis, and with and
without a Whisper multiplexer attached. These work without issues.

The USB-6366 offers more digital input channels on paper, but when used
with a Whisper, it successfully records analog channels but not digital.
This may be related either to its very small internal FIFO buffer size,
or the fact that DMA data transfer doesn't work over USB, so its effective
bandwidth is lower. Go ahead and use it if you already have one and don't
need digital channels.

The PXI-6366 is not yet tested.

#### PXI Chasses

We've successfully used these chasses with both imec BS modules and with
an NI PXI-6133 multifunction IO module:

* NI 1082 (8 slots)
* NI 1071 (4 slots)
* ADLink PXES-2301 (6 slots)

>What you need to know about the ADLink chassis is that it is very
attractively priced and well made, but the PXI-6133 didn't work with
this chassis right out of the box. That's because the routing of signals
from one internal terminal to another over the chassis backplane is very
slightly different. We got this to work fine, but it required some code
rewrites to make it work. If you own or want to purchase other NI modules
besides the 6133, then you might face some snags with this chassis. You
should try to test it first.

#### PXI Controllers

We've tested these remote control modules (Chassis <-> PC) links:

* NI PXIe-8398 (16 GB/s)
* NI PXIe-8381 (4 GB/s)
* NI PXIe-8301 (2.3 GB/s Thunderbolt)
* ADLink 8638  (4 GB/s)

These mix and match in in our chasses without compatibility issues. At this
time we can offer these additional remarks:

* The 8398 may well be overkill. It did everything asked of it, up to 16
probes, which is the maximum we tested to, but it is very costly, uses
up a PCI Gen 3 slot, and its thick cable is very unwieldy. In fact, the
cable is heavy enough to worry that it might not hold securely in the device
connectors which are somewhat flimsy.

* The 8381 and 8638 also performed perfectly in all tests out to 16 probes.
This is what we recommend at present. They are more affordable, small, and
need only Gen 2 slots.

* The Thunderbolt controller could only be tested in a laptop, which was
quite capable for a laptop (the Thinkpad X1 in the table above), but we
could only achieve stable performance up to 8 or perhaps 10 probes. We do
not know if the laptop or the link is the limiting factor.

We don't feel comfortable with less than 4 GB/s controllers for now.


_fin_


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
an additional multifunction IO device to cover physiological data and
trial marking signals, tightly synchronized with the neural probe data.

>SpikeGLX can actually operate two cards **provided they have identical
model numbers**. We are treating such a pair as a single device with
double the channel capacity.

SpikeGLX has these requirements for a multifunction IO device.

1. It must be an NI device that we can talk to via DAQmx (a general
purpose device programming language for NI hardware).

2. It must be an M-series (62XX), S-series (61XX) or X-series
(63XX) device.

We have direct experience with these:

* PCI-based 6221 (M)
* PCI-based 6133 (S)
* PXI-based 6133 (S)
* USB-based 6366 (X)

The 6133 models have 8 analog and 8 digital inputs. We've tested the PCI
and PXI versions with NI chasses and with an ADLink chassis, and with and
without a Whisper multiplexer attached. The Whisper drives its host device
at 800 kHz. The 6133 devices work flawlessly so are our favorite.

The USB-6366 offers more digital input channels on paper, but when run at
the Whisper sample rate, it successfully records analog channels but not
digital. Because this is a USB device it can't use DMA data transfers,
so its effective bandwidth is lower. Go ahead and use it if you already
have one and don't need digital channels.

Later models (S and some X) have a feature called 'simultaneous sampling'
which means each input channel gets its own amplifier and ADC. This allows
the device to sample all its channels in parallel at the advertised maximum
sample rate, for example, 2.5 mega-samples/s/channel for the 6133. Moreover,
there is no crosstalk between the channels. That's what makes these models
very capable and very expensive. These models are a bit more efficient
at transfering data to the PC than the M series. If planning to operate
16 or more imec probes at the same time (which we've tested) the system
will be under considerable stress and we think you'll be better off with
a unit that supports very high sample rates, hence, overall bandwidth.

When doing multichannel acquisition, non-simultaneous-sampling devices,
such as the 6221, use a multiplexing scheme to connect inputs to the
single amplifier/ADC unit in quick succession. The fastest you can drive
such a device depends upon how many channels you want to sample. It's
`R0/nChans`, where, R0 is the advertised maximum sample rate, 250 KS/s for
the 6221. Be aware that switching from channel to channel at this rate
does not allow the amplifier to fully settle before the next input is
connected to it, hence, there will be some crosstalk (charge carryover).
To avoid that issue, one can run at a lower effective maximum sample
rate given by: `1/(1/R0 + 1E-5)`. For, the 6221 example, you should sample
no faster than `71428/nChans`.

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


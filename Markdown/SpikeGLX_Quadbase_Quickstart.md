# SpikeGLX Quadbase Quickstart

**Topics**:

* [Quad-base Hardware](#quad-base-hardware)
* [System Hardware Requirements](#system-hardware-requirements)
    + [Computer Requirements](#computer-requirements)
    + [API4 and Tech Requirements](#api4-and-tech-requirements)
* [Critical Rules](#critical-rules)
    + [Can't Mix Tech](#cant-mix-tech)
    + [Plugs-in Startup](#plugs-in-startup)
* [Configure Run](#Configure-run)
    + [Two Consecutive Ports](#two-consecutive-ports)
    + [IMRO Table](#imro-table)
        + [Channels](#channels)
        + [References](#references)
    + [Save/File Channels](#savefile-channels)
        + [CatGT Splits Shanks](#catgt-splits-shanks)
* [Run Instability](#run-instability)
    + [Symptoms](#symptoms)
        + [Worker-Thread Activity](#worker-thread-activity)
        + [POP Errors](#pop-errors)
        + [Missed Samples](#missed-samples)
        + [Shank Disparity](#shank-disparity)
    + [Causes/Solutions](#causessolutions)
        + [SpikeGLX Filtered IM Stream](#spikeglx-filtered-im-stream)
        + [MATLAB](#matlab)
        + [Videos](#videos)
        + [MS Compatibility Appraiser](#ms-compatibility-appraiser)
        + [Hybrid Core Scheduling](#hybrid-core-scheduling)
* [Broken Shanks](#broken-shanks)


# Quad-base Hardware

### Probe

Quad-probes NP2020 and NP2021 (metal cap) are technically four single
shank 2.0 probes that are packaged together with 250 um shank spacing as
if they were a 4-shank probe. The difference is you can read out 1536
neural channels (384 from each of the shanks). The single-chip base
comprises the equivalent circuitry of four standard 2.0 bases.

Crucially, the four base circuits receive their start signals and sample
clock edges effectively simultaneously. That allows treating the whole
probe as a single 4-shank, 1536-channel entity, which is how SpikeGLX
models these probes.

>Actually 1540 channels are read from the probe: each shank/base supplies
384 neural channels and an SY channel (sync bit + error flags).

### Headstage

The bespoke QB headstage accommodates two QB probes in its two docks.

### Cable

The bespoke QB cable must be plugged into two ports at the same time.

--------

# System Hardware Requirements

## Computer Requirements

Each QB probe is at least as demanding upon compute resources, as four 2.0
probes. However, to create the illusion of a single 1536-channel probe,
SpikeGLX must acquire, process and merge the data from the four shanks
within each sample clock period, which places additional demands on CPU
performance. In particular, QB probes benefit more from single-thread
performance than other types of probes because these operations need to
be done within a single acquisition worker thread to maintain synchrony
and low latency. The following is excerpted from our [general
requirements](https://github.com/billkarsh/SpikeGLX/blob/master/Markdown/SystemRequirements_PXI.md)
document.

### For 1536-channel probes

* **CPU clock speed >= 3.0 GHz**
* **[PassMark Single-thread](https://www.cpubenchmark.net/single-thread/) score >= 3000**

**To run N 1536-channel probes:**

| Max Probes | CPU Cores |
| ---------- | --------- |
| 2          | 4         |
| 4\*        | 6 (8 preferred) |
| 8\*        | 20 (*to be tested*) |

> *\* Note: 6 cores (workstation or laptop) is marginally adequate for
(4) 1536-channel probes, meaning that any other activity on the machine could
easily cause the run to quit. We are more comfortable recommending 8 cores
to run 4 probes. For 8 probes, you will be safe with a
[PassMark Multi-thread](https://www.cpubenchmark.net/multithread/) score >= 50000.*

## API4 and Tech Requirements

The earliest SpikeGLX version to use with quad-probes is
**20250525-API4**.

With this version, SpikeGLX and imec move to major API revision 4.0,
or `API4`. API4 allows imec to better support a wide variety of hardware
technologies (`tech`). Probes/headstages have tech types {STD, QB, OPTO, NXT}
and basestation-modules are {STD, OPTO, NXT}. These have to be matched to
work properly. The rules are pretty simple:

* OneBox is a STD tech.
* Existing STD-PXI modules are STD tech.
* Existing OPTO-PXI modules are OPTO tech.
* OPTO-probes ONLY work in OPTO modules.
* NXT-probes ONLY work in NXT modules.
* QB-probes ONLY work in STD module tech.
* STD-probes work in any module.

QB summary:

* QB-probes work in your existing STD PXI module or OneBox.
* SpikeGLX is tech-aware and makes sure you do this correctly. 

--------

# Critical Rules

## Can't Mix Tech

You can't run QB together with non-QB probes in the same module. 

## Plugs-in Startup

The following is only for PXI modules, not OneBox...

The ports in a PXIe module undergo an initialization procedure at
power-up of the chassis/module. This initialization cannot be repeated
after the power is already on. Initialization won't work properly for
QB without a cable present in the port...

You must plug an imec cable of any type into each port that you intend
to use for QB probes, **before you power-on the chassis**. It doesn't
have to be a QB cable.

Do this whenever you cycle power on the chassis.

--------

# Configure Run

Think of a quad-probe as a 4-shank probe with more channels...but there
are a few special considerations as follows.

## Two Consecutive Ports

* A QB cable has two USB-C port connectors. Plug them into a pair of
consecutive ports, either (1,2) or (3,4). It doesn't matter if you are
using both docks of the headstage, always **plug in both connectors**.

* In the SpikeGLX `Probe Table` on the `Devices` tab (where you `Detect`)
you have to **check both boxes**, (1,2) or (3,4).

## IMRO Table

### Channels

The IMRO editor for QB guides you to select 384 required channels for each
of the four shanks.

### References

QB offers {ext, gnd, tip}, where tip means the tip on the same shank as
that electrode. There is no join tips option as the shanks are electrically
separate.

## Save/File Channels

Per timepoint, the full channel set for QB includes 1536 full-band neural
channels, followed by 4 SY channels, one from each shank.

## CatGT Splits Shanks

If you prefer to work with the shanks individually in post-processing,
use the following CatGT option to save each shank in a separate file:

```
-sepShanks=ip-in,ip-out0,ip-out1,ip-out2,ip-out3
```

--------

# Run Instability

Quad-base require significant compute capacity to acquire data from four
probes (shanks), pre-process those data and merge them together into one
data stream. Most of the time this works just fine, but here we cover
what happens when it fails, and what you can do to help avoid problems.

## Symptoms

If the worker thread that is handling the QB probe is unable to keep up
with the required ~30kHz data rate, the hardware feels `backpressure`, and
a characteristic sequence of pathologies ensue.

When SpikeGLX detects certain performance issues, it now automatically
opens the Metrics window (Ctrl+M). In this case, evidence of backpressure
will be visible as follows...

### Worker-Thread Activity

The first thing to happen is the `worker-thread activity %` will increase
dramatically. The processing thread that gets data from the probe will work
much harder to keep up with a building backlog of data enqueued in the
hardware buffer.

### POP Errors

If the lag is too great, the hardware may detect overruns of its sample
buffers and `POP` errors are reported. You can see this in the `Errors`
section at the top of the Metrics window. These error flags are also
recorded in your run's metadata.

### Missed Samples

Usually, due to buffer overruns, data samples are lost before they can be
delivered to the GUI. SpikeGLX detects such discontinuities and reports
these events as `missed samples`. This is visible also in the `Errors`
section of the Metrics window, in your metadata, and in logs that are
named for your run and saved in the top level of your data directory.
The logs detail where the missed sample segments are in the data and how
long they are. In addition, SpikeGLX will replace runs of missed samples
with zeros to maintain the true length of the data stream. That's also
covered in the `Metadata_Help.html` document.

### Shank Disparity

Finally, due to missed samples, it may happen that the four shanks become
out of phase with each other, that is, some shanks have a few more or fewer
samples that others. We call this `shank disparity`. Disparity is constantly
monitored in SpikeGLX and significant changes are reported in the Metrics
and Log windows.

A few samples of disparity are tolerable and can even self-heal if the
backpressure is temporary. However, if the disparity between any two shanks
grows beyond 10 samples, SpikeGLX will throw a 'Shanks Desynchronized'
error and stop the run on that basis.

## Causes/Solutions

Unfortunately there are several possible causes of backpressure that can
lead to a dreaded shank disparity error, but the majority can be avoided
by your deliberate care.

### SpikeGLX Filtered IM Stream

On the `IM Setup` tab of the `Configuration` dialog is a checkbox to enable
the feature called `Filtered IM stream buffers`. When checked, each history
stream for an imec probe gets a twin history stream in which the data are
band-pass filtered and common average referenced. These data dramatically
improve the quality/clarity of {Audio-out, ShankViewers, SpikeViewers}, but
come at the cost of increased workload for the data acquisition worker
threads. It's an especially high tax for high channel count probes.

The feature includes some self-protection in that it temporarily disables
if the number of pending samples in the probe's fifo buffers is growing.
It resumes operation when backpressure subsides.

Also, even when enabled on the `IM Setup` tab, the feature is inactive
unless these data are being requested by at least one client, again,
{Audio-out, ShankViewer, SpikeViewer, Remote SDK request}.

Nevertheless, if you are running a large number of probes and/or taxing
the machine with remote scripted operations that also need CPU time, then
either disable this feature at the checkbox, or use it very judiciously,
by limiting reference to your QB probes in audio, ShankViewer, SpikeViewer,
etc.

### MATLAB

MATLAB isn't too bad once it is up and running...

But MATLAB tends to hog all the compute power on the machine to start up.
Just be careful to start MATLAB first, and then start your acquisition run
after the machine has settled back to normal work levels.

MATLAB is a known offender, but there can be others having this same
pattern wherein launching that service causes a large spike in CPU
activity that starves our acquisition worker long enough to kill a run.

### Videos

Try to avoid watching videos or launching any content (Browser Window)
that contains video because this can potentially eat compute resources.
Again, this is a large activity spike problem.

### MS Compatibility Appraiser

There are innumerable tasks that run periodically on your machine. They
are scheduled to run by the `Windows Task Service`. You can locate that
as a subsection of the `Computer Management` control panel. Any of these
tasks could be a problem, but the most glaring offender of all is
`Microsoft Compatibility Appraiser` task which is typically scheduled to
run once per day. When it runs it can push every core on the machine to
100% load for 15 to 30 seconds. It's trying to discover if your machine
can be or should be upgraded to Windows 11.

Find this task in the Window Task Scheduler GUI, under:

```
Microsoft/Windows/Application Experience
```

You can highlight this task, and on the far right-hand action panel you'll
see a Disable button. You can disable this if it is proving to be a problem.
It does not typically pose as much of a threat on machines that already have
Windows 11.

### Hybrid Core Scheduling

More and more modern computers are using hybrid processors with mixtures
of high performance P-cores and high efficiency E-cores. SpikeGLX is already
doing what it can to get itself to run on the available P-cores. However,
if your machine has been aggressively tuned by the vendor to be maximally
efficient (Lenovo to name one) it may still try to throttle core power
or even switch our worker threads to E-cores if it is hitting temperature
or voltage limiters. Be sure that your CPU management drivers are up-to-date
and that all the throttling and tuning component services are running
on your machine. Especially beware of IT departments that replace a working
OS image from the vendor with their own suite that enhances their security
at the cost of your tuned performance.

--------

## Broken Shanks

Inevitably shanks "break" which usually means it looks intact externally
but an internal trace has broken. And this may happen to just one shank
of your very expensive quad-probe. Can you still get some use out of it?

Not yet...

There is an enhanced `Shift Register` BIST that now reports which shanks
are good. That works for both QB and 2.0 4-shank probes.

However, there is not yet a facility to program and run with the intact
shanks. That will come soon, so do not discard your probe yet.


_fin_


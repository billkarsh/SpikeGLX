# Metrics: Capture Signs of Trouble During A Run

# Performance Metrics Box

>The disk performance measures do pertain to the NI data stream, but
most metrics are specific to imec devices.

## Errors

```
GOOD: No entries in this section.
BAD:  Any entries in this section.
```

For each imec stream we monitor the cumulative count of several error flags,
hence, imec05 denotes the flags for probe steam 5. Note that quad-base
probes (part number NP2020) record flags for each shank, hence, quad03-2
denotes that stream 3 is a quad-probe and the flags are from shank 2.

The flags are labeled {COUNT, SERDES, LOCK, POP, SYNC, MISS}.

These flags correspond to bits of the status/SYNC word that is visible as
the last channel in the graphs and in your recorded data (quad-probes have
4 separate status words):

```
bit 0: Acquisition start trigger received
bit 1: not used
bit 2: COUNT error
bit 3: SERDES error
bit 4: LOCK error
bit 5: POP error
bit 6: Synchronization waveform
bit 7: SYNC error (unrelated to sync waveform)
bit 11: MISS missed sample
```

The MISS field counts the total number of samples that the hardware has
missed due to any of the other error types. SpikeGLX inserts zeros into
the stream to replace missed samples, and each of those samples has the
MISS bit set in the status word.

The inserts are recorded in a file at the top level of your data directory.
The name of the file will be "runname.missed_samples.imecj.txt" for probe-j
or "runname.missed_samples.obxj.txt" for OneBox ADC stream-j. There is an
entry in the file for each inserted run of zeros. The entries have the form:
<sample,nzeros>, that is, the first value is the sample number at which the
zeros are inserted (measured from the start of the acquisition run) and the
second value is the number of inserted zeros. Each inserted sample is all
zeros except for the status word(s) which set bit 11 and extend the flags
from the neighbor status words.

>It is possible to see which region of recorded data experienced these
errors if you see blips on those bits.

--------

## Acquisition: FIFO Filling

```
GOOD: Lower than    4% full.
BAD:  Greater than 20% full.
```

Samples acquired from each imec stream are held in a FIFO buffer on the BS
card until the SpikeGLX acquisition loop fetches them into the computer for
processing. If the computer is running slow the buffers will start to fill
up which is NOT what you want. The only harm in running with a small amount
of filling is that the latency is a little longer.

If the level rises to 95% full for any stream SpikeGLX will stop the run
before an overflow (data loss) occurs.

--------

## Acquisition: Worker Thread Activity

```
GOOD: Lower than   75% active. // if NP1.0 or NP2.0
BAD:  Greater than 90% active. // if NP1.0 or NP2.0
```

This is akin to CPU activity level you may be familiar with in the Windows
Task Manager. SpikeGLX runs one or more worker threads (sub-processes).
Each worker is responsible for acquiring data from between one and three
probes/OneBoxes, and it marshals those data into the history streams where
you can graph it, record it, and so on. The workers check for fresh data to
process about 500 times per second to keep the latency low. When there
isn't much to do, a worker sleeps and allows other threads more time to
do their jobs. This metric shows the percentage of time that a stream,
really, its worker, is awake and processing data.

If the system is running very comfortably the workers will easily handle
their task and be able to snooze often. If the system is struggling and
work is piling up the workers will have to go into overtime to catch up.
If SpikeGLX can not catch up, the run may be stopped due to a FIFO
overflow or other exceeded limit.

> Note: High channel count probes, like quad-base NP202X or NXT NP30XX,
need more worker activity to keep up. Values greater than 75% are
considered normal for these probes.

> Note: If you check the `Low latency` box on the `IM Setup` tab of the
`Configure` dialog, the data fetching workers run at full speed without
yielding. The activity will be reported as a normal 100%.

### Dealing with stress.

* **Other Applications**: Momentary stresses are caused by launching
Excel, MATLAB or other resource hogging bursts of activity. Try to run
as few applications as possible to prevent overtaxing the CPUs.

* **Filtered IM Streams**: This feature is accessed on the `IM Setup` tab.
Although these streams dramatically improve signal to background for audio
output and the ShankViewers, they are very compute intensive, especially
for high channel count probes like the NP2020 Quad-base and NP3XXX NXT
probes.

* **Audio**: Listening to audio channels is a huge burden on the system.
It's usually fine unless you are recording to disk, which is one of
the next biggest stressors. To ensure successful recording, try not to
run audio when not needed.

* **More Traces**: This window doubles the graphing work load, try closing
the secondary Graphs window to allow more resource for critical recording.

* **Fewer Probes**: Every PC will have an inherent limit to the number of
imec probes it can handle. It takes a modern workstation with NVMe drives
and 7th or 8th generation CPUs to manage 16+ probes.

--------

## Disk: Write Buffer Filling

```
GOOD: Lower than    5% full.
BAD:  Greater than 40% full.
```

When recording is in progress, each stream transfers data to be written
from the history stream into a holding `write buffer`. A worker thread
transfers the data from the write buffer to the disk when the disk is
available. This adds a little extra tolerance against data loss should
the disk be running slow.

There values are the worst case filling percentage for each type of stream
in the run.

These data are also shown as `FileQFill%` in the status bar at the
bottom of the Console window when recording is enabled.

--------

## Disk: Actual (and Required) Write Rate

```
GOOD: Actual and Required match.
BAD:  Actual lower than Required.
```

There are two values shown. The average write rate in MB/s for all of the
streams in the run and the target write rate needed to keep up.

These data are also shown in the status bar at the bottom of the Console
window when recording is enabled.

--------

## Disk: Fetch Depth

```
GOOD: Higher than 50%.
BAD:  Lower than  10%.
```

Think of the history stream as a conveyor belt where samples from the
hardware are placed on the right hand side and they travel on the belt
to the left. Any client needing access to samples {audio, disk,
graphing, MATLAB, ...} can pick them up off the belt anywhere, but
once data items get all the way to the left end they are discarded
to make room for new data entering on the right. So data on the right
are fresh/new and left-hand data are oldest.

This metric describes where the trigger (file writing) worker is getting
its samples, as a percentage of the distance along the stream (the belt)
from left to right. High values mean we are pulling the latest samples,
which we should be able to do unless the system is bogging down. If the
workers responsible for getting data are running slow and falling behind
then their target samples will have travelled some distance to the left
before being picked up for recording.

SpikeGLX will stop the run if any recording streams are so late that
their data have dropped off the left-hand side.

> Note: If the trigger client persistently reports `101%` that tells us
this client is running alright but the history queue isn't getting fresh
samples at an adequate pace. Either the acquisition worker thread that
moves data from the hardware buffer to this history queue is running too
slowly, or the flow of data into the hardware buffer may have stopped.

--------

# Errors and Warnings Box

The box captures all the error and warning messages that are also being
sent to the main Console window, but only within the span of the current
run.

The box is cleared at the start of the next run.


_fin_


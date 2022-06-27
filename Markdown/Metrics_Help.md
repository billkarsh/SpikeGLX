# Metrics: Capture Signs of Trouble During A Run

# Performance Metrics Box

>The disk performance measures do pertain to the NI data stream, but
most metrics are specific to imec devices.

## Errors

```
GOOD: No entries in this section.
BAD:  Any entries in this section.
```

For each imec stream we monitor the cumulative count of several error flags.
These are labeled {COUNT, SERDES, LOCK, POP, SYNC}. These data are only
posted if any errors are happening.

Any instances of these errors implies that samples have been dropped. It is
not possible to tell how many samples are dropped from these counts. All
you can tell is that some of the data being transmitted from the device are
corrupt or missing.

These flags correspond to bits of the status/SYNC word that is visible as
the last channel in the graphs and in your recorded data:

```
bit 0: Acquisition start trigger received
bit 1: not used
bit 2: COUNT error
bit 3: SERDES error
bit 4: LOCK error
bit 5: POP error
bit 6: Synchronization waveform
bit 7: SYNC error (unrelated to sync waveform)
```

>It may be possible to see which region of recorded data experienced these
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
GOOD: Lower than   75% active.
BAD:  Greater than 90% active.
```

This is akin to CPU activity level you may be familiar with in the Windows
Task Manager. SpikeGLX runs one or more worker threads (sub-processes).
Each worker is responsible for acquiring data from between one and three
probes/Oneboxes, and it marshals those data into the history streams where
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

### Dealing with stress.

* **Other Applications**: Momentary stresses are caused by launching
Excel, MATLAB or other resource hogging bursts of activity. Try to run
as few applications as possible to prevent overtaxing the CPUs.

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

This metric describes where the recording system is getting its samples,
as a percentage of the distance along the stream (the belt) from left
to right. High values mean we are pulling the latest samples, which we
should be able to do unless the system is bogging down. If the workers
responsible for getting data are running slow and falling behind then
their target samples will have travelled some distance to the left before
being picked up for recording.

SpikeGLX will stop the run if any recording streams are so late that
their data have dropped off the left-hand side.

--------

# Errors and Warnings Box

The box captures all the error and warning messages that are also being
sent to the main Console window, but only within the span of the current
run.

The box is cleared at the start of the next run.


_fin_


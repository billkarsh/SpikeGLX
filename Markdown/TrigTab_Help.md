## Triggers Tab

**A file is being written when the trigger is high**.

There are currently 5 trigger modes:

* Immediate start
* Timed start and stop
* TTL controlled start and stop
* Spike detection start and stop
* Remote controlled start and stop

--------

## Immediate Start

As soon as the gate is enabled the trigger is immediately set high and it
simply stays high ("latches high") until the gate is closed or the run
is stopped.

Select 'Immediate' mode for both {gate, trigger} modes to start recording
as soon as the run is started, *subject to
[gate manual override settings](GateTab_Help.html#gate-manual-override)*.

--------

## Timed Start and Stop

_Optionally_ wait L0 seconds, then either:

* Latch high until the gate closes, or,

* Perform a sequence:
    - Write for H seconds,
    - Close/Idle for L seconds,
    - Repeat sequence N times, or until gate closes.

--------

## TTL Controlled Start and Stop

Watch a selected (analog or digital) channel for a positive going
threshold crossing, then either:

* Latch high until the gate closes, or,

* Write for H seconds, or,

* Write until channel goes low.

_The latter 2 cases get flexible repeat options_:

>Notes:
>
>+ Perievent margins can be added to the recorded files.
>+ Threshold detection is applied to unfiltered data.
>* Threshold voltages should be the real-world values presented to the sensor.
It's the same value you read from our graphs.

--------

## Spike Detection Start and Stop

Watch a selected channel for negative going threshold crossings:

* Record a given perievent window about each to its own file.
* Repeat as often as desired, with optional refractory period.

>Notes:
>
>* Perievent margins can be added to the recorded files.
>* Threshold detection is applied after a 300Hz high-pass filter.
>* Threshold voltages should be the real-world values presented to the sensor.
It's the same value you read from our graphs. For example, a threshold for
neural spikes might be -100 uV; that's what you should enter regardless
of the gain applied.

--------

## Remote Controlled Start and Stop

The trigger level can be commanded high or low using the `TriggerGT`
remote API command.

>We retain support for a legacy program called "StimGL." StimGL can
send {SETTRIG 1, SETTRIG 0} commands to the "Gate/Trigger" server.

>Normally an NI or OneBox device is used for TTL inputs, but the
[imec SMA connector](SyncTab_Help.html#imec-sma-connector)
can be used in special circumstances.


_fin_


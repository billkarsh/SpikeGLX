## Gates Tab

There are currently 2 gate modes:

* Immediate start
* Remote controlled start and stop

--------

## Immediate Start

As soon as the run starts the gate is immediately set high and it simply
stays high ("latches high") until the run is stopped.

>If the optional `Enable Recording` button is showing in the Graphs Window,
you will have to click it (or remotely call SetRecordingEnable()) to permit
the gate to go high. (See below).

--------

## Remote Controlled Start and Stop

The gate can be commanded high or low using the `TriggerGT` remote API
command.

>If the optional `Enable Recording` button is showing in the Graphs Window,
you will have to click it (or remotely call SetRecordingEnable()) to permit
the gate to go high. (See below).

>We retain support for a legacy program called "StimGL." StimGL can
send {SETGATE 1, SETGATE 0} commands to the "Gate/Trigger" server.

--------

## Gate Manual Override

You can optionally pause and resume the normal **gate/trigger** processing
which is useful if you just want to view the incoming data without writing
files or if you want to ability to stop an experiment and restart it
quickly with a new run name or changed gate/trigger indices. See the
discussion for [continuation runs](SaveTab_Help.html#run-name-and-run-continuation).

To enable manual override:

On the `Gates Tab` check `Show enable/disable recording button`. The
button will appear on the Graphs Window main `Run Toolbar` at run time.

If this button is shown you also have the option of setting the initial
triggering state of a new run to disabled or enabled.

>**Default: Show Button, Initially Disabled.**

>_Warning: If you opt to show the button and to disable triggering, file
writing will not begin until you press the `Enable` button._

>_Warning: If you opt to show the button there is a danger of a run being
paused inadvertently. That's why it's an option._


_fin_


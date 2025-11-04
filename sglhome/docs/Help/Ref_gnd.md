# Noise: Reference and Ground

## Overview

Imec probes have two solder connections: reference and ground.
This note covers what they are and how to use them.

### The Ground Line

**Always Connect To CSF.**

This is an extension of the basestation's wall ground. However, because the
ground runs through a 5-meter, thin wire, the probe ground is about +650 mV
higher than wall ground. This sets the ground level for the probe's base
circuits that sample, amplify, filter, digitize and transmit neural signals.

Generally the subject should be grounded, whether an animal or buffer, for
these reasons:

* **Damage**: If the subject is floating and holding a static charge, the
probe could get zapped and damaged.

* **Saturation**: If subject potential is far from that of the electronics,
the amplifiers can saturate. The reading are then unusable. Moreover, the
amplifiers can sometimes lock into saturation until restarted, which wastes
time.

* **Noise reduction**: Connecting the subject to ground reduces its ability
to act as an antenna. Although the differential nature of the measurements
acts to cancel common noise, it's better to drain the noise to ground before
it ever gets to the amplifiers.

In summary, always ground the animal: run a wire from the ground terminal
of the probe flex to the animal CSF, either via a pin through the skull
into CSF, or placed in the saline in the craniotomy.

>IMPORTANT: The probe ground should be the **ONLY** ground connection to
the animal. Otherwise you'll make ground loops which are noise antennas.

For self-referenced noise measurements in a bath, we also recommend connecting
probe ground to the bath to act as a noise drain. This is a better measurement
of the probe's own internal electronics noise. It's also more similar to an
animal ground configuration. It is highly recommended to do self-referenced
noise measurements **one-probe-at-a-time** to prevent any possibility of
multiple probes interacting with each other.

### The Reference Line

**Flexibility: Depends On Needs.**

All imec probes are designed for differential measurements. The electrode
sites connect to (+) amplifier inputs, and the selected reference level
connects to the (-) inputs. The benefit of differential measurement is
common mode noise rejection. **This only works if the reference is connected
to the animal**.

The system supports a variety of reference configurations to meet your needs:

* **External**: Select `External` in software (in the `IMRO editor`), and
connect the probe's reference terminal to any desired point in the lab.
That could be ground. However, it might be a wire in the brain, far from
the imec probe. **BTW, external is the default option**.

* **Ground**: Some probes (mainly 2.0) offer a software option for `Ground`
referencing. This internally connects the (-) amplifier inputs to circuit
ground. Using this mode, you can make measurements relative to ground
without soldering a wire to the probe's reference terminal. *BTW, EE's
call this a "single-ended measurement," as opposed to a differential one*.
If your probe doesn't support the software `Ground` option, you can select
`External` and solder a wire from the reference terminal to a ground point.

* **Tip**: This internally connects the given shank tip electrode to the (-)
inputs. As with the `Ground` option, this does not require any wire to be
connected to the reference terminal on the flex.

>**Pro tip**: A tiny internal switch is used to connect the (-) amplifier
input to the selected reference. Only the `External` mode **requires** a
wire from the reference terminal on the flex to some point in the brain.
In any other mode, having a reference wire connected is OK, but it isn't
doing anything valuable. The *ideal practice* is to omit the reference wire
entirely to minimize crosstalk from that terminal, across the switch. The
worst thing is to connect a long free wire to the reference terminal, making
a noise antenna.

### Standard Practice For Spikes

The most important thing for referencing in spike measurement is low noise.
There isn't a rule about what reference to use or where to put wires.

For AP band it probably doesn't make a lot of difference where you reference.
Different choices will differ from each other primarily in DC offset, and
hopefully lowish frequencies, and that will be removed by the band-pass
and CAR filters you'll use in postprocessing.

So what does everyone else do? It's helpful to allow a main and a backup
option, and have the convenience of a single extra wire coming off the
probe. The wiring part of the strategy depends on probe model:

* **2.0 with `Ground` Option**: Connect one wire from the flex ground
terminal to the CSF.

* **{1.0, NHP, Ultra} w/o `Ground` Option**: Solder the reference and ground
terminals together at the flex and run a single wire from there to the CSF.

This way you can start with ground referencing, and if the noise is small,
you're all set. If it's too noisy, you may have a contaminated ground, so
just switch to `Tip` referencing in software without changing any wires. That
may look better. So with this setup you get two chances to get data today,
and you can try to get rid of the noise in your lab tomorrow.

### LFP Measurements

**Use External or Ground Modes.**

LFP tends to vary slowly over large areas. Tip referencing may be an
especially bad choice: for sites near the tip the differential measurement
will cancel signal as well as background, and at the other end of the probe
measurements contain (local LFP - tip LFP), so the referencing is inconsistent.
Moreover, you'd rather have a larger area electrode for better averaging.

### Noise

To minimize noise use low impedance paths for reference and ground:

* [Do a good job with soldering](https://billkarsh.github.io/SpikeGLX/help/solder/solder/) any and all the wires.
* Use beefy wires and ground straps where you can.
* Don't use hacks like alligator clips to connect things.


_fin_


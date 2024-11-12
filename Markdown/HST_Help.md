## Headstage Tester (HST)

### Overview

A headstage can be checked by plugging an imec HST dongle component
into that headstage (HS) exactly as you would plug in a probe, and
then running the tests in the HST Diagnostics dialog.

| Test                  | Functional Area |
| --------------------- | -------------------------------- |
| *Communication*       | PCLK signal and I2C bus |
| *Supply Voltages*     | Power connections |
| *Control Signals*     | NRST control signals |
| *Master Clock*        | 93.6 MHz master clock |
| *PSB Data Bus*        | Data bus connectivity |
| *Signal Generator*    | BIST signal path |

```
Terminology:
    HW     = hardware
    SW     = software
    PC     = computer
    BIST   = built-in self test
    I2C    = inter-IC bus
    BS     = base station card
    BSC    = base station connect card
    HS     = headstage
    NRST   = State reset signal
    PCLK   = PSB clock
    PSB    = parallel serial bus
    PRBS   = pseudo random binary signal
    SerDes = serializer/deserializer
```

### Communication

The PCLK test checks the serializer and the connection between the tester
and headstage. A failure here is similar to getting a NO_LOCK with a probe.

The I2C bus is the command/control pathway between HS and probe/tester. If
this is not working no other tests can run.

### Supply Voltages

This checks the presence of four HS power supply voltages:

- 1.8V analog
- 1.8V digital
- 1.2V analog
- 1.2V digital

### Control Signals

The HST module can check the functionality of the NRST and the REC_NRESET
signals between the ZIF connector and serializer on the HS.

### Master Clock

The HST module can verify whether the MCLK clock signal generated on the
HS is present.

### PSB Data Bus

The HST module can check the connectivity of the PSB_DATA<0:6> and
PSB_SYNC signal on between the ZIF connector and the serializer on
the HS. The function first checks PSB_SYNC and continues with PSB_DATA<0>
to PSB_DATA<6> consecutively. The function returns at the first failing
signal, without checking the remaining signals.

Note: an electrical short-circuit on the input pins of the serializer
cannot be detected with the current version of the HST module.

### Signal Generator

The HST module can test the functionality of the test signal generator
which is located on the HS and which is primarily used for the BIST pixel
signal path test.

The test involves two signals: the output signal coming from the HS:
CAL_SIGNAL and the input signal going to the HS which is used to enable
or disable the test signal generator, and which is normally coming from
the probe: OSC_PDOWN.

_fin_


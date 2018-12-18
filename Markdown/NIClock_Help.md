## Clock Source Notes

### Table of Calibrated Values

SpikeGLX maintains a table of NI clock sources in the file

* `_Calibration\calibrated_sample_rates_nidq.ini`

The entries in this table have the form:

```
Name : Set_rate=Measured_rate
```

* **Name**: This is just a label you provide to help you recognize
different sources you might use in your lab. We expect you might
pick a name like `Whisper` if you are using a Janelia-built
Whisper multiplexer, or `Internal` if electing to use program
the NI device's own internal Ctr0 clock.

* **Set_rate**: This is the rate that is used to program a clock,
for example, a Whisper's clock is nominally 25000 Hz, or you
might set 30000 Hz to program the NI internal clock to match the
rate of imec probes.

* **Measured_rate**: Until you have done a calibration the measured rate
is initialized to be the `Set_rate`. When you use the SpikeGLX calibration
features, the measured rate is updated with a more accurate measurement
result. In any case, the `Measured_rate` is used as the best available
estimator of the true sampling rate for the device.


_fin_


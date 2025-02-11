## Calibration from Existing Runs

### Overview

Use this dialog to analyze the sample rates in any existing runs that were
acquired with square wave inputs.

#### Specify Operation

1) Select any single binary file from a run of interest.
2) Choose whether to analyze only that stream or all the streams in the run.
3) Click `Go` to start analysis.

#### Output

This box shows analysis progress and then the old and newly analyzed results
for the selected streams.

#### Evaluating the Results

- Standard errors on measurements spanning 20 minutes or longer should
be less than ~0.01 Hz. If larger, there is some instability to fix
before trusting the measurement.

- If you are measuring a given clock for the first time, the difference
from the manufacturer's uncalibrated rate should be less than ~2 Hz.

- If you are remeasuring a calibrated clock, the new and old rate should
be within ~0.1 Hz of each other. Otherwise, one of the calibrations may
have failed.

If you don't trust the results, do not click 'Apply.' Rather, try to find
the problem, and then repeat the measurement. Good places to look for
issues are the reported error and warning messages in the `Log` window.
Also, try running with the `Metrics` window (Ctrl+M) open to look for
warning signs. Note that imec error flags, which signify possibly dropped
samples, are visible in the Metrics window and are always recorded in
probe metadata files.

Note too, that whether doing an online calibration run, or calculating
rates from an existing run, 20 minutes should be considered an absolute
minimum time span for calibration, but 40 minutes or longer is preferred.

#### Apply

All results producing (measured rate +/- std error) are applied.

Select whether to use the results to:

* Make new runs,
* Update the metadata for this run with the new results,

or both.


_fin_


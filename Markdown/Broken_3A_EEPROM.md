## Working with Broken Phase-3A EEPROMs

### Probe Labeling

**4/30/2018 and later**:

* Labels on probes are *usually* 11 digits long, for example: `15039707141`.
* Labels *usually* start with `1`.
* The option is **NOT** encoded among the digits...
* ...Rather, you must determine the option from your records.
* The serial number is 9 digits but may be the middle 9 or the right-most 9.

**Prior to 4/30/2018**:

* Labels on probes are 11 digits long, for example: `15039707141`.
* Labels always start with `1`.
* The last digit is the option number.
* The middle 9 digits are the serial number, here, `503970714`.

>Note: To program a broken probe we must know the
`full identifier printed on the probe` and the `option` number.
The serial number is essentially arbitrary. It is useful for
communicating problems to Imec, and for your experiment metadata. Using
"the wrong" 9 digits will not affect the probe or experiment outcomes.

### EEPROM Chip

Probes have a tiny EEPROM chip (`circled in red`) containing the probe
serial number, option, and ADC/gain calibration data.

![EEPROM](EEPROM.png)

If the chip detaches or breaks then SpikeGLX displays the following
probe info when you click the 'Detect' button in the Devices tab:

* Probe serial# 107...
* Probe option 4

1. Note that true serial numbers never start with '1' and are only 9
digits long, whereas the bogus serial numbers usually start with '1'
and are often longer.

2. Although there are real option 4 probes, a broken EEPROM always
presents as option 4.

The probe can still be used through a manual override feature. First
ask the manufacturer for the calibration file set matching the probe's
printed (nominally 11-digit) identifier. Then follow the instructions
below to command SpikeGLX to use the files instead of the EEPROM chip.

>**Note: You will have to revisit the 'Force' dialog for this probe
each time you quit and relaunch SpikeGLX**.

### Probe Data Folder

A probe's data folder looks like this:

```
    15039707141\
        Comparator calibration.csv
        Gain correction.csv
        Offset calibration.csv
        Slope calibration.csv
```

### Install a Data Folder

If you need to access a broken probe, get its data folder and drag it
into subfolder `SpikeGLX/ImecProbeData`.

If you don't see the `ImecProbeData` subfolder, create it yourself with
exactly that case and spelling.

### Using the "Force" Features

Click `Detect` on the `Devices` tab when you want to:

* Clear the force-ID flag (shown in the imec message box)
* Clear the skip-calibration flag
* Make a fresh attempt to read data from the EEPROM

Click `Same As Last Time` to keep current flag settings for the next run.

Click `Force...` on the `IM Setup` tab to access data override features:

#### Code Entry in the Force dialog

**Label**: Enter the (nominally 11-digit) code printed on the probe, or
click `Explore ImecProbeData` to copy it from the probe's folder so you
can paste it into the box.

**Serial #**: Enter the 9-digit serial number directly, or click
`Take 9 From Label` to create a 9-digit number based on the label.

**Option**: Remember to select the appropriate option number for this
probe. **This is not derived from the label as of 4/30/2018**.

#### Skip ADC Calibration

This is not recommended, but available in cases where the probe data
files are corrupt or you just don't have them and still want to use
your broken probe.


_fin_


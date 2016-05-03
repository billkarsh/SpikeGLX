## Probe Data Override Notes

### Probe Labeling

* Labels on probes are 11 digits long, for example: `15039707141`
* Labels always start with `1`
* The last digit is the option number
* The middle 9 digits are the serial number, here, `503970714`

### EEPROM Chip

Probes have a tiny EEPROM chip (`circled in red`) containing the probe
serial number, option, and ADC/gain calibration data.

![EEPROM](EEPROM.png)

If the chip detaches or breaks the probe would become unusable...but the
probe still has its label and the data within the chip can be read from
data files obtained from the manufacturer.

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

### Using the Force Features

Click `Detect` on the `Devices` tab when you want to:

* Clear the force-ID flag (shown in the imec message box)
* Clear the skip-calibration flag
* Make a fresh attempt to read data from the EEPROM

Click `Same As Last Time` to keep current flag settings for the next run.

Click `Force...` on the `IM Setup` tab to access data override features:

#### Serial Number Entry

You can type the 9-digit serial number directly into the text box.

Alternatively, you may want to copy the 11-digit name of the `probe data folder`
using Windows File Explorer and paste that into the text box. Use the
`11 -> 9` button to strip that to the middle 9-digit serial number.

#### Skip ADC Calibration

This is not recommended, but available in cases where the probe data
files are corrupt or you just don't have them and still want to use
your broken probe.


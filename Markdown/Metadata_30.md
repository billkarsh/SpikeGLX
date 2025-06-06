# Metafile Tags

* Typical right-hand-side values are shown for each tag illustrating whether
its data are integers, floating-point values or text strings.

* Inapplicable values may be blank, like so: `niXAChans1=`

* Inapplicable tags may be absent.

**Subsections:**

* [Common to All Files](#common-to-all-files)
* [If Using Timed Trigger](#if-using-timed-trigger)
* [If Using TTL Trigger](#if-using-ttl-trigger)
* [If Using Spike Trigger](#if-using-spike-trigger)
* [Nidq](#nidq)
* [If Using 2nd Nidq Card](#if-using-2nd-nidq-card)
* [Imec](#imec)
* [Obx](#obx)

--------

## Common to All Files

```
appVersion=20201024
```

```
fileCreateTime=2020-08-29T00:27:54
```

These are the local date and time on the recording machine.

```
fileName=C:/SGL_DATA/qqq1_g0_t0.nidq.bin
```

This is the original path to the binary file paired with this metadata file.

```
fileSHA1=B209BBB956A9F6371625C118D651DBE9AED4051D
```

This is a checksum calculated for the binary data. Use menu item
`Tools::Verify SHA1` to detect if the binary data have been altered since
the file was first recorded.

```
fileSizeBytes=10144818
```

This should match what you see when you right-click on the binary file in
the 'Windows File Explorer' and select 'Properties'. This is the 'Size'
value, **not** the 'Size on disk' value.

```
fileTimeSecs=1.0
```

This is the span of the file data in seconds and is calculated as:

```
    fileTimeSecs = fileSizeBytes / xxSampleRate / nSavedChans / 2,
    where, xxSampleRate is the niSampleRate or imSampleRate
    recorded in the same metafile.
```

```
firstSample=779283
```

All acquired samples are assigned an index number; the first acquired
during the run is sample 0. This value is the index number of the first
sample recorded in this file.

```
gateMode=0
```

Possible values are {0=Immediate, 1=TCP}.

```
nDataDirs=1
```

The number of data directories holding the data from this run. If greater
than one, then the files for an imec probe with logical index `iProbe`
are written into the data directory whose index is:
`iProbe modulo nDataDirs`.

```
nSavedChans=257
```

The number of channels being saved to disk.

```
rmt_USERTYPE=USERDEFINED
```

When SpikeGLX is set as a Gate/Trigger server it can receive remote
commands to start and stop gate and trigger intervals. It can also receive
remote commands to insert custom metadata into saved files. Such tags are
automatically prepended with 'rmt_' to distinguish them from built-in tags.

```
snsSaveChanSubset=2,4,8,12:150
snsSaveChanSubset=all
```

Two examples are shown above for `snsSaveChanSubset`. If any channels
are NOT being saved the value is a printer-like list of channels that ARE
saved. If ALL are saved, the value is 'all'.

```
syncSourceIdx=0
```

Type of pulser source {0=None, 1=External, 2=NI, 3+=IM}.

```
syncSourcePeriod=1.0
```

Measured period of the shared pulse train in seconds.

```
trigMode=1
```

Possible values are {0=Immediate, 1=Timed, 2=TTL, 3=Spike, 4=TCP}.

```
typeImEnabled=1
```

Count of imec probes enabled for this run.

```
typeNiEnabled=1
```

Whether nidq stream was enabled for this run {1=yes, 0=no}.

```
typeObEnabled=1
```

Count of OneBox analog/digital recording streams enabled for this run.

```
typeThis=nidq
```

Which stream type is described in this file {nidq, imec, obx}.

```
userNotes=Line1\nLine2...
```

Blank lines in your text are each replaced with 'backslash-n'. The `userNotes`
tag is thereby a single line in metafiles.

```
~snsShankMap=(1,2,96)(0:0:0:1)(0:1:0:1)(0:0:1:1)(0:1:1:1)(0:0:2:1)(0:1:2:1)(0:0:3:1)(0:1:3:1)(0:0:4:1)(0:1:4:1)(0:0:5:1)(0:1:5:1)(0:0:6:1)(0:1:6:1)(0:0:7:1)(0:1:7:1)(0:0:8:1)(0:1:8:1)(0:0:9:1)(0:1:9:1)(0:0:10:1)(0:1:10:1)(0:0:11:1)(0:1:11:1)(0:0:12:1)(0:1:12:1)(0:0:13:1)(0:1:13:1)(0:0:14:1)(0:1:14:1)(0:0:15:1)(0:1:15:1)(0:0:16:1)(0:1:16:1)(0:0:17:1)(0:1:17:1)(0:0:18:1)(0:1:18:1)(0:0:19:1)(0:1:19:1)(0:0:20:1)(0:1:20:1)(0:0:21:1)(0:1:21:1)(0:0:22:1)(0:1:22:1)(0:0:23:1)(0:1:23:1)(0:0:24:1)(0:1:24:1)(0:0:25:1)(0:1:25:1)(0:0:26:1)(0:1:26:1)(0:0:27:1)(0:1:27:1)(0:0:28:1)(0:1:28:1)(0:0:29:1)(0:1:29:1)(0:0:30:1)(0:1:30:1)(0:0:31:1)(0:1:31:1)(0:0:32:1)(0:1:32:1)(0:0:33:1)(0:1:33:1)(0:0:34:1)(0:1:34:1)(0:0:35:1)(0:1:35:1)(0:0:36:1)(0:1:36:1)(0:0:37:1)(0:1:37:1)(0:0:38:1)(0:1:38:1)(0:0:39:1)(0:1:39:1)(0:0:40:1)(0:1:40:1)(0:0:41:1)(0:1:41:1)(0:0:42:1)(0:1:42:1)(0:0:43:1)(0:1:43:1)(0:0:44:1)(0:1:44:1)(0:0:45:1)(0:1:45:1)(0:0:46:1)(0:1:46:1)(0:0:47:1)(0:1:47:1)(0:0:48:1)(0:1:48:1)(0:0:49:1)(0:1:49:1)(0:0:50:1)(0:1:50:1)(0:0:51:1)(0:1:51:1)(0:0:52:1)(0:1:52:1)(0:0:53:1)(0:1:53:1)(0:0:54:1)(0:1:54:1)(0:0:55:1)(0:1:55:1)(0:0:56:1)(0:1:56:1)(0:0:57:1)(0:1:57:1)(0:0:58:1)(0:1:58:1)(0:0:59:1)(0:1:59:1)(0:0:60:1)(0:1:60:1)(0:0:61:1)(0:1:61:1)(0:0:62:1)(0:1:62:1)(0:0:63:1)(0:1:63:1)(0:0:64:1)(0:1:64:1)(0:0:65:1)(0:1:65:1)(0:0:66:1)(0:1:66:1)(0:0:67:1)(0:1:67:1)(0:0:68:1)(0:1:68:1)(0:0:69:1)(0:1:69:1)(0:0:70:1)(0:1:70:1)(0:0:71:1)(0:1:71:1)(0:0:72:1)(0:1:72:1)(0:0:73:1)(0:1:73:1)(0:0:74:1)(0:1:74:1)(0:0:75:1)(0:1:75:1)(0:0:76:1)(0:1:76:1)(0:0:77:1)(0:1:77:1)(0:0:78:1)(0:1:78:1)(0:0:79:1)(0:1:79:1)(0:0:80:1)(0:1:80:1)(0:0:81:1)(0:1:81:1)(0:0:82:1)(0:1:82:1)(0:0:83:1)(0:1:83:1)(0:0:84:1)(0:1:84:1)(0:0:85:1)(0:1:85:1)(0:0:86:1)(0:1:86:1)(0:0:87:1)(0:1:87:1)(0:0:88:1)(0:1:88:1)(0:0:89:1)(0:1:89:1)(0:0:90:1)(0:1:90:1)(0:0:91:1)(0:1:91:1)(0:0:92:1)(0:1:92:1)(0:0:93:1)(0:1:93:1)(0:0:94:1)(0:1:94:1)(0:0:95:1)(0:1:95:1)
```

The ShankMap describes how electrodes are arranged on the probe. The first
() entry is a header. Here, (1,2,96) indicates the probe has up to 1 shank
with up to 2 columns and 96 rows. Note that these are maximum values that
define a grid, but not all column and row combinations need be occupied.
Each following electrode entry has four values (s:c:r:u):

1. zero-based shank # (with tips pointing down, shank-0 is left-most),

2. zero-based col #,

3. zero-based row #,

4. 0/1 "used," or, (u-flag), indicating if the electrode should be drawn
in the FileViewer and ShankViewer windows, and if it should be included in
spatial average \<S\> calculations.

>Note: There are electrode entries only for saved channels.

>Note: As of SpikeGLX 20230202, ~snsShankMap became obsolete in imec probe
metadata. Rather, probe layouts are described more accurately by the newer
`~snsGeomMap` data structure.

--------

## If Using Timed Trigger

```
trgTimIsHInf=false
```

Is the duration of the high cycle infinite (latched high)? If not `trgTimTH`
sets the duration.

```
trgTimIsNInf=false
```

Is the count of high cycles infinite? If not, `trgTimNH` sets the count.

>Note that infinite cycle counts or durations are terminated when either
the current gate goes low or the run is stopped manually.

```
trgTimNH=1
```

This is the number of high-low cycles per gate window, unless overridden by
`trgTimIsNInf`.

```
trgTimTH=1.0
```

This is the number of seconds of data to write, unless overridden by
`trgTimIsHInf`.

```
trgTimTL=1.0
```

This is the number of seconds to wait between write-phases.

```
trgTimTL0=10.0
```

This is the number of seconds to wait from the start of a gate window,
until starting the first high-phase (write-phase).

--------

## If Using TTL Trigger

```
trgTTLAIChan=192
```

```
trgTTLBit=0
```

```
trgTTLInarow=5
```

This is the count in consecutive samples that must also be high to confirm
that a rising edge is real rather than noise. This is sometimes referred to
as an "anti-bounce" feature.

```
trgTTLIsAnalog=true
```

If true, `trgTTLAIChan` specifies which analog channel in the stream will
be tested for rising edges. If false, `trgTTLBit` specifies which bit of
the 16-bit digital data words will be tested. The digital data words are
the last words in each timepoint. The bits within are arranged like this:

[analog0][analog1]...[analogN][bit15..bit0][bit31..bit16]...[digitalN].

```
trgTTLIsNInf=true
```

Is the count of high cycles infinite? If not, `trgTTLNH` sets the count.

>Note that infinite cycle counts or durations are terminated when either
the current gate goes low or the run is stopped manually.

```
trgTTLMarginS=1.0
```

This is the number of seconds to add both before and after the peri-event
interval to provide expanded context.

```
trgTTLMode=0
```

Once a rising edge is detected, the mode controls how the length of the
high-phase (file-writing phase) is determined.

Possible values: {0=Latch high, 1=Timed high, 2=Follow TTL}.

```
trgTTLNH=10
```

This is the count of writing cycles to execute per gate window, unless
overridden by `trgTTLIsNInf`.

```
trgTTLRefractS=0.5
```

This is the minimum number of seconds to wait since the last rising-edge
until the rising-edge detector may be re-armed.

```
trgTTLStream=nidq
```

```
trgTTLTH=0.5
```

This is the programmed high duration if `trgTTLMode=1`.

```
trgTTLThresh=2.0
```

This is the voltage threshold used for testing analog-type channels.

--------

## If Using Spike Trigger

```
trgSpikeAIChan=4
```

```
trgSpikeInarow=3
```

This is the count in consecutive samples that must also be low to confirm
that a falling edge is a real spike rather than noise.

```
trgSpikeIsNInf=false
```

Is the count of spikes to detect infinite? If not, `trgSpikeNS` sets
the count.

>Note that infinite spike counts are terminated when either the current
gate goes low or the run is stopped manually.

```
trgSpikeNS=10
```

Maximum number of spikes to detect (files to write) per gate window.

```
trgSpikePeriEvtS=1.0
```

This is the number of seconds to add both before and after the peri-event
interval to provide expanded context.

```
trgSpikeRefractS=0.5
```

This is the minimum number of seconds to wait since the last spike until the
falling-edge detector may be re-armed.

```
trgSpikeStream=nidq
```

```
trgSpikeThresh=-100e-6
```

This trigger defines a spike as a negative-going threshold crossing.

--------

## Nidq

```
acqMnMaXaDw=192,64,0,1
```

This is the count of channels, of each type, in each timepoint,
at acquisition time.

```
niAiRangeMax=2.5
```

Convert from 16-bit analog values (i) to voltages (V) as follows:

V = i * Vmax / Imax / gain.

For nidq data:

* Imax = 32768
* Vmax = `niAiRangeMax`
* gain = `niMNGain` or `niMAGain`, accordingly.

```
niAiRangeMin=-2.5
```

```
niAiTermination=Default
```

```
niClockSource=Whisper : 25000
```

Name of the device generating the sample clock and its programmed (set) rate.

```
niClockLine1=PFI2
```

```
niDev1=Dev1
```

```
niDev1ProductName=FakeDAQ
```

```
niMAChans1=6:7
```

```
niMAGain=1.0
```

```
niMaxInt=32768
```

Maximum amplitude integer encoded in the 16-bit analog channels.
Really, in this example [-32768..32767]. The reason for this apparent
asymmetry is that, by convention, zero is grouped with the positive
values. The stream is 16-bit so can encode 2^16 = 65536 values.
There are 32768 negative values: [-32768..-1] and 32768 positives: [0..32767].
This convention (zero is a positive number) applies in all signed
computer arithmetic.

```
niMNChans1=0:5
```

```
niMNGain=200.0
```

```
niMuxFactor=32
```

```
niSampRate=25000
```

This is the best estimator of the sample rate. It will be the calibrated
rate if calibration was done.

```
niStartEnable=true
```

```
niStartLine=Dev1/port0/line0
```

```
niXAChans1=
```

```
niXDBytes1=1
```

This is the number of bytes needed to hold the lines specified by
niXDChans1. The lines acquired from the second device (if used)
start at offset: 8 * niXDBytes1.

```
niXDChans1=1
```

This is a printer-like list of NI-DAQ line indices. For example, if your
NI device was named 'Fred' and if niXDChans1=2:3, we would acquire from
lines {Fred/line2, Fred/line3}.

```
snsMnMaXaDw=192,64,0,1
```

This is the count of channels, of each type, in each timepoint,
as stored in the binary file.

```
syncNiChan=0
```

The bit or channel number (in the acquired stream).

```
syncNiChanType=0
```

Values are {0=Digital bit, 1=Analog channel}.

```
syncNiThresh=3.0
```

The crossing threshold (V) for an analog channel.

```
~snsChanMap=(6,2,32,0,1)(MN0C0;0:0)(MN0C1;1:1)(MN0C2;2:2)(MN0C3;3:3)(MN0C4;4:4)(MN0C5;5:5)(MN0C6;6:6)(MN0C7;7:7)(MN0C8;8:8)(MN0C9;9:9)(MN0C10;10:10)(MN0C11;11:11)(MN0C12;12:12)(MN0C13;13:13)(MN0C14;14:14)(MN0C15;15:15)(MN0C16;16:16)(MN0C17;17:17)(MN0C18;18:18)(MN0C19;19:19)(MN0C20;20:20)(MN0C21;21:21)(MN0C22;22:22)(MN0C23;23:23)(MN0C24;24:24)(MN0C25;25:25)(MN0C26;26:26)(MN0C27;27:27)(MN0C28;28:28)(MN0C29;29:29)(MN0C30;30:30)(MN0C31;31:31)(MN1C0;32:32)(MN1C1;33:33)(MN1C2;34:34)(MN1C3;35:35)(MN1C4;36:36)(MN1C5;37:37)(MN1C6;38:38)(MN1C7;39:39)(MN1C8;40:40)(MN1C9;41:41)(MN1C10;42:42)(MN1C11;43:43)(MN1C12;44:44)(MN1C13;45:45)(MN1C14;46:46)(MN1C15;47:47)(MN1C16;48:48)(MN1C17;49:49)(MN1C18;50:50)(MN1C19;51:51)(MN1C20;52:52)(MN1C21;53:53)(MN1C22;54:54)(MN1C23;55:55)(MN1C24;56:56)(MN1C25;57:57)(MN1C26;58:58)(MN1C27;59:59)(MN1C28;60:60)(MN1C29;61:61)(MN1C30;62:62)(MN1C31;63:63)(MN2C0;64:64)(MN2C1;65:65)(MN2C2;66:66)(MN2C3;67:67)(MN2C4;68:68)(MN2C5;69:69)(MN2C6;70:70)(MN2C7;71:71)(MN2C8;72:72)(MN2C9;73:73)(MN2C10;74:74)(MN2C11;75:75)(MN2C12;76:76)(MN2C13;77:77)(MN2C14;78:78)(MN2C15;79:79)(MN2C16;80:80)(MN2C17;81:81)(MN2C18;82:82)(MN2C19;83:83)(MN2C20;84:84)(MN2C21;85:85)(MN2C22;86:86)(MN2C23;87:87)(MN2C24;88:88)(MN2C25;89:89)(MN2C26;90:90)(MN2C27;91:91)(MN2C28;92:92)(MN2C29;93:93)(MN2C30;94:94)(MN2C31;95:95)(MN3C0;96:96)(MN3C1;97:97)(MN3C2;98:98)(MN3C3;99:99)(MN3C4;100:100)(MN3C5;101:101)(MN3C6;102:102)(MN3C7;103:103)(MN3C8;104:104)(MN3C9;105:105)(MN3C10;106:106)(MN3C11;107:107)(MN3C12;108:108)(MN3C13;109:109)(MN3C14;110:110)(MN3C15;111:111)(MN3C16;112:112)(MN3C17;113:113)(MN3C18;114:114)(MN3C19;115:115)(MN3C20;116:116)(MN3C21;117:117)(MN3C22;118:118)(MN3C23;119:119)(MN3C24;120:120)(MN3C25;121:121)(MN3C26;122:122)(MN3C27;123:123)(MN3C28;124:124)(MN3C29;125:125)(MN3C30;126:126)(MN3C31;127:127)(MN4C0;128:128)(MN4C1;129:129)(MN4C2;130:130)(MN4C3;131:131)(MN4C4;132:132)(MN4C5;133:133)(MN4C6;134:134)(MN4C7;135:135)(MN4C8;136:136)(MN4C9;137:137)(MN4C10;138:138)(MN4C11;139:139)(MN4C12;140:140)(MN4C13;141:141)(MN4C14;142:142)(MN4C15;143:143)(MN4C16;144:144)(MN4C17;145:145)(MN4C18;146:146)(MN4C19;147:147)(MN4C20;148:148)(MN4C21;149:149)(MN4C22;150:150)(MN4C23;151:151)(MN4C24;152:152)(MN4C25;153:153)(MN4C26;154:154)(MN4C27;155:155)(MN4C28;156:156)(MN4C29;157:157)(MN4C30;158:158)(MN4C31;159:159)(MN5C0;160:160)(MN5C1;161:161)(MN5C2;162:162)(MN5C3;163:163)(MN5C4;164:164)(MN5C5;165:165)(MN5C6;166:166)(MN5C7;167:167)(MN5C8;168:168)(MN5C9;169:169)(MN5C10;170:170)(MN5C11;171:171)(MN5C12;172:172)(MN5C13;173:173)(MN5C14;174:174)(MN5C15;175:175)(MN5C16;176:176)(MN5C17;177:177)(MN5C18;178:178)(MN5C19;179:179)(MN5C20;180:180)(MN5C21;181:181)(MN5C22;182:182)(MN5C23;183:183)(MN5C24;184:184)(MN5C25;185:185)(MN5C26;186:186)(MN5C27;187:187)(MN5C28;188:188)(MN5C29;189:189)(MN5C30;190:190)(MN5C31;191:191)(MA0C0;192:192)(MA0C1;193:193)(MA0C2;194:194)(MA0C3;195:195)(MA0C4;196:196)(MA0C5;197:197)(MA0C6;198:198)(MA0C7;199:199)(MA0C8;200:200)(MA0C9;201:201)(MA0C10;202:202)(MA0C11;203:203)(MA0C12;204:204)(MA0C13;205:205)(MA0C14;206:206)(MA0C15;207:207)(MA0C16;208:208)(MA0C17;209:209)(MA0C18;210:210)(MA0C19;211:211)(MA0C20;212:212)(MA0C21;213:213)(MA0C22;214:214)(MA0C23;215:215)(MA0C24;216:216)(MA0C25;217:217)(MA0C26;218:218)(MA0C27;219:219)(MA0C28;220:220)(MA0C29;221:221)(MA0C30;222:222)(MA0C31;223:223)(MA1C0;224:224)(MA1C1;225:225)(MA1C2;226:226)(MA1C3;227:227)(MA1C4;228:228)(MA1C5;229:229)(MA1C6;230:230)(MA1C7;231:231)(MA1C8;232:232)(MA1C9;233:233)(MA1C10;234:234)(MA1C11;235:235)(MA1C12;236:236)(MA1C13;237:237)(MA1C14;238:238)(MA1C15;239:239)(MA1C16;240:240)(MA1C17;241:241)(MA1C18;242:242)(MA1C19;243:243)(MA1C20;244:244)(MA1C21;245:245)(MA1C22;246:246)(MA1C23;247:247)(MA1C24;248:248)(MA1C25;249:249)(MA1C26;250:250)(MA1C27;251:251)(MA1C28;252:252)(MA1C29;253:253)(MA1C30;254:254)(MA1C31;255:255)(XD0;256:256)
```

The channel map describes the order of graphs in SpikeGLX displays. The
header for the nidq stream, here (6,2,32,0,1), indicates there are:

* 6 MN-type NI-DAQ input channels,
* 2 MA-type NI-DAQ input channels,
* 32 multiplexed logical channels per MN/MA input,
* 0 XA NI-DAQ channels,
* 1 XD NI-DAQ digital word.

Each subsequent entry in the map has two fields, (:)-separated:

* Channel name, e.g., 'MN0C2;2'
* Zero-based order index.

>Note: There are map entries only for saved channels.

--------

### If Using 2nd Nidq Card

```
niClockLine2
```

```
niDev2
```

```
niDev2ProductName
```

```
niDualDevMode=true
```

```
niMAChans2
```

```
niMNChans2
```

```
niXAChans2
```

```
niXDBytes2
```

```
niXDChans2
```

--------

## Imec

```
acqApLfSy=384,384,1
```

This is the count of channels, of each type, in each timepoint,
at acquisition time.

```
imAiRangeMax=0.6
```

Convert from 16-bit analog values (i) to voltages (V) as follows:

V = i * Vmax / Imax / gain.

* Imax = `imMaxInt`
* Vmax = `imAiRangeMax`

For type 21 or type 24 imec probes:

* gain = 80 (fixed).

For type 0 or other probes with selectable gain:

* gain = imroTbl gain entry for AP or LF type.

```
imAiRangeMin=-0.6
```

```
imAnyChanFullBand=false
```

This is true if the probe has no LF band, or if this a dual-band probe
with at least one channel set to full-band in the imro table.

```
imCalibrated=true
```

Imec ADC and/or gain calibration files were applied to the probe.

```
imChan0apGain=100
```

The AP-band gain applied to channel 0. If you used the graphical imro editor
to create your imro file then all channels will have this AP gain.

```
imChan0lfGain=100
```

The LF-band gain applied to channel 0. If you used the graphical imro editor
to create your imro file then all channels will have this LF gain.

```
imChan0Ref=ext
```

The type of referencing applied to channel 0. If you used the graphical
imro editor to create your imro file then all channels will have this type
of referencing.

```
imColsPerShank=2
```

Number of physical electrode columns on each shank row.

```
imDatApi=3.31.0
```

This is the Imec API version number: major.minor.build.

```
imDatBs_fw=2.0.137
```

This is the BS firmware version number: major.minor.build.

```
imDatBsc_fw=3.2.176
```

This is the BSC firmware version number: major.minor.build.

```
imDatBsc_hw=2.1
```

This is the BSC hardware version number: major.minor.

```
imDatBsc_pn=NP2_QBSC_00
```

This is the BSC part number.

```
imDatBsc_sn=175
```

This is the PXI BSC serial number or OneBox physical ID number.

```
imDatFx_hw=1.7
```

This is the Flex hardware version number: major.minor.

```
imDatFx_pn=NPM_FLEX_0
```

This is the Flex part number.

```
imDatFx_sn=0
```

This is the Flex serial number.

```
imDatHs_hw=1.0
```

This is the HS hardware version number: major.minor.

```
imDatHs_pn=NPM_HS_01
```

This is the HS part number.

```
imDatHs_sn=1440
```

This is the HS serial number.

```
imDatPrb_dock=1
```

This is the probe dock number.

```
imDatPrb_pn=PRB2_1_2_0640_0
```

This is the probe part number.

```
imDatPrb_port=1
```

This is the probe port number.

```
imDatPrb_slot=2
```

This is the probe slot number.

```
imDatPrb_sn=19011116444
```

This is the probe serial number.

```
imDatPrb_type=21
```

This is the probe type {0=NP1.0, 21=NP2.0(1-shank), 24=NP2.0(4-shank)}.

```
imErrFlags0_IS_CT_SR_LK_PP_SY_MS=0 0 0 0 0 0 0
```

For each imec stream we monitor the cumulative count of several error flags,
hence, imErrFlags5 denotes the flags for probe steam 5. Note that quad-base
probes (part number NP2020/2021) record flags for each shank, hence,
imErrFlags3_2 denotes that stream 3 is a quad-probe and the flags are from
shank 2.

The flags are labeled {COUNT, SERDES, LOCK, POP, SYNC, MISS}. The metadata
field 'IS' = 1 if any error occurred, 0 otherwise. I.e., "is an error."

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

```
imLEDEnable=false
```

```
imLowLatency=false
```

This mode runs the probe sample fetching loop faster, which reduces the
time to get fresh data. However, this drives the CPU 50%+ harder and
reduces the maximum number of probes you can safely run concurrently.

```
imIsSvyRun=false
```

```
imMaxInt=512
```

Maximum amplitude integer encoded in the 16-bit analog channels.
Really, in this example [-512..511]. The reason for this apparent
asymmetry is that, by convention, zero is grouped with the positive
values. The example probe is 10-bit, so encodes 2^10 = 1024 values.
There are 512 negative values: [-512..-1] and 512 positives: [0..511].
This convention (zero is a positive number) applies in all signed
computer arithmetic.

```
imRoFile=
```

This is a path to your custom Imec Readout Table (imRo) file. If you
elect default settings no file is needed. The active table content is
stored as tag `~imroTbl` whether custom or default.

```
imRowsPerShank=480
```

Number of electrode rows on each shank.

```
imSampRate=30000
```

This is the best estimator of the sample rate. It will be the calibrated
rate if calibration was done.

```
imStdby=0:12,45
```

These channels had been placed in stand-by mode, which means their analog
amplifiers were switched off. Stand-by channels are still read from the
hardware and stored in the data stream. The only reason to set stand-by
mode is to reduce noise/crosstalk in the system (may not be needed in
commercial system).

```
imSvyMaxBnk=2
```

The highest bank index to survey.

```
imSvyNShanks=4
```

The count of shanks to survey.

```
imTipLength=206
```

Number of microns from probe tip to center of lowest electrodes.

```
imTrgRising=true
```

Selects whether external trigger detects a rising or falling edge.

```
imTrgSource=0
```

Selects the type of trigger that starts the run: {0=software}.

```
imX0EvenRow=27
```

Number of microns from left edge of shank to left-most electrode center
on an even numbered row.

```
imX0OddRow=27
```

Number of microns from left edge of shank to left-most electrode center
on an odd numbered row.

```
imXPitch=32
```

Electrode column-to-column stride in microns.

```
imZPitch=15
```

Electrode row-to-row stride in microns.

```
snsApLfSy=384,0,1
```

This is the count of channels, of each type, in each timepoint,
as stored in the binary file.

```
syncImInputSlot=2
```

The Imec slot getting a sync signal input. You should connect sync signals
to only one slot (one SMA). Note that the signal is recorded in the data
for each probe as bit 6 of that stream's aux (SY) word.

```
~anatomy_shank0=(206,306,100,120,40,Thalamus)(306,406,20,60,180,Cortex)
```

Anatomy string received from Pinpoint software, listing brain regions
pierced by given shank (with tips pointing down, shank-0 is left-most).

Each (element) specifies:
- region-start-microns-from-tip
- region-end-microns-from-tip
- RGB: red component
- RGB: blue component
- RGB: green component
- region-label

```
~imroTbl=(0,384)(0 0 0 500 250 1)(1 0 0 500 250 1)(2 0 0 500 250 1)(3 0 0 500 250 1)(4 0 0 500 250 1)(5 0 0 500 250 1)(6 0 0 500 250 1)(7 0 0 500 250 1)(8 0 0 500 250 1)(9 0 0 500 250 1)(10 0 0 500 250 1)(11 0 0 500 250 1)(12 0 0 500 250 1)(13 0 0 500 250 1)(14 0 0 500 250 1)(15 0 0 500 250 1)(16 0 0 500 250 1)(17 0 0 500 250 1)(18 0 0 500 250 1)(19 0 0 500 250 1)(20 0 0 500 250 1)(21 0 0 500 250 1)(22 0 0 500 250 1)(23 0 0 500 250 1)(24 0 0 500 250 1)(25 0 0 500 250 1)(26 0 0 500 250 1)(27 0 0 500 250 1)(28 0 0 500 250 1)(29 0 0 500 250 1)(30 0 0 500 250 1)(31 0 0 500 250 1)(32 0 0 500 250 1)(33 0 0 500 250 1)(34 0 0 500 250 1)(35 0 0 500 250 1)(36 0 0 500 250 1)(37 0 0 500 250 1)(38 0 0 500 250 1)(39 0 0 500 250 1)(40 0 0 500 250 1)(41 0 0 500 250 1)(42 0 0 500 250 1)(43 0 0 500 250 1)(44 0 0 500 250 1)(45 0 0 500 250 1)(46 0 0 500 250 1)(47 0 0 500 250 1)(48 0 0 500 250 1)(49 0 0 500 250 1)(50 0 0 500 250 1)(51 0 0 500 250 1)(52 0 0 500 250 1)(53 0 0 500 250 1)(54 0 0 500 250 1)(55 0 0 500 250 1)(56 0 0 500 250 1)(57 0 0 500 250 1)(58 0 0 500 250 1)(59 0 0 500 250 1)(60 0 0 500 250 1)(61 0 0 500 250 1)(62 0 0 500 250 1)(63 0 0 500 250 1)(64 0 0 500 250 1)(65 0 0 500 250 1)(66 0 0 500 250 1)(67 0 0 500 250 1)(68 0 0 500 250 1)(69 0 0 500 250 1)(70 0 0 500 250 1)(71 0 0 500 250 1)(72 0 0 500 250 1)(73 0 0 500 250 1)(74 0 0 500 250 1)(75 0 0 500 250 1)(76 0 0 500 250 1)(77 0 0 500 250 1)(78 0 0 500 250 1)(79 0 0 500 250 1)(80 0 0 500 250 1)(81 0 0 500 250 1)(82 0 0 500 250 1)(83 0 0 500 250 1)(84 0 0 500 250 1)(85 0 0 500 250 1)(86 0 0 500 250 1)(87 0 0 500 250 1)(88 0 0 500 250 1)(89 0 0 500 250 1)(90 0 0 500 250 1)(91 0 0 500 250 1)(92 0 0 500 250 1)(93 0 0 500 250 1)(94 0 0 500 250 1)(95 0 0 500 250 1)(96 0 0 500 250 1)(97 0 0 500 250 1)(98 0 0 500 250 1)(99 0 0 500 250 1)(100 0 0 500 250 1)(101 0 0 500 250 1)(102 0 0 500 250 1)(103 0 0 500 250 1)(104 0 0 500 250 1)(105 0 0 500 250 1)(106 0 0 500 250 1)(107 0 0 500 250 1)(108 0 0 500 250 1)(109 0 0 500 250 1)(110 0 0 500 250 1)(111 0 0 500 250 1)(112 0 0 500 250 1)(113 0 0 500 250 1)(114 0 0 500 250 1)(115 0 0 500 250 1)(116 0 0 500 250 1)(117 0 0 500 250 1)(118 0 0 500 250 1)(119 0 0 500 250 1)(120 0 0 500 250 1)(121 0 0 500 250 1)(122 0 0 500 250 1)(123 0 0 500 250 1)(124 0 0 500 250 1)(125 0 0 500 250 1)(126 0 0 500 250 1)(127 0 0 500 250 1)(128 0 0 500 250 1)(129 0 0 500 250 1)(130 0 0 500 250 1)(131 0 0 500 250 1)(132 0 0 500 250 1)(133 0 0 500 250 1)(134 0 0 500 250 1)(135 0 0 500 250 1)(136 0 0 500 250 1)(137 0 0 500 250 1)(138 0 0 500 250 1)(139 0 0 500 250 1)(140 0 0 500 250 1)(141 0 0 500 250 1)(142 0 0 500 250 1)(143 0 0 500 250 1)(144 0 0 500 250 1)(145 0 0 500 250 1)(146 0 0 500 250 1)(147 0 0 500 250 1)(148 0 0 500 250 1)(149 0 0 500 250 1)(150 0 0 500 250 1)(151 0 0 500 250 1)(152 0 0 500 250 1)(153 0 0 500 250 1)(154 0 0 500 250 1)(155 0 0 500 250 1)(156 0 0 500 250 1)(157 0 0 500 250 1)(158 0 0 500 250 1)(159 0 0 500 250 1)(160 0 0 500 250 1)(161 0 0 500 250 1)(162 0 0 500 250 1)(163 0 0 500 250 1)(164 0 0 500 250 1)(165 0 0 500 250 1)(166 0 0 500 250 1)(167 0 0 500 250 1)(168 0 0 500 250 1)(169 0 0 500 250 1)(170 0 0 500 250 1)(171 0 0 500 250 1)(172 0 0 500 250 1)(173 0 0 500 250 1)(174 0 0 500 250 1)(175 0 0 500 250 1)(176 0 0 500 250 1)(177 0 0 500 250 1)(178 0 0 500 250 1)(179 0 0 500 250 1)(180 0 0 500 250 1)(181 0 0 500 250 1)(182 0 0 500 250 1)(183 0 0 500 250 1)(184 0 0 500 250 1)(185 0 0 500 250 1)(186 0 0 500 250 1)(187 0 0 500 250 1)(188 0 0 500 250 1)(189 0 0 500 250 1)(190 0 0 500 250 1)(191 0 0 500 250 1)(192 0 0 500 250 1)(193 0 0 500 250 1)(194 0 0 500 250 1)(195 0 0 500 250 1)(196 0 0 500 250 1)(197 0 0 500 250 1)(198 0 0 500 250 1)(199 0 0 500 250 1)(200 0 0 500 250 1)(201 0 0 500 250 1)(202 0 0 500 250 1)(203 0 0 500 250 1)(204 0 0 500 250 1)(205 0 0 500 250 1)(206 0 0 500 250 1)(207 0 0 500 250 1)(208 0 0 500 250 1)(209 0 0 500 250 1)(210 0 0 500 250 1)(211 0 0 500 250 1)(212 0 0 500 250 1)(213 0 0 500 250 1)(214 0 0 500 250 1)(215 0 0 500 250 1)(216 0 0 500 250 1)(217 0 0 500 250 1)(218 0 0 500 250 1)(219 0 0 500 250 1)(220 0 0 500 250 1)(221 0 0 500 250 1)(222 0 0 500 250 1)(223 0 0 500 250 1)(224 0 0 500 250 1)(225 0 0 500 250 1)(226 0 0 500 250 1)(227 0 0 500 250 1)(228 0 0 500 250 1)(229 0 0 500 250 1)(230 0 0 500 250 1)(231 0 0 500 250 1)(232 0 0 500 250 1)(233 0 0 500 250 1)(234 0 0 500 250 1)(235 0 0 500 250 1)(236 0 0 500 250 1)(237 0 0 500 250 1)(238 0 0 500 250 1)(239 0 0 500 250 1)(240 0 0 500 250 1)(241 0 0 500 250 1)(242 0 0 500 250 1)(243 0 0 500 250 1)(244 0 0 500 250 1)(245 0 0 500 250 1)(246 0 0 500 250 1)(247 0 0 500 250 1)(248 0 0 500 250 1)(249 0 0 500 250 1)(250 0 0 500 250 1)(251 0 0 500 250 1)(252 0 0 500 250 1)(253 0 0 500 250 1)(254 0 0 500 250 1)(255 0 0 500 250 1)(256 0 0 500 250 1)(257 0 0 500 250 1)(258 0 0 500 250 1)(259 0 0 500 250 1)(260 0 0 500 250 1)(261 0 0 500 250 1)(262 0 0 500 250 1)(263 0 0 500 250 1)(264 0 0 500 250 1)(265 0 0 500 250 1)(266 0 0 500 250 1)(267 0 0 500 250 1)(268 0 0 500 250 1)(269 0 0 500 250 1)(270 0 0 500 250 1)(271 0 0 500 250 1)(272 0 0 500 250 1)(273 0 0 500 250 1)(274 0 0 500 250 1)(275 0 0 500 250 1)(276 0 0 500 250 1)(277 0 0 500 250 1)(278 0 0 500 250 1)(279 0 0 500 250 1)(280 0 0 500 250 1)(281 0 0 500 250 1)(282 0 0 500 250 1)(283 0 0 500 250 1)(284 0 0 500 250 1)(285 0 0 500 250 1)(286 0 0 500 250 1)(287 0 0 500 250 1)(288 0 0 500 250 1)(289 0 0 500 250 1)(290 0 0 500 250 1)(291 0 0 500 250 1)(292 0 0 500 250 1)(293 0 0 500 250 1)(294 0 0 500 250 1)(295 0 0 500 250 1)(296 0 0 500 250 1)(297 0 0 500 250 1)(298 0 0 500 250 1)(299 0 0 500 250 1)(300 0 0 500 250 1)(301 0 0 500 250 1)(302 0 0 500 250 1)(303 0 0 500 250 1)(304 0 0 500 250 1)(305 0 0 500 250 1)(306 0 0 500 250 1)(307 0 0 500 250 1)(308 0 0 500 250 1)(309 0 0 500 250 1)(310 0 0 500 250 1)(311 0 0 500 250 1)(312 0 0 500 250 1)(313 0 0 500 250 1)(314 0 0 500 250 1)(315 0 0 500 250 1)(316 0 0 500 250 1)(317 0 0 500 250 1)(318 0 0 500 250 1)(319 0 0 500 250 1)(320 0 0 500 250 1)(321 0 0 500 250 1)(322 0 0 500 250 1)(323 0 0 500 250 1)(324 0 0 500 250 1)(325 0 0 500 250 1)(326 0 0 500 250 1)(327 0 0 500 250 1)(328 0 0 500 250 1)(329 0 0 500 250 1)(330 0 0 500 250 1)(331 0 0 500 250 1)(332 0 0 500 250 1)(333 0 0 500 250 1)(334 0 0 500 250 1)(335 0 0 500 250 1)(336 0 0 500 250 1)(337 0 0 500 250 1)(338 0 0 500 250 1)(339 0 0 500 250 1)(340 0 0 500 250 1)(341 0 0 500 250 1)(342 0 0 500 250 1)(343 0 0 500 250 1)(344 0 0 500 250 1)(345 0 0 500 250 1)(346 0 0 500 250 1)(347 0 0 500 250 1)(348 0 0 500 250 1)(349 0 0 500 250 1)(350 0 0 500 250 1)(351 0 0 500 250 1)(352 0 0 500 250 1)(353 0 0 500 250 1)(354 0 0 500 250 1)(355 0 0 500 250 1)(356 0 0 500 250 1)(357 0 0 500 250 1)(358 0 0 500 250 1)(359 0 0 500 250 1)(360 0 0 500 250 1)(361 0 0 500 250 1)(362 0 0 500 250 1)(363 0 0 500 250 1)(364 0 0 500 250 1)(365 0 0 500 250 1)(366 0 0 500 250 1)(367 0 0 500 250 1)(368 0 0 500 250 1)(369 0 0 500 250 1)(370 0 0 500 250 1)(371 0 0 500 250 1)(372 0 0 500 250 1)(373 0 0 500 250 1)(374 0 0 500 250 1)(375 0 0 500 250 1)(376 0 0 500 250 1)(377 0 0 500 250 1)(378 0 0 500 250 1)(379 0 0 500 250 1)(380 0 0 500 250 1)(381 0 0 500 250 1)(382 0 0 500 250 1)(383 0 0 500 250 1)
~imroTbl=(21,384)(0 1 0 0)(1 1 0 1)(2 1 0 2)(3 1 0 3)(4 1 0 4)(5 1 0 5)(6 1 0 6)(7 1 0 7)(8 1 0 8)(9 1 0 9)(10 1 0 10)(11 1 0 11)(12 1 0 12)(13 1 0 13)(14 1 0 14)(15 1 0 15)(16 1 0 16)(17 1 0 17)(18 1 0 18)(19 1 0 19)(20 1 0 20)(21 1 0 21)(22 1 0 22)(23 1 0 23)(24 1 0 24)(25 1 0 25)(26 1 0 26)(27 1 0 27)(28 1 0 28)(29 1 0 29)(30 1 0 30)(31 1 0 31)(32 1 0 32)(33 1 0 33)(34 1 0 34)(35 1 0 35)(36 1 0 36)(37 1 0 37)(38 1 0 38)(39 1 0 39)(40 1 0 40)(41 1 0 41)(42 1 0 42)(43 1 0 43)(44 1 0 44)(45 1 0 45)(46 1 0 46)(47 1 0 47)(48 1 0 48)(49 1 0 49)(50 1 0 50)(51 1 0 51)(52 1 0 52)(53 1 0 53)(54 1 0 54)(55 1 0 55)(56 1 0 56)(57 1 0 57)(58 1 0 58)(59 1 0 59)(60 1 0 60)(61 1 0 61)(62 1 0 62)(63 1 0 63)(64 1 0 64)(65 1 0 65)(66 1 0 66)(67 1 0 67)(68 1 0 68)(69 1 0 69)(70 1 0 70)(71 1 0 71)(72 1 0 72)(73 1 0 73)(74 1 0 74)(75 1 0 75)(76 1 0 76)(77 1 0 77)(78 1 0 78)(79 1 0 79)(80 1 0 80)(81 1 0 81)(82 1 0 82)(83 1 0 83)(84 1 0 84)(85 1 0 85)(86 1 0 86)(87 1 0 87)(88 1 0 88)(89 1 0 89)(90 1 0 90)(91 1 0 91)(92 1 0 92)(93 1 0 93)(94 1 0 94)(95 1 0 95)(96 1 0 96)(97 1 0 97)(98 1 0 98)(99 1 0 99)(100 1 0 100)(101 1 0 101)(102 1 0 102)(103 1 0 103)(104 1 0 104)(105 1 0 105)(106 1 0 106)(107 1 0 107)(108 1 0 108)(109 1 0 109)(110 1 0 110)(111 1 0 111)(112 1 0 112)(113 1 0 113)(114 1 0 114)(115 1 0 115)(116 1 0 116)(117 1 0 117)(118 1 0 118)(119 1 0 119)(120 1 0 120)(121 1 0 121)(122 1 0 122)(123 1 0 123)(124 1 0 124)(125 1 0 125)(126 1 0 126)(127 1 0 127)(128 1 0 128)(129 1 0 129)(130 1 0 130)(131 1 0 131)(132 1 0 132)(133 1 0 133)(134 1 0 134)(135 1 0 135)(136 1 0 136)(137 1 0 137)(138 1 0 138)(139 1 0 139)(140 1 0 140)(141 1 0 141)(142 1 0 142)(143 1 0 143)(144 1 0 144)(145 1 0 145)(146 1 0 146)(147 1 0 147)(148 1 0 148)(149 1 0 149)(150 1 0 150)(151 1 0 151)(152 1 0 152)(153 1 0 153)(154 1 0 154)(155 1 0 155)(156 1 0 156)(157 1 0 157)(158 1 0 158)(159 1 0 159)(160 1 0 160)(161 1 0 161)(162 1 0 162)(163 1 0 163)(164 1 0 164)(165 1 0 165)(166 1 0 166)(167 1 0 167)(168 1 0 168)(169 1 0 169)(170 1 0 170)(171 1 0 171)(172 1 0 172)(173 1 0 173)(174 1 0 174)(175 1 0 175)(176 1 0 176)(177 1 0 177)(178 1 0 178)(179 1 0 179)(180 1 0 180)(181 1 0 181)(182 1 0 182)(183 1 0 183)(184 1 0 184)(185 1 0 185)(186 1 0 186)(187 1 0 187)(188 1 0 188)(189 1 0 189)(190 1 0 190)(191 1 0 191)(192 1 0 192)(193 1 0 193)(194 1 0 194)(195 1 0 195)(196 1 0 196)(197 1 0 197)(198 1 0 198)(199 1 0 199)(200 1 0 200)(201 1 0 201)(202 1 0 202)(203 1 0 203)(204 1 0 204)(205 1 0 205)(206 1 0 206)(207 1 0 207)(208 1 0 208)(209 1 0 209)(210 1 0 210)(211 1 0 211)(212 1 0 212)(213 1 0 213)(214 1 0 214)(215 1 0 215)(216 1 0 216)(217 1 0 217)(218 1 0 218)(219 1 0 219)(220 1 0 220)(221 1 0 221)(222 1 0 222)(223 1 0 223)(224 1 0 224)(225 1 0 225)(226 1 0 226)(227 1 0 227)(228 1 0 228)(229 1 0 229)(230 1 0 230)(231 1 0 231)(232 1 0 232)(233 1 0 233)(234 1 0 234)(235 1 0 235)(236 1 0 236)(237 1 0 237)(238 1 0 238)(239 1 0 239)(240 1 0 240)(241 1 0 241)(242 1 0 242)(243 1 0 243)(244 1 0 244)(245 1 0 245)(246 1 0 246)(247 1 0 247)(248 1 0 248)(249 1 0 249)(250 1 0 250)(251 1 0 251)(252 1 0 252)(253 1 0 253)(254 1 0 254)(255 1 0 255)(256 1 0 256)(257 1 0 257)(258 1 0 258)(259 1 0 259)(260 1 0 260)(261 1 0 261)(262 1 0 262)(263 1 0 263)(264 1 0 264)(265 1 0 265)(266 1 0 266)(267 1 0 267)(268 1 0 268)(269 1 0 269)(270 1 0 270)(271 1 0 271)(272 1 0 272)(273 1 0 273)(274 1 0 274)(275 1 0 275)(276 1 0 276)(277 1 0 277)(278 1 0 278)(279 1 0 279)(280 1 0 280)(281 1 0 281)(282 1 0 282)(283 1 0 283)(284 1 0 284)(285 1 0 285)(286 1 0 286)(287 1 0 287)(288 1 0 288)(289 1 0 289)(290 1 0 290)(291 1 0 291)(292 1 0 292)(293 1 0 293)(294 1 0 294)(295 1 0 295)(296 1 0 296)(297 1 0 297)(298 1 0 298)(299 1 0 299)(300 1 0 300)(301 1 0 301)(302 1 0 302)(303 1 0 303)(304 1 0 304)(305 1 0 305)(306 1 0 306)(307 1 0 307)(308 1 0 308)(309 1 0 309)(310 1 0 310)(311 1 0 311)(312 1 0 312)(313 1 0 313)(314 1 0 314)(315 1 0 315)(316 1 0 316)(317 1 0 317)(318 1 0 318)(319 1 0 319)(320 1 0 320)(321 1 0 321)(322 1 0 322)(323 1 0 323)(324 1 0 324)(325 1 0 325)(326 1 0 326)(327 1 0 327)(328 1 0 328)(329 1 0 329)(330 1 0 330)(331 1 0 331)(332 1 0 332)(333 1 0 333)(334 1 0 334)(335 1 0 335)(336 1 0 336)(337 1 0 337)(338 1 0 338)(339 1 0 339)(340 1 0 340)(341 1 0 341)(342 1 0 342)(343 1 0 343)(344 1 0 344)(345 1 0 345)(346 1 0 346)(347 1 0 347)(348 1 0 348)(349 1 0 349)(350 1 0 350)(351 1 0 351)(352 1 0 352)(353 1 0 353)(354 1 0 354)(355 1 0 355)(356 1 0 356)(357 1 0 357)(358 1 0 358)(359 1 0 359)(360 1 0 360)(361 1 0 361)(362 1 0 362)(363 1 0 363)(364 1 0 364)(365 1 0 365)(366 1 0 366)(367 1 0 367)(368 1 0 368)(369 1 0 369)(370 1 0 370)(371 1 0 371)(372 1 0 372)(373 1 0 373)(374 1 0 374)(375 1 0 375)(376 1 0 376)(377 1 0 377)(378 1 0 378)(379 1 0 379)(380 1 0 380)(381 1 0 381)(382 1 0 382)(383 1 0 383)
~imroTbl=(24,384)(0 0 0 1 0)(1 0 0 1 1)(2 0 0 1 2)(3 0 0 1 3)(4 0 0 1 4)(5 0 0 1 5)(6 0 0 1 6)(7 0 0 1 7)(8 0 0 1 8)(9 0 0 1 9)(10 0 0 1 10)(11 0 0 1 11)(12 0 0 1 12)(13 0 0 1 13)(14 0 0 1 14)(15 0 0 1 15)(16 0 0 1 16)(17 0 0 1 17)(18 0 0 1 18)(19 0 0 1 19)(20 0 0 1 20)(21 0 0 1 21)(22 0 0 1 22)(23 0 0 1 23)(24 0 0 1 24)(25 0 0 1 25)(26 0 0 1 26)(27 0 0 1 27)(28 0 0 1 28)(29 0 0 1 29)(30 0 0 1 30)(31 0 0 1 31)(32 0 0 1 32)(33 0 0 1 33)(34 0 0 1 34)(35 0 0 1 35)(36 0 0 1 36)(37 0 0 1 37)(38 0 0 1 38)(39 0 0 1 39)(40 0 0 1 40)(41 0 0 1 41)(42 0 0 1 42)(43 0 0 1 43)(44 0 0 1 44)(45 0 0 1 45)(46 0 0 1 46)(47 0 0 1 47)(48 0 0 1 288)(49 0 0 1 289)(50 0 0 1 290)(51 0 0 1 291)(52 0 0 1 292)(53 0 0 1 293)(54 0 0 1 294)(55 0 0 1 295)(56 0 0 1 296)(57 0 0 1 297)(58 0 0 1 298)(59 0 0 1 299)(60 0 0 1 300)(61 0 0 1 301)(62 0 0 1 302)(63 0 0 1 303)(64 0 0 1 304)(65 0 0 1 305)(66 0 0 1 306)(67 0 0 1 307)(68 0 0 1 308)(69 0 0 1 309)(70 0 0 1 310)(71 0 0 1 311)(72 0 0 1 312)(73 0 0 1 313)(74 0 0 1 314)(75 0 0 1 315)(76 0 0 1 316)(77 0 0 1 317)(78 0 0 1 318)(79 0 0 1 319)(80 0 0 1 320)(81 0 0 1 321)(82 0 0 1 322)(83 0 0 1 323)(84 0 0 1 324)(85 0 0 1 325)(86 0 0 1 326)(87 0 0 1 327)(88 0 0 1 328)(89 0 0 1 329)(90 0 0 1 330)(91 0 0 1 331)(92 0 0 1 332)(93 0 0 1 333)(94 0 0 1 334)(95 0 0 1 335)(96 0 0 1 48)(97 0 0 1 49)(98 0 0 1 50)(99 0 0 1 51)(100 0 0 1 52)(101 0 0 1 53)(102 0 0 1 54)(103 0 0 1 55)(104 0 0 1 56)(105 0 0 1 57)(106 0 0 1 58)(107 0 0 1 59)(108 0 0 1 60)(109 0 0 1 61)(110 0 0 1 62)(111 0 0 1 63)(112 0 0 1 64)(113 0 0 1 65)(114 0 0 1 66)(115 0 0 1 67)(116 0 0 1 68)(117 0 0 1 69)(118 0 0 1 70)(119 0 0 1 71)(120 0 0 1 72)(121 0 0 1 73)(122 0 0 1 74)(123 0 0 1 75)(124 0 0 1 76)(125 0 0 1 77)(126 0 0 1 78)(127 0 0 1 79)(128 0 0 1 80)(129 0 0 1 81)(130 0 0 1 82)(131 0 0 1 83)(132 0 0 1 84)(133 0 0 1 85)(134 0 0 1 86)(135 0 0 1 87)(136 0 0 1 88)(137 0 0 1 89)(138 0 0 1 90)(139 0 0 1 91)(140 0 0 1 92)(141 0 0 1 93)(142 0 0 1 94)(143 0 0 1 95)(144 0 0 1 336)(145 0 0 1 337)(146 0 0 1 338)(147 0 0 1 339)(148 0 0 1 340)(149 0 0 1 341)(150 0 0 1 342)(151 0 0 1 343)(152 0 0 1 344)(153 0 0 1 345)(154 0 0 1 346)(155 0 0 1 347)(156 0 0 1 348)(157 0 0 1 349)(158 0 0 1 350)(159 0 0 1 351)(160 0 0 1 352)(161 0 0 1 353)(162 0 0 1 354)(163 0 0 1 355)(164 0 0 1 356)(165 0 0 1 357)(166 0 0 1 358)(167 0 0 1 359)(168 0 0 1 360)(169 0 0 1 361)(170 0 0 1 362)(171 0 0 1 363)(172 0 0 1 364)(173 0 0 1 365)(174 0 0 1 366)(175 0 0 1 367)(176 0 0 1 368)(177 0 0 1 369)(178 0 0 1 370)(179 0 0 1 371)(180 0 0 1 372)(181 0 0 1 373)(182 0 0 1 374)(183 0 0 1 375)(184 0 0 1 376)(185 0 0 1 377)(186 0 0 1 378)(187 0 0 1 379)(188 0 0 1 380)(189 0 0 1 381)(190 0 0 1 382)(191 0 0 1 383)(192 0 0 1 96)(193 0 0 1 97)(194 0 0 1 98)(195 0 0 1 99)(196 0 0 1 100)(197 0 0 1 101)(198 0 0 1 102)(199 0 0 1 103)(200 0 0 1 104)(201 0 0 1 105)(202 0 0 1 106)(203 0 0 1 107)(204 0 0 1 108)(205 0 0 1 109)(206 0 0 1 110)(207 0 0 1 111)(208 0 0 1 112)(209 0 0 1 113)(210 0 0 1 114)(211 0 0 1 115)(212 0 0 1 116)(213 0 0 1 117)(214 0 0 1 118)(215 0 0 1 119)(216 0 0 1 120)(217 0 0 1 121)(218 0 0 1 122)(219 0 0 1 123)(220 0 0 1 124)(221 0 0 1 125)(222 0 0 1 126)(223 0 0 1 127)(224 0 0 1 128)(225 0 0 1 129)(226 0 0 1 130)(227 0 0 1 131)(228 0 0 1 132)(229 0 0 1 133)(230 0 0 1 134)(231 0 0 1 135)(232 0 0 1 136)(233 0 0 1 137)(234 0 0 1 138)(235 0 0 1 139)(236 0 0 1 140)(237 0 0 1 141)(238 0 0 1 142)(239 0 0 1 143)(240 0 0 1 192)(241 0 0 1 193)(242 0 0 1 194)(243 0 0 1 195)(244 0 0 1 196)(245 0 0 1 197)(246 0 0 1 198)(247 0 0 1 199)(248 0 0 1 200)(249 0 0 1 201)(250 0 0 1 202)(251 0 0 1 203)(252 0 0 1 204)(253 0 0 1 205)(254 0 0 1 206)(255 0 0 1 207)(256 0 0 1 208)(257 0 0 1 209)(258 0 0 1 210)(259 0 0 1 211)(260 0 0 1 212)(261 0 0 1 213)(262 0 0 1 214)(263 0 0 1 215)(264 0 0 1 216)(265 0 0 1 217)(266 0 0 1 218)(267 0 0 1 219)(268 0 0 1 220)(269 0 0 1 221)(270 0 0 1 222)(271 0 0 1 223)(272 0 0 1 224)(273 0 0 1 225)(274 0 0 1 226)(275 0 0 1 227)(276 0 0 1 228)(277 0 0 1 229)(278 0 0 1 230)(279 0 0 1 231)(280 0 0 1 232)(281 0 0 1 233)(282 0 0 1 234)(283 0 0 1 235)(284 0 0 1 236)(285 0 0 1 237)(286 0 0 1 238)(287 0 0 1 239)(288 0 0 1 144)(289 0 0 1 145)(290 0 0 1 146)(291 0 0 1 147)(292 0 0 1 148)(293 0 0 1 149)(294 0 0 1 150)(295 0 0 1 151)(296 0 0 1 152)(297 0 0 1 153)(298 0 0 1 154)(299 0 0 1 155)(300 0 0 1 156)(301 0 0 1 157)(302 0 0 1 158)(303 0 0 1 159)(304 0 0 1 160)(305 0 0 1 161)(306 0 0 1 162)(307 0 0 1 163)(308 0 0 1 164)(309 0 0 1 165)(310 0 0 1 166)(311 0 0 1 167)(312 0 0 1 168)(313 0 0 1 169)(314 0 0 1 170)(315 0 0 1 171)(316 0 0 1 172)(317 0 0 1 173)(318 0 0 1 174)(319 0 0 1 175)(320 0 0 1 176)(321 0 0 1 177)(322 0 0 1 178)(323 0 0 1 179)(324 0 0 1 180)(325 0 0 1 181)(326 0 0 1 182)(327 0 0 1 183)(328 0 0 1 184)(329 0 0 1 185)(330 0 0 1 186)(331 0 0 1 187)(332 0 0 1 188)(333 0 0 1 189)(334 0 0 1 190)(335 0 0 1 191)(336 0 0 1 240)(337 0 0 1 241)(338 0 0 1 242)(339 0 0 1 243)(340 0 0 1 244)(341 0 0 1 245)(342 0 0 1 246)(343 0 0 1 247)(344 0 0 1 248)(345 0 0 1 249)(346 0 0 1 250)(347 0 0 1 251)(348 0 0 1 252)(349 0 0 1 253)(350 0 0 1 254)(351 0 0 1 255)(352 0 0 1 256)(353 0 0 1 257)(354 0 0 1 258)(355 0 0 1 259)(356 0 0 1 260)(357 0 0 1 261)(358 0 0 1 262)(359 0 0 1 263)(360 0 0 1 264)(361 0 0 1 265)(362 0 0 1 266)(363 0 0 1 267)(364 0 0 1 268)(365 0 0 1 269)(366 0 0 1 270)(367 0 0 1 271)(368 0 0 1 272)(369 0 0 1 273)(370 0 0 1 274)(371 0 0 1 275)(372 0 0 1 276)(373 0 0 1 277)(374 0 0 1 278)(375 0 0 1 279)(376 0 0 1 280)(377 0 0 1 281)(378 0 0 1 282)(379 0 0 1 283)(380 0 0 1 284)(381 0 0 1 285)(382 0 0 1 286)(383 0 0 1 287)
~imroTbl=(2020,1536)(0 0 0 0 0)(1 0 0 0 1)(2 0 0 0 2)(3 0 0 0 3)(4 0 0 0 4)(5 0 0 0 5)(6 0 0 0 6)(7 0 0 0 7)(8 0 0 0 8)(9 0 0 0 9)(10 0 0 0 10)(11 0 0 0 11)(12 0 0 0 12)(13 0 0 0 13)(14 0 0 0 14)(15 0 0 0 15)(16 0 0 0 16)(17 0 0 0 17)(18 0 0 0 18)(19 0 0 0 19)(20 0 0 0 20)(21 0 0 0 21)(22 0 0 0 22)(23 0 0 0 23)(24 0 0 0 24)(25 0 0 0 25)(26 0 0 0 26)(27 0 0 0 27)(28 0 0 0 28)(29 0 0 0 29)(30 0 0 0 30)(31 0 0 0 31)(32 0 0 0 32)(33 0 0 0 33)(34 0 0 0 34)(35 0 0 0 35)(36 0 0 0 36)(37 0 0 0 37)(38 0 0 0 38)(39 0 0 0 39)(40 0 0 0 40)(41 0 0 0 41)(42 0 0 0 42)(43 0 0 0 43)(44 0 0 0 44)(45 0 0 0 45)(46 0 0 0 46)(47 0 0 0 47)(48 0 0 0 48)(49 0 0 0 49)(50 0 0 0 50)(51 0 0 0 51)(52 0 0 0 52)(53 0 0 0 53)(54 0 0 0 54)(55 0 0 0 55)(56 0 0 0 56)(57 0 0 0 57)(58 0 0 0 58)(59 0 0 0 59)(60 0 0 0 60)(61 0 0 0 61)(62 0 0 0 62)(63 0 0 0 63)(64 0 0 0 64)(65 0 0 0 65)(66 0 0 0 66)(67 0 0 0 67)(68 0 0 0 68)(69 0 0 0 69)(70 0 0 0 70)(71 0 0 0 71)(72 0 0 0 72)(73 0 0 0 73)(74 0 0 0 74)(75 0 0 0 75)(76 0 0 0 76)(77 0 0 0 77)(78 0 0 0 78)(79 0 0 0 79)(80 0 0 0 80)(81 0 0 0 81)(82 0 0 0 82)(83 0 0 0 83)(84 0 0 0 84)(85 0 0 0 85)(86 0 0 0 86)(87 0 0 0 87)(88 0 0 0 88)(89 0 0 0 89)(90 0 0 0 90)(91 0 0 0 91)(92 0 0 0 92)(93 0 0 0 93)(94 0 0 0 94)(95 0 0 0 95)(96 0 0 0 96)(97 0 0 0 97)(98 0 0 0 98)(99 0 0 0 99)(100 0 0 0 100)(101 0 0 0 101)(102 0 0 0 102)(103 0 0 0 103)(104 0 0 0 104)(105 0 0 0 105)(106 0 0 0 106)(107 0 0 0 107)(108 0 0 0 108)(109 0 0 0 109)(110 0 0 0 110)(111 0 0 0 111)(112 0 0 0 112)(113 0 0 0 113)(114 0 0 0 114)(115 0 0 0 115)(116 0 0 0 116)(117 0 0 0 117)(118 0 0 0 118)(119 0 0 0 119)(120 0 0 0 120)(121 0 0 0 121)(122 0 0 0 122)(123 0 0 0 123)(124 0 0 0 124)(125 0 0 0 125)(126 0 0 0 126)(127 0 0 0 127)(128 0 0 0 128)(129 0 0 0 129)(130 0 0 0 130)(131 0 0 0 131)(132 0 0 0 132)(133 0 0 0 133)(134 0 0 0 134)(135 0 0 0 135)(136 0 0 0 136)(137 0 0 0 137)(138 0 0 0 138)(139 0 0 0 139)(140 0 0 0 140)(141 0 0 0 141)(142 0 0 0 142)(143 0 0 0 143)(144 0 0 0 144)(145 0 0 0 145)(146 0 0 0 146)(147 0 0 0 147)(148 0 0 0 148)(149 0 0 0 149)(150 0 0 0 150)(151 0 0 0 151)(152 0 0 0 152)(153 0 0 0 153)(154 0 0 0 154)(155 0 0 0 155)(156 0 0 0 156)(157 0 0 0 157)(158 0 0 0 158)(159 0 0 0 159)(160 0 0 0 160)(161 0 0 0 161)(162 0 0 0 162)(163 0 0 0 163)(164 0 0 0 164)(165 0 0 0 165)(166 0 0 0 166)(167 0 0 0 167)(168 0 0 0 168)(169 0 0 0 169)(170 0 0 0 170)(171 0 0 0 171)(172 0 0 0 172)(173 0 0 0 173)(174 0 0 0 174)(175 0 0 0 175)(176 0 0 0 176)(177 0 0 0 177)(178 0 0 0 178)(179 0 0 0 179)(180 0 0 0 180)(181 0 0 0 181)(182 0 0 0 182)(183 0 0 0 183)(184 0 0 0 184)(185 0 0 0 185)(186 0 0 0 186)(187 0 0 0 187)(188 0 0 0 188)(189 0 0 0 189)(190 0 0 0 190)(191 0 0 0 191)(192 0 0 0 192)(193 0 0 0 193)(194 0 0 0 194)(195 0 0 0 195)(196 0 0 0 196)(197 0 0 0 197)(198 0 0 0 198)(199 0 0 0 199)(200 0 0 0 200)(201 0 0 0 201)(202 0 0 0 202)(203 0 0 0 203)(204 0 0 0 204)(205 0 0 0 205)(206 0 0 0 206)(207 0 0 0 207)(208 0 0 0 208)(209 0 0 0 209)(210 0 0 0 210)(211 0 0 0 211)(212 0 0 0 212)(213 0 0 0 213)(214 0 0 0 214)(215 0 0 0 215)(216 0 0 0 216)(217 0 0 0 217)(218 0 0 0 218)(219 0 0 0 219)(220 0 0 0 220)(221 0 0 0 221)(222 0 0 0 222)(223 0 0 0 223)(224 0 0 0 224)(225 0 0 0 225)(226 0 0 0 226)(227 0 0 0 227)(228 0 0 0 228)(229 0 0 0 229)(230 0 0 0 230)(231 0 0 0 231)(232 0 0 0 232)(233 0 0 0 233)(234 0 0 0 234)(235 0 0 0 235)(236 0 0 0 236)(237 0 0 0 237)(238 0 0 0 238)(239 0 0 0 239)(240 0 0 0 240)(241 0 0 0 241)(242 0 0 0 242)(243 0 0 0 243)(244 0 0 0 244)(245 0 0 0 245)(246 0 0 0 246)(247 0 0 0 247)(248 0 0 0 248)(249 0 0 0 249)(250 0 0 0 250)(251 0 0 0 251)(252 0 0 0 252)(253 0 0 0 253)(254 0 0 0 254)(255 0 0 0 255)(256 0 0 0 256)(257 0 0 0 257)(258 0 0 0 258)(259 0 0 0 259)(260 0 0 0 260)(261 0 0 0 261)(262 0 0 0 262)(263 0 0 0 263)(264 0 0 0 264)(265 0 0 0 265)(266 0 0 0 266)(267 0 0 0 267)(268 0 0 0 268)(269 0 0 0 269)(270 0 0 0 270)(271 0 0 0 271)(272 0 0 0 272)(273 0 0 0 273)(274 0 0 0 274)(275 0 0 0 275)(276 0 0 0 276)(277 0 0 0 277)(278 0 0 0 278)(279 0 0 0 279)(280 0 0 0 280)(281 0 0 0 281)(282 0 0 0 282)(283 0 0 0 283)(284 0 0 0 284)(285 0 0 0 285)(286 0 0 0 286)(287 0 0 0 287)(288 0 0 0 288)(289 0 0 0 289)(290 0 0 0 290)(291 0 0 0 291)(292 0 0 0 292)(293 0 0 0 293)(294 0 0 0 294)(295 0 0 0 295)(296 0 0 0 296)(297 0 0 0 297)(298 0 0 0 298)(299 0 0 0 299)(300 0 0 0 300)(301 0 0 0 301)(302 0 0 0 302)(303 0 0 0 303)(304 0 0 0 304)(305 0 0 0 305)(306 0 0 0 306)(307 0 0 0 307)(308 0 0 0 308)(309 0 0 0 309)(310 0 0 0 310)(311 0 0 0 311)(312 0 0 0 312)(313 0 0 0 313)(314 0 0 0 314)(315 0 0 0 315)(316 0 0 0 316)(317 0 0 0 317)(318 0 0 0 318)(319 0 0 0 319)(320 0 0 0 320)(321 0 0 0 321)(322 0 0 0 322)(323 0 0 0 323)(324 0 0 0 324)(325 0 0 0 325)(326 0 0 0 326)(327 0 0 0 327)(328 0 0 0 328)(329 0 0 0 329)(330 0 0 0 330)(331 0 0 0 331)(332 0 0 0 332)(333 0 0 0 333)(334 0 0 0 334)(335 0 0 0 335)(336 0 0 0 336)(337 0 0 0 337)(338 0 0 0 338)(339 0 0 0 339)(340 0 0 0 340)(341 0 0 0 341)(342 0 0 0 342)(343 0 0 0 343)(344 0 0 0 344)(345 0 0 0 345)(346 0 0 0 346)(347 0 0 0 347)(348 0 0 0 348)(349 0 0 0 349)(350 0 0 0 350)(351 0 0 0 351)(352 0 0 0 352)(353 0 0 0 353)(354 0 0 0 354)(355 0 0 0 355)(356 0 0 0 356)(357 0 0 0 357)(358 0 0 0 358)(359 0 0 0 359)(360 0 0 0 360)(361 0 0 0 361)(362 0 0 0 362)(363 0 0 0 363)(364 0 0 0 364)(365 0 0 0 365)(366 0 0 0 366)(367 0 0 0 367)(368 0 0 0 368)(369 0 0 0 369)(370 0 0 0 370)(371 0 0 0 371)(372 0 0 0 372)(373 0 0 0 373)(374 0 0 0 374)(375 0 0 0 375)(376 0 0 0 376)(377 0 0 0 377)(378 0 0 0 378)(379 0 0 0 379)(380 0 0 0 380)(381 0 0 0 381)(382 0 0 0 382)(383 0 0 0 383)(384 1 0 0 0)(385 1 0 0 1)(386 1 0 0 2)(387 1 0 0 3)(388 1 0 0 4)(389 1 0 0 5)(390 1 0 0 6)(391 1 0 0 7)(392 1 0 0 8)(393 1 0 0 9)(394 1 0 0 10)(395 1 0 0 11)(396 1 0 0 12)(397 1 0 0 13)(398 1 0 0 14)(399 1 0 0 15)(400 1 0 0 16)(401 1 0 0 17)(402 1 0 0 18)(403 1 0 0 19)(404 1 0 0 20)(405 1 0 0 21)(406 1 0 0 22)(407 1 0 0 23)(408 1 0 0 24)(409 1 0 0 25)(410 1 0 0 26)(411 1 0 0 27)(412 1 0 0 28)(413 1 0 0 29)(414 1 0 0 30)(415 1 0 0 31)(416 1 0 0 32)(417 1 0 0 33)(418 1 0 0 34)(419 1 0 0 35)(420 1 0 0 36)(421 1 0 0 37)(422 1 0 0 38)(423 1 0 0 39)(424 1 0 0 40)(425 1 0 0 41)(426 1 0 0 42)(427 1 0 0 43)(428 1 0 0 44)(429 1 0 0 45)(430 1 0 0 46)(431 1 0 0 47)(432 1 0 0 48)(433 1 0 0 49)(434 1 0 0 50)(435 1 0 0 51)(436 1 0 0 52)(437 1 0 0 53)(438 1 0 0 54)(439 1 0 0 55)(440 1 0 0 56)(441 1 0 0 57)(442 1 0 0 58)(443 1 0 0 59)(444 1 0 0 60)(445 1 0 0 61)(446 1 0 0 62)(447 1 0 0 63)(448 1 0 0 64)(449 1 0 0 65)(450 1 0 0 66)(451 1 0 0 67)(452 1 0 0 68)(453 1 0 0 69)(454 1 0 0 70)(455 1 0 0 71)(456 1 0 0 72)(457 1 0 0 73)(458 1 0 0 74)(459 1 0 0 75)(460 1 0 0 76)(461 1 0 0 77)(462 1 0 0 78)(463 1 0 0 79)(464 1 0 0 80)(465 1 0 0 81)(466 1 0 0 82)(467 1 0 0 83)(468 1 0 0 84)(469 1 0 0 85)(470 1 0 0 86)(471 1 0 0 87)(472 1 0 0 88)(473 1 0 0 89)(474 1 0 0 90)(475 1 0 0 91)(476 1 0 0 92)(477 1 0 0 93)(478 1 0 0 94)(479 1 0 0 95)(480 1 0 0 96)(481 1 0 0 97)(482 1 0 0 98)(483 1 0 0 99)(484 1 0 0 100)(485 1 0 0 101)(486 1 0 0 102)(487 1 0 0 103)(488 1 0 0 104)(489 1 0 0 105)(490 1 0 0 106)(491 1 0 0 107)(492 1 0 0 108)(493 1 0 0 109)(494 1 0 0 110)(495 1 0 0 111)(496 1 0 0 112)(497 1 0 0 113)(498 1 0 0 114)(499 1 0 0 115)(500 1 0 0 116)(501 1 0 0 117)(502 1 0 0 118)(503 1 0 0 119)(504 1 0 0 120)(505 1 0 0 121)(506 1 0 0 122)(507 1 0 0 123)(508 1 0 0 124)(509 1 0 0 125)(510 1 0 0 126)(511 1 0 0 127)(512 1 0 0 128)(513 1 0 0 129)(514 1 0 0 130)(515 1 0 0 131)(516 1 0 0 132)(517 1 0 0 133)(518 1 0 0 134)(519 1 0 0 135)(520 1 0 0 136)(521 1 0 0 137)(522 1 0 0 138)(523 1 0 0 139)(524 1 0 0 140)(525 1 0 0 141)(526 1 0 0 142)(527 1 0 0 143)(528 1 0 0 144)(529 1 0 0 145)(530 1 0 0 146)(531 1 0 0 147)(532 1 0 0 148)(533 1 0 0 149)(534 1 0 0 150)(535 1 0 0 151)(536 1 0 0 152)(537 1 0 0 153)(538 1 0 0 154)(539 1 0 0 155)(540 1 0 0 156)(541 1 0 0 157)(542 1 0 0 158)(543 1 0 0 159)(544 1 0 0 160)(545 1 0 0 161)(546 1 0 0 162)(547 1 0 0 163)(548 1 0 0 164)(549 1 0 0 165)(550 1 0 0 166)(551 1 0 0 167)(552 1 0 0 168)(553 1 0 0 169)(554 1 0 0 170)(555 1 0 0 171)(556 1 0 0 172)(557 1 0 0 173)(558 1 0 0 174)(559 1 0 0 175)(560 1 0 0 176)(561 1 0 0 177)(562 1 0 0 178)(563 1 0 0 179)(564 1 0 0 180)(565 1 0 0 181)(566 1 0 0 182)(567 1 0 0 183)(568 1 0 0 184)(569 1 0 0 185)(570 1 0 0 186)(571 1 0 0 187)(572 1 0 0 188)(573 1 0 0 189)(574 1 0 0 190)(575 1 0 0 191)(576 1 0 0 192)(577 1 0 0 193)(578 1 0 0 194)(579 1 0 0 195)(580 1 0 0 196)(581 1 0 0 197)(582 1 0 0 198)(583 1 0 0 199)(584 1 0 0 200)(585 1 0 0 201)(586 1 0 0 202)(587 1 0 0 203)(588 1 0 0 204)(589 1 0 0 205)(590 1 0 0 206)(591 1 0 0 207)(592 1 0 0 208)(593 1 0 0 209)(594 1 0 0 210)(595 1 0 0 211)(596 1 0 0 212)(597 1 0 0 213)(598 1 0 0 214)(599 1 0 0 215)(600 1 0 0 216)(601 1 0 0 217)(602 1 0 0 218)(603 1 0 0 219)(604 1 0 0 220)(605 1 0 0 221)(606 1 0 0 222)(607 1 0 0 223)(608 1 0 0 224)(609 1 0 0 225)(610 1 0 0 226)(611 1 0 0 227)(612 1 0 0 228)(613 1 0 0 229)(614 1 0 0 230)(615 1 0 0 231)(616 1 0 0 232)(617 1 0 0 233)(618 1 0 0 234)(619 1 0 0 235)(620 1 0 0 236)(621 1 0 0 237)(622 1 0 0 238)(623 1 0 0 239)(624 1 0 0 240)(625 1 0 0 241)(626 1 0 0 242)(627 1 0 0 243)(628 1 0 0 244)(629 1 0 0 245)(630 1 0 0 246)(631 1 0 0 247)(632 1 0 0 248)(633 1 0 0 249)(634 1 0 0 250)(635 1 0 0 251)(636 1 0 0 252)(637 1 0 0 253)(638 1 0 0 254)(639 1 0 0 255)(640 1 0 0 256)(641 1 0 0 257)(642 1 0 0 258)(643 1 0 0 259)(644 1 0 0 260)(645 1 0 0 261)(646 1 0 0 262)(647 1 0 0 263)(648 1 0 0 264)(649 1 0 0 265)(650 1 0 0 266)(651 1 0 0 267)(652 1 0 0 268)(653 1 0 0 269)(654 1 0 0 270)(655 1 0 0 271)(656 1 0 0 272)(657 1 0 0 273)(658 1 0 0 274)(659 1 0 0 275)(660 1 0 0 276)(661 1 0 0 277)(662 1 0 0 278)(663 1 0 0 279)(664 1 0 0 280)(665 1 0 0 281)(666 1 0 0 282)(667 1 0 0 283)(668 1 0 0 284)(669 1 0 0 285)(670 1 0 0 286)(671 1 0 0 287)(672 1 0 0 288)(673 1 0 0 289)(674 1 0 0 290)(675 1 0 0 291)(676 1 0 0 292)(677 1 0 0 293)(678 1 0 0 294)(679 1 0 0 295)(680 1 0 0 296)(681 1 0 0 297)(682 1 0 0 298)(683 1 0 0 299)(684 1 0 0 300)(685 1 0 0 301)(686 1 0 0 302)(687 1 0 0 303)(688 1 0 0 304)(689 1 0 0 305)(690 1 0 0 306)(691 1 0 0 307)(692 1 0 0 308)(693 1 0 0 309)(694 1 0 0 310)(695 1 0 0 311)(696 1 0 0 312)(697 1 0 0 313)(698 1 0 0 314)(699 1 0 0 315)(700 1 0 0 316)(701 1 0 0 317)(702 1 0 0 318)(703 1 0 0 319)(704 1 0 0 320)(705 1 0 0 321)(706 1 0 0 322)(707 1 0 0 323)(708 1 0 0 324)(709 1 0 0 325)(710 1 0 0 326)(711 1 0 0 327)(712 1 0 0 328)(713 1 0 0 329)(714 1 0 0 330)(715 1 0 0 331)(716 1 0 0 332)(717 1 0 0 333)(718 1 0 0 334)(719 1 0 0 335)(720 1 0 0 336)(721 1 0 0 337)(722 1 0 0 338)(723 1 0 0 339)(724 1 0 0 340)(725 1 0 0 341)(726 1 0 0 342)(727 1 0 0 343)(728 1 0 0 344)(729 1 0 0 345)(730 1 0 0 346)(731 1 0 0 347)(732 1 0 0 348)(733 1 0 0 349)(734 1 0 0 350)(735 1 0 0 351)(736 1 0 0 352)(737 1 0 0 353)(738 1 0 0 354)(739 1 0 0 355)(740 1 0 0 356)(741 1 0 0 357)(742 1 0 0 358)(743 1 0 0 359)(744 1 0 0 360)(745 1 0 0 361)(746 1 0 0 362)(747 1 0 0 363)(748 1 0 0 364)(749 1 0 0 365)(750 1 0 0 366)(751 1 0 0 367)(752 1 0 0 368)(753 1 0 0 369)(754 1 0 0 370)(755 1 0 0 371)(756 1 0 0 372)(757 1 0 0 373)(758 1 0 0 374)(759 1 0 0 375)(760 1 0 0 376)(761 1 0 0 377)(762 1 0 0 378)(763 1 0 0 379)(764 1 0 0 380)(765 1 0 0 381)(766 1 0 0 382)(767 1 0 0 383)(768 2 0 0 0)(769 2 0 0 1)(770 2 0 0 2)(771 2 0 0 3)(772 2 0 0 4)(773 2 0 0 5)(774 2 0 0 6)(775 2 0 0 7)(776 2 0 0 8)(777 2 0 0 9)(778 2 0 0 10)(779 2 0 0 11)(780 2 0 0 12)(781 2 0 0 13)(782 2 0 0 14)(783 2 0 0 15)(784 2 0 0 16)(785 2 0 0 17)(786 2 0 0 18)(787 2 0 0 19)(788 2 0 0 20)(789 2 0 0 21)(790 2 0 0 22)(791 2 0 0 23)(792 2 0 0 24)(793 2 0 0 25)(794 2 0 0 26)(795 2 0 0 27)(796 2 0 0 28)(797 2 0 0 29)(798 2 0 0 30)(799 2 0 0 31)(800 2 0 0 32)(801 2 0 0 33)(802 2 0 0 34)(803 2 0 0 35)(804 2 0 0 36)(805 2 0 0 37)(806 2 0 0 38)(807 2 0 0 39)(808 2 0 0 40)(809 2 0 0 41)(810 2 0 0 42)(811 2 0 0 43)(812 2 0 0 44)(813 2 0 0 45)(814 2 0 0 46)(815 2 0 0 47)(816 2 0 0 48)(817 2 0 0 49)(818 2 0 0 50)(819 2 0 0 51)(820 2 0 0 52)(821 2 0 0 53)(822 2 0 0 54)(823 2 0 0 55)(824 2 0 0 56)(825 2 0 0 57)(826 2 0 0 58)(827 2 0 0 59)(828 2 0 0 60)(829 2 0 0 61)(830 2 0 0 62)(831 2 0 0 63)(832 2 0 0 64)(833 2 0 0 65)(834 2 0 0 66)(835 2 0 0 67)(836 2 0 0 68)(837 2 0 0 69)(838 2 0 0 70)(839 2 0 0 71)(840 2 0 0 72)(841 2 0 0 73)(842 2 0 0 74)(843 2 0 0 75)(844 2 0 0 76)(845 2 0 0 77)(846 2 0 0 78)(847 2 0 0 79)(848 2 0 0 80)(849 2 0 0 81)(850 2 0 0 82)(851 2 0 0 83)(852 2 0 0 84)(853 2 0 0 85)(854 2 0 0 86)(855 2 0 0 87)(856 2 0 0 88)(857 2 0 0 89)(858 2 0 0 90)(859 2 0 0 91)(860 2 0 0 92)(861 2 0 0 93)(862 2 0 0 94)(863 2 0 0 95)(864 2 0 0 96)(865 2 0 0 97)(866 2 0 0 98)(867 2 0 0 99)(868 2 0 0 100)(869 2 0 0 101)(870 2 0 0 102)(871 2 0 0 103)(872 2 0 0 104)(873 2 0 0 105)(874 2 0 0 106)(875 2 0 0 107)(876 2 0 0 108)(877 2 0 0 109)(878 2 0 0 110)(879 2 0 0 111)(880 2 0 0 112)(881 2 0 0 113)(882 2 0 0 114)(883 2 0 0 115)(884 2 0 0 116)(885 2 0 0 117)(886 2 0 0 118)(887 2 0 0 119)(888 2 0 0 120)(889 2 0 0 121)(890 2 0 0 122)(891 2 0 0 123)(892 2 0 0 124)(893 2 0 0 125)(894 2 0 0 126)(895 2 0 0 127)(896 2 0 0 128)(897 2 0 0 129)(898 2 0 0 130)(899 2 0 0 131)(900 2 0 0 132)(901 2 0 0 133)(902 2 0 0 134)(903 2 0 0 135)(904 2 0 0 136)(905 2 0 0 137)(906 2 0 0 138)(907 2 0 0 139)(908 2 0 0 140)(909 2 0 0 141)(910 2 0 0 142)(911 2 0 0 143)(912 2 0 0 144)(913 2 0 0 145)(914 2 0 0 146)(915 2 0 0 147)(916 2 0 0 148)(917 2 0 0 149)(918 2 0 0 150)(919 2 0 0 151)(920 2 0 0 152)(921 2 0 0 153)(922 2 0 0 154)(923 2 0 0 155)(924 2 0 0 156)(925 2 0 0 157)(926 2 0 0 158)(927 2 0 0 159)(928 2 0 0 160)(929 2 0 0 161)(930 2 0 0 162)(931 2 0 0 163)(932 2 0 0 164)(933 2 0 0 165)(934 2 0 0 166)(935 2 0 0 167)(936 2 0 0 168)(937 2 0 0 169)(938 2 0 0 170)(939 2 0 0 171)(940 2 0 0 172)(941 2 0 0 173)(942 2 0 0 174)(943 2 0 0 175)(944 2 0 0 176)(945 2 0 0 177)(946 2 0 0 178)(947 2 0 0 179)(948 2 0 0 180)(949 2 0 0 181)(950 2 0 0 182)(951 2 0 0 183)(952 2 0 0 184)(953 2 0 0 185)(954 2 0 0 186)(955 2 0 0 187)(956 2 0 0 188)(957 2 0 0 189)(958 2 0 0 190)(959 2 0 0 191)(960 2 0 0 192)(961 2 0 0 193)(962 2 0 0 194)(963 2 0 0 195)(964 2 0 0 196)(965 2 0 0 197)(966 2 0 0 198)(967 2 0 0 199)(968 2 0 0 200)(969 2 0 0 201)(970 2 0 0 202)(971 2 0 0 203)(972 2 0 0 204)(973 2 0 0 205)(974 2 0 0 206)(975 2 0 0 207)(976 2 0 0 208)(977 2 0 0 209)(978 2 0 0 210)(979 2 0 0 211)(980 2 0 0 212)(981 2 0 0 213)(982 2 0 0 214)(983 2 0 0 215)(984 2 0 0 216)(985 2 0 0 217)(986 2 0 0 218)(987 2 0 0 219)(988 2 0 0 220)(989 2 0 0 221)(990 2 0 0 222)(991 2 0 0 223)(992 2 0 0 224)(993 2 0 0 225)(994 2 0 0 226)(995 2 0 0 227)(996 2 0 0 228)(997 2 0 0 229)(998 2 0 0 230)(999 2 0 0 231)(1000 2 0 0 232)(1001 2 0 0 233)(1002 2 0 0 234)(1003 2 0 0 235)(1004 2 0 0 236)(1005 2 0 0 237)(1006 2 0 0 238)(1007 2 0 0 239)(1008 2 0 0 240)(1009 2 0 0 241)(1010 2 0 0 242)(1011 2 0 0 243)(1012 2 0 0 244)(1013 2 0 0 245)(1014 2 0 0 246)(1015 2 0 0 247)(1016 2 0 0 248)(1017 2 0 0 249)(1018 2 0 0 250)(1019 2 0 0 251)(1020 2 0 0 252)(1021 2 0 0 253)(1022 2 0 0 254)(1023 2 0 0 255)(1024 2 0 0 256)(1025 2 0 0 257)(1026 2 0 0 258)(1027 2 0 0 259)(1028 2 0 0 260)(1029 2 0 0 261)(1030 2 0 0 262)(1031 2 0 0 263)(1032 2 0 0 264)(1033 2 0 0 265)(1034 2 0 0 266)(1035 2 0 0 267)(1036 2 0 0 268)(1037 2 0 0 269)(1038 2 0 0 270)(1039 2 0 0 271)(1040 2 0 0 272)(1041 2 0 0 273)(1042 2 0 0 274)(1043 2 0 0 275)(1044 2 0 0 276)(1045 2 0 0 277)(1046 2 0 0 278)(1047 2 0 0 279)(1048 2 0 0 280)(1049 2 0 0 281)(1050 2 0 0 282)(1051 2 0 0 283)(1052 2 0 0 284)(1053 2 0 0 285)(1054 2 0 0 286)(1055 2 0 0 287)(1056 2 0 0 288)(1057 2 0 0 289)(1058 2 0 0 290)(1059 2 0 0 291)(1060 2 0 0 292)(1061 2 0 0 293)(1062 2 0 0 294)(1063 2 0 0 295)(1064 2 0 0 296)(1065 2 0 0 297)(1066 2 0 0 298)(1067 2 0 0 299)(1068 2 0 0 300)(1069 2 0 0 301)(1070 2 0 0 302)(1071 2 0 0 303)(1072 2 0 0 304)(1073 2 0 0 305)(1074 2 0 0 306)(1075 2 0 0 307)(1076 2 0 0 308)(1077 2 0 0 309)(1078 2 0 0 310)(1079 2 0 0 311)(1080 2 0 0 312)(1081 2 0 0 313)(1082 2 0 0 314)(1083 2 0 0 315)(1084 2 0 0 316)(1085 2 0 0 317)(1086 2 0 0 318)(1087 2 0 0 319)(1088 2 0 0 320)(1089 2 0 0 321)(1090 2 0 0 322)(1091 2 0 0 323)(1092 2 0 0 324)(1093 2 0 0 325)(1094 2 0 0 326)(1095 2 0 0 327)(1096 2 0 0 328)(1097 2 0 0 329)(1098 2 0 0 330)(1099 2 0 0 331)(1100 2 0 0 332)(1101 2 0 0 333)(1102 2 0 0 334)(1103 2 0 0 335)(1104 2 0 0 336)(1105 2 0 0 337)(1106 2 0 0 338)(1107 2 0 0 339)(1108 2 0 0 340)(1109 2 0 0 341)(1110 2 0 0 342)(1111 2 0 0 343)(1112 2 0 0 344)(1113 2 0 0 345)(1114 2 0 0 346)(1115 2 0 0 347)(1116 2 0 0 348)(1117 2 0 0 349)(1118 2 0 0 350)(1119 2 0 0 351)(1120 2 0 0 352)(1121 2 0 0 353)(1122 2 0 0 354)(1123 2 0 0 355)(1124 2 0 0 356)(1125 2 0 0 357)(1126 2 0 0 358)(1127 2 0 0 359)(1128 2 0 0 360)(1129 2 0 0 361)(1130 2 0 0 362)(1131 2 0 0 363)(1132 2 0 0 364)(1133 2 0 0 365)(1134 2 0 0 366)(1135 2 0 0 367)(1136 2 0 0 368)(1137 2 0 0 369)(1138 2 0 0 370)(1139 2 0 0 371)(1140 2 0 0 372)(1141 2 0 0 373)(1142 2 0 0 374)(1143 2 0 0 375)(1144 2 0 0 376)(1145 2 0 0 377)(1146 2 0 0 378)(1147 2 0 0 379)(1148 2 0 0 380)(1149 2 0 0 381)(1150 2 0 0 382)(1151 2 0 0 383)(1152 3 0 0 0)(1153 3 0 0 1)(1154 3 0 0 2)(1155 3 0 0 3)(1156 3 0 0 4)(1157 3 0 0 5)(1158 3 0 0 6)(1159 3 0 0 7)(1160 3 0 0 8)(1161 3 0 0 9)(1162 3 0 0 10)(1163 3 0 0 11)(1164 3 0 0 12)(1165 3 0 0 13)(1166 3 0 0 14)(1167 3 0 0 15)(1168 3 0 0 16)(1169 3 0 0 17)(1170 3 0 0 18)(1171 3 0 0 19)(1172 3 0 0 20)(1173 3 0 0 21)(1174 3 0 0 22)(1175 3 0 0 23)(1176 3 0 0 24)(1177 3 0 0 25)(1178 3 0 0 26)(1179 3 0 0 27)(1180 3 0 0 28)(1181 3 0 0 29)(1182 3 0 0 30)(1183 3 0 0 31)(1184 3 0 0 32)(1185 3 0 0 33)(1186 3 0 0 34)(1187 3 0 0 35)(1188 3 0 0 36)(1189 3 0 0 37)(1190 3 0 0 38)(1191 3 0 0 39)(1192 3 0 0 40)(1193 3 0 0 41)(1194 3 0 0 42)(1195 3 0 0 43)(1196 3 0 0 44)(1197 3 0 0 45)(1198 3 0 0 46)(1199 3 0 0 47)(1200 3 0 0 48)(1201 3 0 0 49)(1202 3 0 0 50)(1203 3 0 0 51)(1204 3 0 0 52)(1205 3 0 0 53)(1206 3 0 0 54)(1207 3 0 0 55)(1208 3 0 0 56)(1209 3 0 0 57)(1210 3 0 0 58)(1211 3 0 0 59)(1212 3 0 0 60)(1213 3 0 0 61)(1214 3 0 0 62)(1215 3 0 0 63)(1216 3 0 0 64)(1217 3 0 0 65)(1218 3 0 0 66)(1219 3 0 0 67)(1220 3 0 0 68)(1221 3 0 0 69)(1222 3 0 0 70)(1223 3 0 0 71)(1224 3 0 0 72)(1225 3 0 0 73)(1226 3 0 0 74)(1227 3 0 0 75)(1228 3 0 0 76)(1229 3 0 0 77)(1230 3 0 0 78)(1231 3 0 0 79)(1232 3 0 0 80)(1233 3 0 0 81)(1234 3 0 0 82)(1235 3 0 0 83)(1236 3 0 0 84)(1237 3 0 0 85)(1238 3 0 0 86)(1239 3 0 0 87)(1240 3 0 0 88)(1241 3 0 0 89)(1242 3 0 0 90)(1243 3 0 0 91)(1244 3 0 0 92)(1245 3 0 0 93)(1246 3 0 0 94)(1247 3 0 0 95)(1248 3 0 0 96)(1249 3 0 0 97)(1250 3 0 0 98)(1251 3 0 0 99)(1252 3 0 0 100)(1253 3 0 0 101)(1254 3 0 0 102)(1255 3 0 0 103)(1256 3 0 0 104)(1257 3 0 0 105)(1258 3 0 0 106)(1259 3 0 0 107)(1260 3 0 0 108)(1261 3 0 0 109)(1262 3 0 0 110)(1263 3 0 0 111)(1264 3 0 0 112)(1265 3 0 0 113)(1266 3 0 0 114)(1267 3 0 0 115)(1268 3 0 0 116)(1269 3 0 0 117)(1270 3 0 0 118)(1271 3 0 0 119)(1272 3 0 0 120)(1273 3 0 0 121)(1274 3 0 0 122)(1275 3 0 0 123)(1276 3 0 0 124)(1277 3 0 0 125)(1278 3 0 0 126)(1279 3 0 0 127)(1280 3 0 0 128)(1281 3 0 0 129)(1282 3 0 0 130)(1283 3 0 0 131)(1284 3 0 0 132)(1285 3 0 0 133)(1286 3 0 0 134)(1287 3 0 0 135)(1288 3 0 0 136)(1289 3 0 0 137)(1290 3 0 0 138)(1291 3 0 0 139)(1292 3 0 0 140)(1293 3 0 0 141)(1294 3 0 0 142)(1295 3 0 0 143)(1296 3 0 0 144)(1297 3 0 0 145)(1298 3 0 0 146)(1299 3 0 0 147)(1300 3 0 0 148)(1301 3 0 0 149)(1302 3 0 0 150)(1303 3 0 0 151)(1304 3 0 0 152)(1305 3 0 0 153)(1306 3 0 0 154)(1307 3 0 0 155)(1308 3 0 0 156)(1309 3 0 0 157)(1310 3 0 0 158)(1311 3 0 0 159)(1312 3 0 0 160)(1313 3 0 0 161)(1314 3 0 0 162)(1315 3 0 0 163)(1316 3 0 0 164)(1317 3 0 0 165)(1318 3 0 0 166)(1319 3 0 0 167)(1320 3 0 0 168)(1321 3 0 0 169)(1322 3 0 0 170)(1323 3 0 0 171)(1324 3 0 0 172)(1325 3 0 0 173)(1326 3 0 0 174)(1327 3 0 0 175)(1328 3 0 0 176)(1329 3 0 0 177)(1330 3 0 0 178)(1331 3 0 0 179)(1332 3 0 0 180)(1333 3 0 0 181)(1334 3 0 0 182)(1335 3 0 0 183)(1336 3 0 0 184)(1337 3 0 0 185)(1338 3 0 0 186)(1339 3 0 0 187)(1340 3 0 0 188)(1341 3 0 0 189)(1342 3 0 0 190)(1343 3 0 0 191)(1344 3 0 0 192)(1345 3 0 0 193)(1346 3 0 0 194)(1347 3 0 0 195)(1348 3 0 0 196)(1349 3 0 0 197)(1350 3 0 0 198)(1351 3 0 0 199)(1352 3 0 0 200)(1353 3 0 0 201)(1354 3 0 0 202)(1355 3 0 0 203)(1356 3 0 0 204)(1357 3 0 0 205)(1358 3 0 0 206)(1359 3 0 0 207)(1360 3 0 0 208)(1361 3 0 0 209)(1362 3 0 0 210)(1363 3 0 0 211)(1364 3 0 0 212)(1365 3 0 0 213)(1366 3 0 0 214)(1367 3 0 0 215)(1368 3 0 0 216)(1369 3 0 0 217)(1370 3 0 0 218)(1371 3 0 0 219)(1372 3 0 0 220)(1373 3 0 0 221)(1374 3 0 0 222)(1375 3 0 0 223)(1376 3 0 0 224)(1377 3 0 0 225)(1378 3 0 0 226)(1379 3 0 0 227)(1380 3 0 0 228)(1381 3 0 0 229)(1382 3 0 0 230)(1383 3 0 0 231)(1384 3 0 0 232)(1385 3 0 0 233)(1386 3 0 0 234)(1387 3 0 0 235)(1388 3 0 0 236)(1389 3 0 0 237)(1390 3 0 0 238)(1391 3 0 0 239)(1392 3 0 0 240)(1393 3 0 0 241)(1394 3 0 0 242)(1395 3 0 0 243)(1396 3 0 0 244)(1397 3 0 0 245)(1398 3 0 0 246)(1399 3 0 0 247)(1400 3 0 0 248)(1401 3 0 0 249)(1402 3 0 0 250)(1403 3 0 0 251)(1404 3 0 0 252)(1405 3 0 0 253)(1406 3 0 0 254)(1407 3 0 0 255)(1408 3 0 0 256)(1409 3 0 0 257)(1410 3 0 0 258)(1411 3 0 0 259)(1412 3 0 0 260)(1413 3 0 0 261)(1414 3 0 0 262)(1415 3 0 0 263)(1416 3 0 0 264)(1417 3 0 0 265)(1418 3 0 0 266)(1419 3 0 0 267)(1420 3 0 0 268)(1421 3 0 0 269)(1422 3 0 0 270)(1423 3 0 0 271)(1424 3 0 0 272)(1425 3 0 0 273)(1426 3 0 0 274)(1427 3 0 0 275)(1428 3 0 0 276)(1429 3 0 0 277)(1430 3 0 0 278)(1431 3 0 0 279)(1432 3 0 0 280)(1433 3 0 0 281)(1434 3 0 0 282)(1435 3 0 0 283)(1436 3 0 0 284)(1437 3 0 0 285)(1438 3 0 0 286)(1439 3 0 0 287)(1440 3 0 0 288)(1441 3 0 0 289)(1442 3 0 0 290)(1443 3 0 0 291)(1444 3 0 0 292)(1445 3 0 0 293)(1446 3 0 0 294)(1447 3 0 0 295)(1448 3 0 0 296)(1449 3 0 0 297)(1450 3 0 0 298)(1451 3 0 0 299)(1452 3 0 0 300)(1453 3 0 0 301)(1454 3 0 0 302)(1455 3 0 0 303)(1456 3 0 0 304)(1457 3 0 0 305)(1458 3 0 0 306)(1459 3 0 0 307)(1460 3 0 0 308)(1461 3 0 0 309)(1462 3 0 0 310)(1463 3 0 0 311)(1464 3 0 0 312)(1465 3 0 0 313)(1466 3 0 0 314)(1467 3 0 0 315)(1468 3 0 0 316)(1469 3 0 0 317)(1470 3 0 0 318)(1471 3 0 0 319)(1472 3 0 0 320)(1473 3 0 0 321)(1474 3 0 0 322)(1475 3 0 0 323)(1476 3 0 0 324)(1477 3 0 0 325)(1478 3 0 0 326)(1479 3 0 0 327)(1480 3 0 0 328)(1481 3 0 0 329)(1482 3 0 0 330)(1483 3 0 0 331)(1484 3 0 0 332)(1485 3 0 0 333)(1486 3 0 0 334)(1487 3 0 0 335)(1488 3 0 0 336)(1489 3 0 0 337)(1490 3 0 0 338)(1491 3 0 0 339)(1492 3 0 0 340)(1493 3 0 0 341)(1494 3 0 0 342)(1495 3 0 0 343)(1496 3 0 0 344)(1497 3 0 0 345)(1498 3 0 0 346)(1499 3 0 0 347)(1500 3 0 0 348)(1501 3 0 0 349)(1502 3 0 0 350)(1503 3 0 0 351)(1504 3 0 0 352)(1505 3 0 0 353)(1506 3 0 0 354)(1507 3 0 0 355)(1508 3 0 0 356)(1509 3 0 0 357)(1510 3 0 0 358)(1511 3 0 0 359)(1512 3 0 0 360)(1513 3 0 0 361)(1514 3 0 0 362)(1515 3 0 0 363)(1516 3 0 0 364)(1517 3 0 0 365)(1518 3 0 0 366)(1519 3 0 0 367)(1520 3 0 0 368)(1521 3 0 0 369)(1522 3 0 0 370)(1523 3 0 0 371)(1524 3 0 0 372)(1525 3 0 0 373)(1526 3 0 0 374)(1527 3 0 0 375)(1528 3 0 0 376)(1529 3 0 0 377)(1530 3 0 0 378)(1531 3 0 0 379)(1532 3 0 0 380)(1533 3 0 0 381)(1534 3 0 0 382)(1535 3 0 0 383
~imroTbl=(1110,2,0,500,250,1)(0 0 0)(1 0 0)(2 0 0)(3 0 0)(4 0 0)(5 0 0)(6 0 0)(7 0 0)(8 0 0)(9 0 0)(10 0 0)(11 0 0)(12 0 0)(13 0 0)(14 0 0)(15 0 0)(16 0 0)(17 0 0)(18 0 0)(19 0 0)(20 0 0)(21 0 0)(22 0 0)(23 0 0)
```

The first entry of the Imec Readout Table (imRo) is a header indicating
(probe-type, number of channels). The meaning of the subsequent channel
entries depends upon the probe type.

>Note: Unlike {snsGeomMap, snsShankMap, snsChanMap}, which store entries
only for saved channels, the imroTbl always has entries for all acquired
channels.

### Channel Entries By Type

**Type {0,1020,1030,1100,1120,1121,1122,1123,1200,1300}, NP 1.0-like**:

* Channel ID,
* Bank number of the connected electrode,
* Reference ID index,
* AP band gain,
* LF band gain,
* AP hipass filter applied (1=ON)

The reference ID values are {0=ext, 1=tip, [2..4]=on-shnk-ref}.
The on-shnk ref electrodes are {192,576,960}.

**Type {21,2003} (NP 2.0, single multiplexed shank)**:

* Channel ID,
* Bank mask (logical OR of {1=bnk-0, 2=bnk-1, 4=bnk-2, 8=bnk-3}),
* Reference ID index,
* Electrode ID (range [0,1279])

Type-21 reference ID values are {0=ext, 1=tip, [2..5]=on-shnk-ref}.
The on-shnk ref electrodes are {127,507,887,1251}.

Type-2003 reference ID values are {0=ext, 1=gnd, 2=tip}. On-shank
reference electrodes are removed from commercial 2B probes.

**Type {24,2013} (NP 2.0, 4-shank)**:

* Channel ID,
* Shank ID (with tips pointing down, shank-0 is left-most),
* Bank ID,
* Reference ID index,
* Electrode ID (range [0,1279] on each shank)

Type-24 reference ID values are {0=ext, [1..4]=tip[0..3], [5..8]=on-shnk-0, [9..12]=on-shnk-1, [13..16]=on-shnk-2, [17..20]=on-shnk-3}.
The on-shnk ref electrodes of any shank are {127,511,895,1279}.

Type-2013 reference ID values are {0=ext, 1=gnd, [2..5]=tip[0..3]}. On-shank
reference electrodes are removed from commercial 2B probes.

**Type {2020} (Quad-probe)**:

* Channel ID,
* Shank ID (with tips pointing down, shank-0 is left-most),
* Bank ID,
* Reference ID index,
* Electrode ID (range [0,1279] on each shank)

Quad-base reference ID values are {0=ext, 1=gnd, 2=tip on same shank as electode}.

**Type 1110 (UHD programmable)**:

This has a unique header:

* Type (1110),
* ColumnMode {0=INNER, 1=OUTER, 2=ALL},
* Reference ID index {0=ext, 1=tip},
* AP band gain,
* LF band gain,
* AP hipass filter applied (1=ON)

And 24 entries following the header:

* Group ID,
* Bank-A,
* Bank-B,

```
~muxTbl=(32,12)(0 1 24 25 48 49 72 73 96 97 120 121 144 145 168 169 192 193 216 217 240 241 264 265 288 289 312 313 336 337 360 361)(2 3 26 27 50 51 74 75 98 99 122 123 146 147 170 171 194 195 218 219 242 243 266 267 290 291 314 315 338 339 362 363)(4 5 28 29 52 53 76 77 100 101 124 125 148 149 172 173 196 197 220 221 244 245 268 269 292 293 316 317 340 341 364 365)(6 7 30 31 54 55 78 79 102 103 126 127 150 151 174 175 198 199 222 223 246 247 270 271 294 295 318 319 342 343 366 367)(8 9 32 33 56 57 80 81 104 105 128 129 152 153 176 177 200 201 224 225 248 249 272 273 296 297 320 321 344 345 368 369)(10 11 34 35 58 59 82 83 106 107 130 131 154 155 178 179 202 203 226 227 250 251 274 275 298 299 322 323 346 347 370 371)(12 13 36 37 60 61 84 85 108 109 132 133 156 157 180 181 204 205 228 229 252 253 276 277 300 301 324 325 348 349 372 373)(14 15 38 39 62 63 86 87 110 111 134 135 158 159 182 183 206 207 230 231 254 255 278 279 302 303 326 327 350 351 374 375)(16 17 40 41 64 65 88 89 112 113 136 137 160 161 184 185 208 209 232 233 256 257 280 281 304 305 328 329 352 353 376 377)(18 19 42 43 66 67 90 91 114 115 138 139 162 163 186 187 210 211 234 235 258 259 282 283 306 307 330 331 354 355 378 379)(20 21 44 45 68 69 92 93 116 117 140 141 164 165 188 189 212 213 236 237 260 261 284 285 308 309 332 333 356 357 380 381)(22 23 46 47 70 71 94 95 118 119 142 143 166 167 190 191 214 215 238 239 262 263 286 287 310 311 334 335 358 359 382 383)
```

The multiplexing table header describes (nADC,nGrp) where nADC is the number
of analog-to-digital converters on the probe, hence, the number of channels
that are digitized at the same time, and nGrp is the number of channels
assigned to each ADC, hence, the number of channel groups that are digitized
for each sample.

There are `count=nGrp` subsequent entries (), each listing the `count=nADC`
channels that are digitized together. The table is used to "tshift" the
data which temporally aligns the channel groups to each other. This is done
by post-processing software such as CatGT.

```
~snsChanMap=(384,384,1)(AP0;0:0)(AP1;1:1)(AP2;2:2)(AP3;3:3)(AP4;4:4)(AP5;5:5)(AP6;6:6)(AP7;7:7)(AP8;8:8)(AP9;9:9)(AP10;10:10)(AP11;11:11)(AP12;12:12)(AP13;13:13)(AP14;14:14)(AP15;15:15)(AP16;16:16)(AP17;17:17)(AP18;18:18)(AP19;19:19)(AP20;20:20)(AP21;21:21)(AP22;22:22)(AP23;23:23)(AP24;24:24)(AP25;25:25)(AP26;26:26)(AP27;27:27)(AP28;28:28)(AP29;29:29)(AP30;30:30)(AP31;31:31)(AP32;32:32)(AP33;33:33)(AP34;34:34)(AP35;35:35)(AP36;36:36)(AP37;37:37)(AP38;38:38)(AP39;39:39)(AP40;40:40)(AP41;41:41)(AP42;42:42)(AP43;43:43)(AP44;44:44)(AP45;45:45)(AP46;46:46)(AP47;47:47)(AP48;48:48)(AP49;49:49)(AP50;50:50)(AP51;51:51)(AP52;52:52)(AP53;53:53)(AP54;54:54)(AP55;55:55)(AP56;56:56)(AP57;57:57)(AP58;58:58)(AP59;59:59)(AP60;60:60)(AP61;61:61)(AP62;62:62)(AP63;63:63)(AP64;64:64)(AP65;65:65)(AP66;66:66)(AP67;67:67)(AP68;68:68)(AP69;69:69)(AP70;70:70)(AP71;71:71)(AP72;72:72)(AP73;73:73)(AP74;74:74)(AP75;75:75)(AP76;76:76)(AP77;77:77)(AP78;78:78)(AP79;79:79)(AP80;80:80)(AP81;81:81)(AP82;82:82)(AP83;83:83)(AP84;84:84)(AP85;85:85)(AP86;86:86)(AP87;87:87)(AP88;88:88)(AP89;89:89)(AP90;90:90)(AP91;91:91)(AP92;92:92)(AP93;93:93)(AP94;94:94)(AP95;95:95)(AP96;96:96)(AP97;97:97)(AP98;98:98)(AP99;99:99)(AP100;100:100)(AP101;101:101)(AP102;102:102)(AP103;103:103)(AP104;104:104)(AP105;105:105)(AP106;106:106)(AP107;107:107)(AP108;108:108)(AP109;109:109)(AP110;110:110)(AP111;111:111)(AP112;112:112)(AP113;113:113)(AP114;114:114)(AP115;115:115)(AP116;116:116)(AP117;117:117)(AP118;118:118)(AP119;119:119)(AP120;120:120)(AP121;121:121)(AP122;122:122)(AP123;123:123)(AP124;124:124)(AP125;125:125)(AP126;126:126)(AP127;127:127)(AP128;128:128)(AP129;129:129)(AP130;130:130)(AP131;131:131)(AP132;132:132)(AP133;133:133)(AP134;134:134)(AP135;135:135)(AP136;136:136)(AP137;137:137)(AP138;138:138)(AP139;139:139)(AP140;140:140)(AP141;141:141)(AP142;142:142)(AP143;143:143)(AP144;144:144)(AP145;145:145)(AP146;146:146)(AP147;147:147)(AP148;148:148)(AP149;149:149)(AP150;150:150)(AP151;151:151)(AP152;152:152)(AP153;153:153)(AP154;154:154)(AP155;155:155)(AP156;156:156)(AP157;157:157)(AP158;158:158)(AP159;159:159)(AP160;160:160)(AP161;161:161)(AP162;162:162)(AP163;163:163)(AP164;164:164)(AP165;165:165)(AP166;166:166)(AP167;167:167)(AP168;168:168)(AP169;169:169)(AP170;170:170)(AP171;171:171)(AP172;172:172)(AP173;173:173)(AP174;174:174)(AP175;175:175)(AP176;176:176)(AP177;177:177)(AP178;178:178)(AP179;179:179)(AP180;180:180)(AP181;181:181)(AP182;182:182)(AP183;183:183)(AP184;184:184)(AP185;185:185)(AP186;186:186)(AP187;187:187)(AP188;188:188)(AP189;189:189)(AP190;190:190)(AP191;191:191)(AP192;192:192)(AP193;193:193)(AP194;194:194)(AP195;195:195)(AP196;196:196)(AP197;197:197)(AP198;198:198)(AP199;199:199)(AP200;200:200)(AP201;201:201)(AP202;202:202)(AP203;203:203)(AP204;204:204)(AP205;205:205)(AP206;206:206)(AP207;207:207)(AP208;208:208)(AP209;209:209)(AP210;210:210)(AP211;211:211)(AP212;212:212)(AP213;213:213)(AP214;214:214)(AP215;215:215)(AP216;216:216)(AP217;217:217)(AP218;218:218)(AP219;219:219)(AP220;220:220)(AP221;221:221)(AP222;222:222)(AP223;223:223)(AP224;224:224)(AP225;225:225)(AP226;226:226)(AP227;227:227)(AP228;228:228)(AP229;229:229)(AP230;230:230)(AP231;231:231)(AP232;232:232)(AP233;233:233)(AP234;234:234)(AP235;235:235)(AP236;236:236)(AP237;237:237)(AP238;238:238)(AP239;239:239)(AP240;240:240)(AP241;241:241)(AP242;242:242)(AP243;243:243)(AP244;244:244)(AP245;245:245)(AP246;246:246)(AP247;247:247)(AP248;248:248)(AP249;249:249)(AP250;250:250)(AP251;251:251)(AP252;252:252)(AP253;253:253)(AP254;254:254)(AP255;255:255)(AP256;256:256)(AP257;257:257)(AP258;258:258)(AP259;259:259)(AP260;260:260)(AP261;261:261)(AP262;262:262)(AP263;263:263)(AP264;264:264)(AP265;265:265)(AP266;266:266)(AP267;267:267)(AP268;268:268)(AP269;269:269)(AP270;270:270)(AP271;271:271)(AP272;272:272)(AP273;273:273)(AP274;274:274)(AP275;275:275)(AP276;276:276)(AP277;277:277)(AP278;278:278)(AP279;279:279)(AP280;280:280)(AP281;281:281)(AP282;282:282)(AP283;283:283)(AP284;284:284)(AP285;285:285)(AP286;286:286)(AP287;287:287)(AP288;288:288)(AP289;289:289)(AP290;290:290)(AP291;291:291)(AP292;292:292)(AP293;293:293)(AP294;294:294)(AP295;295:295)(AP296;296:296)(AP297;297:297)(AP298;298:298)(AP299;299:299)(AP300;300:300)(AP301;301:301)(AP302;302:302)(AP303;303:303)(AP304;304:304)(AP305;305:305)(AP306;306:306)(AP307;307:307)(AP308;308:308)(AP309;309:309)(AP310;310:310)(AP311;311:311)(AP312;312:312)(AP313;313:313)(AP314;314:314)(AP315;315:315)(AP316;316:316)(AP317;317:317)(AP318;318:318)(AP319;319:319)(AP320;320:320)(AP321;321:321)(AP322;322:322)(AP323;323:323)(AP324;324:324)(AP325;325:325)(AP326;326:326)(AP327;327:327)(AP328;328:328)(AP329;329:329)(AP330;330:330)(AP331;331:331)(AP332;332:332)(AP333;333:333)(AP334;334:334)(AP335;335:335)(AP336;336:336)(AP337;337:337)(AP338;338:338)(AP339;339:339)(AP340;340:340)(AP341;341:341)(AP342;342:342)(AP343;343:343)(AP344;344:344)(AP345;345:345)(AP346;346:346)(AP347;347:347)(AP348;348:348)(AP349;349:349)(AP350;350:350)(AP351;351:351)(AP352;352:352)(AP353;353:353)(AP354;354:354)(AP355;355:355)(AP356;356:356)(AP357;357:357)(AP358;358:358)(AP359;359:359)(AP360;360:360)(AP361;361:361)(AP362;362:362)(AP363;363:363)(AP364;364:364)(AP365;365:365)(AP366;366:366)(AP367;367:367)(AP368;368:368)(AP369;369:369)(AP370;370:370)(AP371;371:371)(AP372;372:372)(AP373;373:373)(AP374;374:374)(AP375;375:375)(AP376;376:376)(AP377;377:377)(AP378;378:378)(AP379;379:379)(AP380;380:380)(AP381;381:381)(AP382;382:382)(AP383;383:383)(SY0;768:768)
```

The channel map describes the order of graphs in SpikeGLX displays. The
header for the imec stream, here (384,384,1), indicates there are:

* 384 AP-band channels,
* 384 LF-band channels,
* 1 SY (sync) channel.

Each subsequent entry in the map has two fields, (:)-separated:

* Channel name, e.g., 'AP2;2'
* Zero-based order index.

>Note: There are map entries only for saved channels.

```
~snsGeomMap=(NP1000,1,0,70)(0:27:0:1)(0:59:0:1)(0:11:20:1)(0:43:20:1)(0:27:40:1)(0:59:40:1)(0:11:60:1)(0:43:60:1)(0:27:80:1)(0:59:80:1)(0:11:100:1)(0:43:100:1)(0:27:120:1)(0:59:120:1)(0:11:140:1)(0:43:140:1)(0:27:160:1)(0:59:160:1)(0:11:180:1)(0:43:180:1)(0:27:200:1)(0:59:200:1)(0:11:220:1)(0:43:220:1)(0:27:240:1)(0:59:240:1)(0:11:260:1)(0:43:260:1)(0:27:280:1)(0:59:280:1)(0:11:300:1)(0:43:300:1)(0:27:320:1)(0:59:320:1)(0:11:340:1)(0:43:340:1)(0:27:360:1)(0:59:360:1)(0:11:380:1)(0:43:380:1)(0:27:400:1)(0:59:400:1)(0:11:420:1)(0:43:420:1)(0:27:440:1)(0:59:440:1)(0:11:460:1)(0:43:460:1)(0:27:480:1)(0:59:480:1)(0:11:500:1)(0:43:500:1)(0:27:520:1)(0:59:520:1)(0:11:540:1)(0:43:540:1)(0:27:560:1)(0:59:560:1)(0:11:580:1)(0:43:580:1)(0:27:600:1)(0:59:600:1)(0:11:620:1)(0:43:620:1)(0:27:640:1)(0:59:640:1)(0:11:660:1)(0:43:660:1)(0:27:680:1)(0:59:680:1)(0:11:700:1)(0:43:700:1)(0:27:720:1)(0:59:720:1)(0:11:740:1)(0:43:740:1)(0:27:760:1)(0:59:760:1)(0:11:780:1)(0:43:780:1)(0:27:800:1)(0:59:800:1)(0:11:820:1)(0:43:820:1)(0:27:840:1)(0:59:840:1)(0:11:860:1)(0:43:860:1)(0:27:880:1)(0:59:880:1)(0:11:900:1)(0:43:900:1)(0:27:920:1)(0:59:920:1)(0:11:940:1)(0:43:940:1)(0:27:960:1)(0:59:960:1)(0:11:980:1)(0:43:980:1)(0:27:1000:1)(0:59:1000:1)(0:11:1020:1)(0:43:1020:1)(0:27:1040:1)(0:59:1040:1)(0:11:1060:1)(0:43:1060:1)(0:27:1080:1)(0:59:1080:1)(0:11:1100:1)(0:43:1100:1)(0:27:1120:1)(0:59:1120:1)(0:11:1140:1)(0:43:1140:1)(0:27:1160:1)(0:59:1160:1)(0:11:1180:1)(0:43:1180:1)(0:27:1200:1)(0:59:1200:1)(0:11:1220:1)(0:43:1220:1)(0:27:1240:1)(0:59:1240:1)(0:11:1260:1)(0:43:1260:1)(0:27:1280:1)(0:59:1280:1)(0:11:1300:1)(0:43:1300:1)(0:27:1320:1)(0:59:1320:1)(0:11:1340:1)(0:43:1340:1)(0:27:1360:1)(0:59:1360:1)(0:11:1380:1)(0:43:1380:1)(0:27:1400:1)(0:59:1400:1)(0:11:1420:1)(0:43:1420:1)(0:27:1440:1)(0:59:1440:1)(0:11:1460:1)(0:43:1460:1)(0:27:1480:1)(0:59:1480:1)(0:11:1500:1)(0:43:1500:1)(0:27:1520:1)(0:59:1520:1)(0:11:1540:1)(0:43:1540:1)(0:27:1560:1)(0:59:1560:1)(0:11:1580:1)(0:43:1580:1)(0:27:1600:1)(0:59:1600:1)(0:11:1620:1)(0:43:1620:1)(0:27:1640:1)(0:59:1640:1)(0:11:1660:1)(0:43:1660:1)(0:27:1680:1)(0:59:1680:1)(0:11:1700:1)(0:43:1700:1)(0:27:1720:1)(0:59:1720:1)(0:11:1740:1)(0:43:1740:1)(0:27:1760:1)(0:59:1760:1)(0:11:1780:1)(0:43:1780:1)(0:27:1800:1)(0:59:1800:1)(0:11:1820:1)(0:43:1820:1)(0:27:1840:1)(0:59:1840:1)(0:11:1860:1)(0:43:1860:1)(0:27:1880:1)(0:59:1880:1)(0:11:1900:1)(0:43:1900:0)(0:27:1920:1)(0:59:1920:1)(0:11:1940:1)(0:43:1940:1)(0:27:1960:1)(0:59:1960:1)(0:11:1980:1)(0:43:1980:1)(0:27:2000:1)(0:59:2000:1)(0:11:2020:1)(0:43:2020:1)(0:27:2040:1)(0:59:2040:1)(0:11:2060:1)(0:43:2060:1)(0:27:2080:1)(0:59:2080:1)(0:11:2100:1)(0:43:2100:1)(0:27:2120:1)(0:59:2120:1)(0:11:2140:1)(0:43:2140:1)(0:27:2160:1)(0:59:2160:1)(0:11:2180:1)(0:43:2180:1)(0:27:2200:1)(0:59:2200:1)(0:11:2220:1)(0:43:2220:1)(0:27:2240:1)(0:59:2240:1)(0:11:2260:1)(0:43:2260:1)(0:27:2280:1)(0:59:2280:1)(0:11:2300:1)(0:43:2300:1)(0:27:2320:1)(0:59:2320:1)(0:11:2340:1)(0:43:2340:1)(0:27:2360:1)(0:59:2360:1)(0:11:2380:1)(0:43:2380:1)(0:27:2400:1)(0:59:2400:1)(0:11:2420:1)(0:43:2420:1)(0:27:2440:1)(0:59:2440:1)(0:11:2460:1)(0:43:2460:1)(0:27:2480:1)(0:59:2480:1)(0:11:2500:1)(0:43:2500:1)(0:27:2520:1)(0:59:2520:1)(0:11:2540:1)(0:43:2540:1)(0:27:2560:1)(0:59:2560:1)(0:11:2580:1)(0:43:2580:1)(0:27:2600:1)(0:59:2600:1)(0:11:2620:1)(0:43:2620:1)(0:27:2640:1)(0:59:2640:1)(0:11:2660:1)(0:43:2660:1)(0:27:2680:1)(0:59:2680:1)(0:11:2700:1)(0:43:2700:1)(0:27:2720:1)(0:59:2720:1)(0:11:2740:1)(0:43:2740:1)(0:27:2760:1)(0:59:2760:1)(0:11:2780:1)(0:43:2780:1)(0:27:2800:1)(0:59:2800:1)(0:11:2820:1)(0:43:2820:1)(0:27:2840:1)(0:59:2840:1)(0:11:2860:1)(0:43:2860:1)(0:27:2880:1)(0:59:2880:1)(0:11:2900:1)(0:43:2900:1)(0:27:2920:1)(0:59:2920:1)(0:11:2940:1)(0:43:2940:1)(0:27:2960:1)(0:59:2960:1)(0:11:2980:1)(0:43:2980:1)(0:27:3000:1)(0:59:3000:1)(0:11:3020:1)(0:43:3020:1)(0:27:3040:1)(0:59:3040:1)(0:11:3060:1)(0:43:3060:1)(0:27:3080:1)(0:59:3080:1)(0:11:3100:1)(0:43:3100:1)(0:27:3120:1)(0:59:3120:1)(0:11:3140:1)(0:43:3140:1)(0:27:3160:1)(0:59:3160:1)(0:11:3180:1)(0:43:3180:1)(0:27:3200:1)(0:59:3200:1)(0:11:3220:1)(0:43:3220:1)(0:27:3240:1)(0:59:3240:1)(0:11:3260:1)(0:43:3260:1)(0:27:3280:1)(0:59:3280:1)(0:11:3300:1)(0:43:3300:1)(0:27:3320:1)(0:59:3320:1)(0:11:3340:1)(0:43:3340:1)(0:27:3360:1)(0:59:3360:1)(0:11:3380:1)(0:43:3380:1)(0:27:3400:1)(0:59:3400:1)(0:11:3420:1)(0:43:3420:1)(0:27:3440:1)(0:59:3440:1)(0:11:3460:1)(0:43:3460:1)(0:27:3480:1)(0:59:3480:1)(0:11:3500:1)(0:43:3500:1)(0:27:3520:1)(0:59:3520:1)(0:11:3540:1)(0:43:3540:1)(0:27:3560:1)(0:59:3560:1)(0:11:3580:1)(0:43:3580:1)(0:27:3600:1)(0:59:3600:1)(0:11:3620:1)(0:43:3620:1)(0:27:3640:1)(0:59:3640:1)(0:11:3660:1)(0:43:3660:1)(0:27:3680:1)(0:59:3680:1)(0:11:3700:1)(0:43:3700:1)(0:27:3720:1)(0:59:3720:1)(0:11:3740:1)(0:43:3740:1)(0:27:3760:1)(0:59:3760:1)(0:11:3780:1)(0:43:3780:1)(0:27:3800:1)(0:59:3800:1)(0:11:3820:1)(0:43:3820:1)
```

The GeomMap replaces the ShankMap for imec probes (as of SpikeGLX 20230202).
It describes how electrodes are arranged on the probe. The first () entry
is a header. Here, (NP1000,1,0,70) indicates probe part-number NP1000. This
standard NP 1.0 probe has 1 shank, the spacing between shanks is 0 microns
and the per_shank_width is 70 microns. Each following electrode entry has
four values (s:x:z:u):

1. zero-based shank # (with tips pointing down, shank-0 is left-most),

2. x-coordinate (um) of **electrode center**,

3. z-coordinate (um) of **electrode center**,

4. 0/1 "used," or, (u-flag), indicating if the electrode should be drawn
in the FileViewer and ShankViewer windows, and if it should be included in
spatial average \<S\> calculations.

> **(X,Z) Coordinates**: Looking face-on at a probe with tip pointing
downward, X is measured along the width of the probe, from left to right.
The X-origin is the left edge of the shank. Z is measured along the shank
of the probe, from tip, upward, toward the probe base. The Z-origin is the
center of the bottom-most electrode row (closest to the tip). Note that for
a multi-shank probe, each shank has its own (X,Z) origin and coordinate
system. That is, the (X,Z) location of an electrode is given relative to
its own shank (with tips pointing down, shank-0 is left-most).

>Note: There are electrode entries only for saved channels.

```
~svySBTT=(0 1 312963 360017)(0 2 644811 692174)
```

The list of survey transitions:
(shank, bank, start-time(samples), end-time(samples)).

The end-times indicate when the amplifiers have settled (more or less).

--------

## Obx

```
acqXaDwSy
```

This is the count of channels, of each type, in each timepoint,
at acquisition time.

```
obAiRangeMax=5.0
```

Convert from 16-bit analog values (i) to voltages (V) as follows:

V = i * Vmax / Imax.

For obx data:

* Imax = 32768
* Vmax = `obAiRangeMax`

```
obAiRangeMin=-5.0
```

```
obMaxInt=32768
```

Maximum amplitude integer encoded in the 16-bit analog channels.
Really, in this example [-32768..32767]. The reason for this apparent
asymmetry is that, by convention, zero is grouped with the positive
values. The stream is 16-bit so can encode 2^16 = 65536 values.
There are 32768 negative values: [-32768..-1] and 32768 positives: [0..32767].
This convention (zero is a positive number) applies in all signed
computer arithmetic.

```
obSampRate=30000
```

This is the best estimator of the sample rate. It will be the calibrated
rate if calibration was done.

```
imDatApi=3.31.0
```

This is the Imec API version number: major.minor.build.

```
imDatBs_fw=2.0.137
```

This is the BS firmware version number: major.minor.build.

```
imDatBsc_fw=3.2.176
```

This is the BSC firmware version number: major.minor.build.

```
imDatBsc_hw=2.1
```

This is the BSC hardware version number: major.minor.

```
imDatBsc_pn=NP2_QBSC_00
```

This is the BSC part number.

```
imDatBsc_sn=175
```

This is the PXI BSC serial number or OneBox physical ID number.

```
imDatObx_slot=25
```

This is the obx slot number, range [20..31].

```
imTrgRising=true
```

Selects whether external trigger detects a rising or falling edge.

```
imTrgSource=0
```

Selects the type of trigger that starts the run: {0=software}.

```
snsXaDwSy=11,1,1
```

This is the count of channels, of each type, in each timepoint,
as stored in the binary file.

```
~snsChanMap=(11,1,1)(XA0;0:0)(XA1;1:1)(XA2;2:2)(XA3;3:3)(XA4;4:4)(XA5;5:5)(XA6;6:6)(XA7;7:7)(XA8;8:8)(XA9;9:9)(XA10;10:10)(XD0;11:11)(SY0;12:12)
```

The channel map describes the order of graphs in SpikeGLX displays. The
header for the obx stream, here (11,1,1), indicates there are:

* 11 XA-type obx analog input channels,
* 1 XD-type obx digital word.
* 1 SY (sync) channel.

Each subsequent entry in the map has two fields, (:)-separated:

* Channel name, e.g., 'XA2;2'
* Zero-based order index.

>Note: There are map entries only for saved channels.


_fin_


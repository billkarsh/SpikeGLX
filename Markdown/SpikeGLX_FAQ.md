# SpikeGLX FAQ

**Topics:**

* [Why SpikeGLX?](#why-spikeglx)
* [How to Uninstall](#how-to-uninstall)
* [Side by Side Versions](#side-by-side-versions)
* [Running Two Copies of SpikeGLX](#running-two-copies-of-spikeglx)
* [Data Integrity](#data-integrity)
    + [Graceful Shutdown](#graceful-shutdown)
    + [Crash](#crash)
* [Gauging System Health](#gauging-system-health)
* [How to Report Bugs](#how-to-report-bugs)
* [Graphics Problems](#graphics-problems)
    + [Fuzzy Text](#fuzzy-text)
    + [Black Graphics](#black-graphics)
    + [Graphs Flicker](#graphs-flicker)
* [Remote Desktop](#remote-desktop)
    + [Audio Output Tip](#audio-output-tip)
    + [Audio Output Crash](#audio-output-crash)

--------

## Why SpikeGLX?

"What's the point? Why SpikeGLX vs. the other leading brand?"

1) **Strong integration and synchronization**.
As originally conceived the Imec 'Neuropixels' probes recorded lots of
neural channels but had very limited auxiliary inputs for accelerometers,
physiological readouts, lick responses, door activations and so on.
Combining the Imec and Nidq streams vastly expands the aux inputs
available in your experiment. Moreover, all of the data are tightly
synchronized. You can see them together on screen during an experiment,
the output files are synced to within a few samples and the offline
File Viewer lets you review all the recorded data in a time-locked way.
This helps you see the experiment as an integrated whole.

2) **Several options for timer or event-driven control of file writing**.

3) **Remote control**.
You can use the MATLAB, Python, C#, C or C++ interfaces over a network
connection to: set/get run parameters, start/stop runs and file writing,
and retrieve data, all in real time.

--------

## How to Uninstall

"How do I completely remove SpikeGLX from my computer?"

When you download a release of SpikeGLX you get a folder with a name like
"Release_v20161101". Everything SpikeGLX needs to run is in that folder.
There are no Registry entries, no DLLs placed into Windows OS folders or
any other cookies or crumbs.

To delete it, drag the release folder to the trash.

--------

## Side by Side Versions

"Can I have multiple versions of SpikeGLX on one computer?...Will they
interfere with each other?"

Yes, not a problem. Refer to FAQ
[How to Uninstall](#how-to-uninstall) to see
that each SpikeGLX setup is self-contained. We organize things like this:

```
SpikeGLX/               // master folder with all versions
    ...
    Release_v20160703/  // a release folder
    Release_v20160806/  // another
    Release_v20161101/  // and so on
    ...
```

--------

## Running Two Copies of SpikeGLX

"What happens if I try to run two copies at the same time?"

If you are only doing offline things, like using the File Viewer, there is
no problem. You can run as many instances as you want until you run out
of RAM. There are only issues if data acquisition hardware is involved.

If you are using NI-DAQ hardware, you have to visit the Configuration dialog
in SpikeGLX, check `Enable NI-DAQ` and click `Detect`. The first instance
of the app to do that gets ownership of the NI-DAQ hardware and no other
instance will be allowed to use it. This prevents accidental conflicts which
is a good thing, but it also prevents running multiple NI-based probes on
one host computer.

Imec hardware uses a fixed static IP address, and that generally prevents
running two setups from one host. There is a workaround developed
<[here](https://github.com/cortex-lab/neuropixels/wiki/Multiprobe_acquisition)>
that I haven't tried; please don't ask me for any details about that.

Another related question is whether one can run two copies of SpikeGLX on
one computer, each running a command server that can be individually
addressed from MATLAB. Yes, it works fine. In each copy of SpikeGLX, use
`Options/Command Server Settings...` to assign the computer's network IP
address, but give each application its own port number. From MATLAB,
connect using `SpikeGL( IP-address, port )`.

--------

## Data Integrity

"My run quit unexpectedly, are my data likely to be corrupt or garbage?"

### Graceful Shutdown

SpikeGLX monitors a number of health and performance metrics during a
data taking run. If there are signs of **pending** trouble it will
initiate a graceful shutdown of the run before a catastrophic failure
occurs. It closes open data files and then stops the data acquisition.
Messages in the Console window's log will describe the specific problem
encountered and the fact that the run was stopped. Your data files are
intact because we close them before corruption happens.

### Crash

A graceful shutdown, described above, is **not a "crash"**. Software
engineers reserve the term "crash" for a completely pathological and
unexpected event that is so bad, the operating system must step in
and terminate the application with extreme prejudice before harm is
done to other programs or to the OS itself.

What are the signs of a crash? When an event like this happens, there
will usually be a Windows OS dialog box on the screen with a cryptic
message about quitting unexpectedly. Often, since Windows 7, the screen
has a peculiar milky appearance after a crash. A really severe crash ends
in the blue screen of death.

Crashes are usually the consequence of software bugs (bad practice) and
these mistakes are fixed over time as they're uncovered. If a crash happens
while running SpikeGLX (unlikely), you're still in pretty good shape
because the data files will be intact up to the moment just before the
crash. The crash itself will be very swift, so only the last few data
points in the files may be in question.

You can edit and recover files even after a crash by doing two things:

1) Right-click on the `bin` file in Windows File Explorer and select
`Properties` to get the size in bytes (**not** size on disk). This needs
to be an integral number of whole timepoints. A timepoint has size:
numChannels x bytesPerChannel, that is: meta-item `nSavedChans` x 2.
If the actual file size does not match, trim the excess using a binary
file editor or using a Linux editing tool like `head` to remove the excess,
like this:
`head -c [number of bytes to keep] [my bin filename] > [my new filename.bin]`

2) If a crash occurs the final write to the meta file may not happen, so
you'll need to reconstruct three key meta items. First, set `fileSizeBytes=nn`,
where, nn is the same size as discussed in step (1). Second, set
`fileTimeSecs=ss`, the span of the file in seconds, calculated like this:

```
    ss = fileSizeBytes / xxSampleRate / nSavedChans / 2,
    where, xxSampleRate is the niSampleRate or imSampleRate
    recorded in the same metafile.
```

Lastly, if the meta file is missing checksum item `fileSHA1` then simply
add an entry setting it to zero: `fileSHA1=0`. This will allow you to
open the file with the SpikeGLX File Viewer.

--------

## Gauging System Health

"What can I observe about SpikeGLX to look for performance issues?"

- Open the Windows Task Manager and watch network performance. The
usage graph runs at something like 15.3% to 15.5% for Imec data transfer
and is pretty steady. Steady is good, while varying throughput means
the system is struggling.

- The main Console window status bar, **when writing files**, displays the
current file buffer fill percentages (imec%,nidq%), which should remain
below 5%, and it shows the disk write rate which should be around 1500+ MB/s.
The write rate could be much higher for SSD drives. If these are varying
the system is struggling.

SpikeGLX doesn’t write data files with gaps. Rather, if any resources
are choked beyond a monitored threshold the run is stopped gracefully.
There are some specific monitoring messages that may appear in the text
of the Console window…

- If data are not being pulled from the hardware fast enough you’ll
start to see messages like this: **"IMEC FIFOQFill% value, …”** These
messages are triggered if the fill % exceeds 5%. The run is automatically
stopped at 95%. No messages are written if the percentage stays or drops
below 5%.

- The data are placed into a central stream where graphs, file writing
and other services can get it. If that queue is backing up you’ll get
messages like: **"AIQ::enqueue low mem."**

- If a data service is bogging down fetching from the stream you may see
one or more of:
**"AIQ::catBlocks low mem"**, **"GraphFetcher low mem…"**,
**"Write queue low mem"**, **"Trigger low mem"**.

- The Metrics window (Ctrl+M) gathers these readouts making it easy to
monitor run health. Be sure to read the help file for that window.

--------

## How to Report Bugs

If something unexpected happens while running SpikeGLX try to gather these
two files for diagnosis:

1. A **screen shot** that covers as much context as you can get, including
any Windows message box about the incident. You can make a screen shot by
pressing `shift + Print-Screen`. This saves a picture file to the clipboard.
You can then paste the picture into MS Paint and save that as a `jpg` image.

2. The **Console window's log**. If the program is still operable you can
use command `Tools/Save Log File...` This is the best way to get the log
content because it provides some context about what SpikeGLX was doing
before the error. Alternatively, if a run has quit due to an error, SpikeGLX
saves a brief file with the same name as your run and next to the bin and
meta files. It's called `runname.errors.txt`.

If the computer is hung so you can't save files, the next best thing is
to write down any error messages you see in dialog boxes and the Console
window.

--------

## Graphics Problems

### Fuzzy Text

"Why is the text in my SpikeGLX controls, menus and log window so tiny
and hard to read?"

Try altering the following Windows setting:

1. Right-click on your Windows desktop and select 'Display settings.'
2. Click 'Advanced scaling settings.'
3. Find the button called 'Let Windows try to fix apps so they're not blurry.'
4. Turn this feature **OFF**. -- Yes, disable it.
5. Quit and restart SpikeGLX.

### Black Graphics

"Why do the graphs and shank views look mostly black?"

You likely have missing or old OpenGL drivers. Follow these steps:

1. Get and install the `Realtech VR OpenGL Extensions Viewer`. The installer
app is called [GLView](https://www.realtech-vr.com/home/?page_id=142).
*If the link doesn't work, Google for 'glview'.*

2. The installed 'OpenGL Extensions Viewer' app is under the 'Realtech VR'
folder in your Start Menu.

The viewer displays info about your installed graphics hardware, your
current version of OpenGL, enabled features, and more.

There is a link in the app to help you find the latest drivers for your
card.

### Graphs Flicker

"Why do the Graph window panels appear to flash or flicker?"

Windows may be using your computer's built-in graphics instead of your
higher performance graphics card. Follow these steps:

1. Type "Graphics settings" into the search box of the Windows Taskbar.
2. Select `Desktop app` and below that click `Browse`.
3. Navigate to and select SpikeGLX.exe.
4. SpikeGLX now appears below the `Browse` button; click on it.
5. Click `Options`; select the `High performance` option and `Save`.
6. Restart SpikeGLX.

--------

## Remote Desktop

"Can I use Windows Remote Desktop Services (RDP) to check up on SpikeGLX
remotely?"

* For **Phase3B2 and later**: No. These versions of SpikeGLX use a later
release of OpenGL that is incompatible with RDP. There are many other
remote control apps that do work fine, like AnyDesk or TeamViewer.

* For **Phase3B1 and Phase3A**: Yes you can; it generally works, but to use Audio Out,
there's a tip needed to make it work, and a hitch that could cause it
to crash.

### Audio Output Tip

You probably already know that you have to enable Audio redirection on
the client computer from the **Remote Desktop Connection** dialog:
`Options\Local Resources tab\Remote Audio\Settings...\Play on this computer`.

However, you might also have to enable sound redirection on the host
computer that's running SpikeGLX. Do that by running the Windows **Group
Policy Editor** application: From the Window Start menu, use the `Run`
option to launch `gpedit.msc`. In the editor, navigate down to:

```
    Computer Configuration\
    Administrative Templates\
    Windows Components\
    Remote Desktop Services\
    Remote Desktop Session Host\
    Device and Resource Redirection\
    Allow audio and video playback redirection
```

Make sure this is set to `Enabled`.

### Audio Output Crash

You should be fine in an RDP session turning sound on or off via the
`SpikeGLX Audio Settings` dialog `Apply` and `Stop` buttons. However,
if you quit your RDP session without first stopping the audio, SpikeGLX
will crash on the host computer. **Manually stop audio output and then
close the RDP session.**

>Programmer note: If I could detect an event that tells me an RDP session
is about to close, I could programmatically stop the audio and prevent the
crash. While there does exist a Windows message: **WM_WTSSESSION_CHANGE**
with wParam **WTS_REMOTE_DISCONNECT**, it arrives after the session is
already closed: too late to prevent the problem. No preceding message
traffic predicts an impending disconnect.


_fin_


## Audio Notes

### WARNING!

Audio output is surprisingly resource hungry. If you are running 16 imec
probes and recording all channels to disk, you may find that turning the
audio on tips the system into instability. You'll hear excessive skips
and crackle in the audio stream, and you'll see performance drop off in
the Metrics window.

Audio is fine as long as the system is not at maximum stress.

### Output to Speakers and Headphones

As of version **20170501** SpikeGLX  no longer uses NI-DAQ based analog out.
Rather, selected channels are fed to the Windows sound drivers and, just
like music, are accessed through the PC speakers and headphone jack. You
can also use USB-based headphones.

If you want, you can make a cable to go from the audio jack to a BNC and
feed the signal into an A-M Systems 3300 Audio Monitor box. I've tested
that.

### Hot-plugging Headphones

I advise using the 'Stop' button to stop audio output before un/plugging
headphones. Sometimes changing the hardware when sound is running will
cause a crash.

Some later versions of Windows will show a dialog asking you to confirm
that the device is headphones or other. This dialog may be hidden behind
SpikeGLX windows. You'll have to find and answer the dialog before sound
output can resume.

### Stream

Because they run at different sample rates you can listen
to Imec channels or NI channels, but not both at the same time.

### Channels

You can select any channel you can see in the Graphs Window, even the
digital channels.

You can set the Left and Right channels the same, or, use your two ears
to monitor two channels at the same time.

You can **right-click on the Graphs Window** to select channels. When you do
that, the stream will change to the one you clicked on, and audio out
will start immediately if not already running.

### Bandpass

The two popup menus work together to define a pass band. The left popup
controls the left edge (the highpass part):

* Select 'OFF' to turn all filtering completely off.
* Select '0' to enable the right popup and set only lowpass filtering.

Remember that DC offset in the signal can be removed by applying at least
a **0.1 Hz** filter. DC may cause clipping, which manifests as distortion
and artifacts.

### Volume

Be careful not to boost volume too much. As amplitude grows, transitions
in frequency are faster and may be harder for you to hear. For example,
a ramp becomes more like a pulse edge and will contain less audible detail.

Of course, clipping is a risk at large volumes.

### Latency

Latency can be measured using an oscilloscope and a signal generator setup.
I find the Imec stream latency to be roughly **135 to 200 ms**. NI latency
is roughly **110 to 130 ms**. Disturbances in total computer activity can
cause the latency to slowly drift or even take a jump to a new value.
However, resetting the audio, that is, stopping and starting it again, will
bring latency back to its normal range.

The following actions perform a stop/start reset (if audio is already running):

* Pressing the 'Apply' button.
* Changing any parameter in the Audio Dialog.
* Autoreset: The software automatically does a restart if the internally
monitored latency appears near the high end of the above ranges. How often
this occurs is a function of computer, OS and goblins. In typical quiet
conditions this is every ~5 minutes.

Resets cause a momentary crackle.

### Crackle

Audio crackle (brief glitches) may occur occasionally. The actual sound
drivers only work at a limited set of sample rates, and those do not match
the sample rates of our data streams, so the data are resampled within the
Windows sound drivers. This creates a small amount of distortion (as does
filtering), and every so often the driver finds itself missing a sample.
The missing sample causes a discontinuity in the waveform, which sounds
like a crackle.


_fin_


## AO Wiring Notes

>General rule: AO clock source **is same as** AI clock source.

### Using Whisper and Separate AO Device

(1) In the AO dialog, select a PFI terminal.
(2) In the Configure Acquisition dialog, set Chans/muxer to the appropriate
value for your Whisper box (32 is most common).
(3) Connect the "Sample Clock" output BNC from the Whisper to the selected
PFI terminal on the NI breakout box for the AO device.

>Note: The Whisper should be running at: `(nominal sample rate)
X (muxing factor)`. For example, a typical Whisper box samples at 25 KHz
with 32-channel muxers, so the BNC provides a 800 KHz clock.

### Using Separate AI Device and AO Device

(1) In the AO dialog, select a PFI terminal.
(2) In the Configure Acquisition dialog, set Chans/muxer to 1.
(2) Locate a user manual for the AI device. Connect the CTR0 output terminal
of the AI device to the selected PFI terminal of the AO device.

### Using One Device for Both AI and AO

(1) Select "Internal" clock source both in the AO dialog and in the
Configure Acquisition dialog (for device1 input).
(2) In the Configure Acquisition dialog, set Chans/muxer to 1.

## AO Channel String (Soft Wiring)

An example channel string in the AO dialog looks like: `0=14, 1=0, 3=T`.
This routes AI data stream channel index 14 to NI breakout box AO terminal
#0; AI channel index zero goes to AO terminal #1; and AO terminal #3
gets whatever AI channel has been selected as the trigger channel.

In other words, **(AO hardware channel)=(AI virtual channel)**.


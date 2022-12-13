
-------
Version
-------

SpikeGLX Release v20221031-phase30.

Imec API at v3.52.


IMPORTANT
---------

[[ DO NOT DETECT OR RUN UNTIL YOU HAVE UPDATED YOUR MODULE FIRMWARE!!
   All versions since release 20220101 use the same module firmware:
   BS 169, BSC 176 (included in the release package). You do not need
   to update firmware if you already did so since version 20220101. ]]

- Start SpikeGLX 3.0 and use the 'Tools/Update Imec Firmware' command.
- Read the help document in that dialog ('?' in titlebar).
- Select the files included in this release's 'Firmware' folder.
- Power cycle everything, that is, shut everything off, wait a few seconds, then turn on again.
- Now you can run.


Current release highlights
--------------------------

+ Fix imec sync SMA input.
Just after SfN we discovered that sync input to the imec modules was not working. This was reported to the slack channel (November 21, 2022) with recommended workarounds. In brief, the imec input responds to rising edges 29 samples late (~1 ms). That is fixed here.

+ Fix IMRO editor boxes selector bug.
At times editor clicks were not detected.

+ Fix shankMap handling for NI files.
No longer assume every NI file has a shankMap.

+ Improve Graphs window slider paging.
Clicking in the page part of the channel selector scrollbar moves 1/4 full channel range; making it easier to scroll to LFP pages.

+ Add LFP filter to Graphs window.
Live insertion can now monitor for LFP signatures.

+ Add FileViewer command to view meta file.
From the FileViewer you can now open the corresponding metadata file.

+ Add ~muxTbl entry to probe metadata.
Output metadata for probes now includes mux table data allowing post processors to apply the proper TShift corrections for this probe. (CatGT always had these tables internally).

+ Add sim slots.
You can do simulated runs without any hardware by adding a simulated slot and simulated probes.

+ Add sim probes.
Sim probes use preexisting data files to substitute voltage values for real hardware. Basically, this replays file data through the entire data acquisition stream as if it was real time data.


Critical release notes
----------------------

- Using SpikeGLX will be very familiar if you used NP 1.0 (any prior version).

- For 2.0 headstages, Dock 1 is on the side of the HS with two large capacitors and the Omnetics connector. Dock 2 is on the label side (flatter contour). You can use either dock or both.

- The BIST (probe health tests) all function for 1.0-like probes. For 2.0 probes, the {heartbeat, signal, noise} tests are not supported by 2.0 hardware.

- The Noise BIST for 1.0-like probes has a peculiarity. If the test is successful the report says so, no problem. However, if the noise test fails, it really has failed...but the reason for the failure that is quoted in the report is wrong. The reason tends to say something about a firmware version issue, but this text string is faulty and should be ignored.

- As always, you should calibrate your HS sample clock rates. These are stored in SpikeGLX using the HS serial number as the key because this a measurement about that particular part. However, imec did not include an EEPROM chip on the early-model 2.0 HS so the HS does not know its own serial number. SpikeGLX will require you to type the serial number into a dialog when you click Detect. You need to have read that from the label on the HS. This was corrected for later 2.0 headstages and the serial numbers are read for later HS automatically.


If You've Already Used Imec Probes...
-------------------------------------

This release supports all current probes (NOT phase3A). The look and feel of the software and hardware for all phases are quite similar, making it all the more important to recognize that there are key differences:

<< BEWARE!! >>
The ZIF connector on the 1.0 and 2.0 headstage (HS) works oppositely from 3A:
- In 3A you gently closed the black ZIF tab by pressing down toward the probe.
- In 1.0 and 2.0, insert the probe and press the black tab back toward the headstage.


[ Calibration Files ]
Unlike phase3A probes, the later probes do not contain ADC and gain calibration tables in their EEPROM chips. Rather, for each probe, you need to find its matching folder of calibration data and copy it into your 'SpikeGLX/_Calibration' folder. A calibration folder is named with the serial number of its probe (as printed on the probe flex cable). These folders are available from whoever shipped you the probes.


-------------------------
Deployment / Installation
-------------------------

Basically, download a zip file and extract the whole folder 'Release_vXXX' to the C:\ drive.

However, we recommend that you manually create a folder on the C:\ drive -- 'C:\SpikeGLX\' as a master SpikeGLX folder that will hold one or more downloaded software versions. As many versions as desired can live side by side in the folder. Manually create a desktop shortcut pointing just to the version you currently like (usually the latest). The rationale for this is you can readily revert to an older version (just by changing the shortcut) should the latest one have a bug or an incompatibility with your current experiment setup.

So, your drive should resemble:

C:\SpikeGLX
  Release_v20221031-phase30
  Release_v20221012-phase30
  Release_v20200520-phase20
  Release_v20190413-phase3B1
  ...


---------------
Getting Started
---------------

You are urged to read the included 'UserManual.html' to familiarize yourself with some important concepts.

The other '.html' documents in the SpikeGLX/_Help folder add specialized discussion.

Help and How-To videos are on my downloads page: <https://billkarsh.github.io/SpikeGLX/>.


-----------------
Launching the App
-----------------

Double-click the SpikeGLX.exe (or SpikeGLX_NISIM.exe) icon...

Your package includes two application versions:

- SpikeGLX.exe:
This is the fully functional version. It can acquire data from imec and from nidq hardware. However, you have to install National Instruments NI-DAQmx driver software to run it. Otherwise, when you try to launch you'll get messages about one or more missing niXXX.dll components. This version is appropriate for the lab computer where you run experiments.

- SpikeGLX_NISIM.exe:
This is the 'simulated nidq' version. It can acquire real imec data, but it cannot talk to nidq hardware (nor is NI software needed). Rather, if you enable nidq data acquisition (using the Devices Tab of the Config dialog) it will generate simple sine waves on all of the analog channels in the nidq stream. Use this version if you only care about imec probes. Also use this version to open and review existing runs on a laptop where you likely don't have any hardware.


-------
License
-------

The Janelia Research Campus Software Copyright 1.2

Copyright (c) 2020, Howard Hughes Medical Institute, All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
    Neither the name of the Howard Hughes Medical Institute nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, ANY IMPLIED WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; REASONABLE ROYALTIES; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


QLed components are subject to GNU Library GPL v2.0 terms, which are described here:
<https://github.com/billkarsh/SpikeGLX/blob/master/QLed-LGPLv2-LICENSE.txt>


---------------
Release History
---------------

Changes in 20221031
-------------------
+ Fix imec sync SMA input.
+ Fix IMRO editor boxes selector bug.
+ Fix shankMap handling for NI files.
+ Improve Graphs window slider paging.
+ Add LFP filter to Graphs window.
+ Add FileViewer command to view meta file.
+ Add ~muxTbl entry to probe metadata.
+ Add sim slots.
+ Add sim probes.


Changes in 20221012-phase30
---------------------------
+ API 3.52 fixes newer HS detection issue.


Changes in 20220101-phase30
---------------------------
+ API 3.51.
+ Fix NI channel counting.
+ Fix NI AI buffer sizing.
+ Fix remote gate/trigger server.
+ LF metadata gets ~snsShankMap.
+ Enable HST for 1.0.
+ Update Par2 tool.
+ Write firstSample earlier in run.
+ LF FileViewer understands CatGT downsampled AP.
+ Improve NI analog input accuracy.
+ Improve BinMax speed.
+ Improve Configuration dialog UI.
+ Improve run startup dialog.
+ Improve graph label visibility.
+ Improve help.
+ Edit run notes from Graphs Window.
+ ShankViewers now floating windows.
+ FileViewer gets ShankViewer.
+ Add probe Survey mode.
+ Add graphical IMRO editing.
+ Add IMRO editing to online/offline ShankViewers
+ Add Onebox support.
+ Add MATLAB Onebox support.
+ Add MATLAB opto-probe support.
+ Add MATLAB probe param getters and setters.
+ Add MATLAB Onebox param getters and setters.
+ Simplify/optimize MATLAB remote interface.
+ Add C++ remote API.
+ Remote APIs now separate downloads.
+ MATLAB users must reinstall @SpikeGL and CalinsNetMex.mexw64.


Changes in 20201103-phase30
---------------------------
+ Make Update Firmware dialog more modal.
+ Improve lagging trigger error report.
+ Update docs.


Changes in 20201024-phase30
---------------------------
+ Improve firmware update reliability.
+ Update HST nonimplemented message.


Changes in 20201012-phase30
---------------------------
+ API 3.31.
+ Add support for NP1010 and NP1100 probes.
+ Improve text in firmware update dialog.
+ Improve text in BIST dialog.


Changes in 20200513-phase30
---------------------------
+ Initial NP 3.0.
+ Improve table editors.
+ Improve firmware update progress bar.
+ Fix crash when delete and re-add slots.
+ Fix "Sample Rates From Run."
+ Add new imro selectors.
+ Add FileViewer graph statistics.
+ FileViewer can open any 3A or later run.
+ ShankMap editor handles zero neural channels.
+ Change names of firmware files: "BS_FPGA_XXX, "QBSC_FPGA_XXX."


Changes in 20200309-phase20
---------------------------
+ Add multidisk run splitting.
+ Can now select/explore disk top level directory.
+ MATLAB better handling of Windows path strings.


Changes in 20191001-phase20
---------------------------
+ "Sample Rates From Run" now updates LF.
+ CAR filters applied whole-probe, not shank-by-shank.
+ Fix NI DI and PFI channel counting.
+ Fix ShankViewer crash if shift or right-click.
+ Fix linking of exported files in FileViewer.
+ Remove SY entry from exported snsShankMap.
+ Graphs Window default now user order.
+ Improve FileViewer slider tracking.
+ Improve short timespan graphing.


Changes in 20190919-phase20
---------------------------
+ API v2.11.
+ Fix SYNC, now fully functional.
+ Fix 4-shank tip and internal refs.
+ Fix calibration run file naming.
+ Fix select NI for sync if not enabled.
+ Fix ShankView LFP option.
+ Trigger context margin now half stream length.


Changes in 20190911-phase20
---------------------------
+ Invert sign of AP band data.
+ Fix 'AP + LF' bandpass filter in Graphs window.


Changes in 20190724-phase20
---------------------------
+ Initial NP 2.0.
+ BIST tests not implemented.
+ HST  tests not implemented.
+ HS has no EEPROM to remember its serial number.
+ IMRO editors load externally made maps.
+ Right-click-in-graph config editing has 60s delay.
+ Module SYNC connector is INPUT ONLY.


Changes in 20190413-phase3B2
----------------------------
+ Better parsing of file paths.
+ TTL settings now remember stream correctly.
+ Fix MATLAB SDK: DemoReadSGLXData.m/ChanGainsIM().
+ IMRO: Select block anywhere on shank.
+ NI supports 653X DIO devices.
+ Time trigger: negative L overlaps files.
+ Headstage tester (HST) now supported.


Changes in 20190327-phase3B2
----------------------------
+ Fix parsing of filenames with dots.
+ Fix MATLAB GetImProbeSN command.
+ Add MATLAB GetImProbeCount command.
+ Add MATLAB GetImVoltageRange command.
+ Add MATLAB MapSample command.
+ Add MATLAB file-writing beeps.
+ NI Clock Source dialog gets settle time variable.


Changes in 20190305-phase3B2
----------------------------
+ Fix negative g-index from Graphs window.
+ Fix max rate query for some NI devices.
+ Support up to 32 NI multifunction DI channels.
+ Time trigger realigns each file.
+ Optional confirmation on Disable Recording.
+ Right-click/Refresh Graphs command.
+ Add MATLAB SetNextFileName command.
+ Imec SMA always an input unless specifically set as sync output.


Changes in 20190214-phase3B2
----------------------------
+ Cleaner stops when disabling recording.
+ Optional LF-AP save-channel pairing.
+ Always save sync channel.
+ Smarter continuation run naming.
+ Folder per run.
+ Folder per probe option.


Changes in 20180829-phase3B1
----------------------------
+ Fix TTL trigger bug.
+ NI works with PXI, M, X devices.
+ Updates to MATLAB API.
+ Calibrated sample rates stored in _Calibration.
+ ImecProbeData folder renamed to _Calibration.
+ configs folder renamed to _Configs.
+ More accurate disk speed estimates.
+ Offline binMax stepping is selectable.
+ More efficient data acquisition.
+ Log reports caller CPU.
+ Better show/hide window behavior.
+ Fix typos in documentation.
+ Merge practice with 3B2 version.


Changes in 20180715-phase3B1
----------------------------
+ Introduce imec probe cal policy.
+ Fix test for existing run dir.
+ Fix test for existing run name.
+ Name NI devices anything you want.
+ Add MATLAB 'connectionClosed' warningid.
+ Add MATLAB query for imec probe SN/type.


Changes in 20180515-phase3B1
----------------------------
+ Fix FileViewer imec.lf crash.


Changes in 20180415-phase3B1
----------------------------
+ Graph Window stream selectors.
+ Fix FileViewer <T> filter.
+ Gate/Trigger server gets enable checkbox.
+ Brighter sweep cursor.
+ More accurate disk space calculator.
+ Update Qled components.
+ Update license notices.


Changes in 20180325-phase3B1
----------------------------
+ Fix file writing when gate auto-enabled at run start.
+ Fix innacurate Acquisition clock.
+ Fix imro per-channel AP filter (flip sense).
+ Fix imro gain 3000 setting.
+ Fix MATLAB SetParams, StartRun, GetRunName.
+ Fix MATLAB connection timeouts.
+ Improve graph filters.
+ FileViewer gets right-click shankMap editing.
+ Steadier view when changing Graphs:chans_per_page.
+ Enable Recording button shown by default.


Changes in 20180316-phase3B1
----------------------------
+ Fix problem triggering on imec without nidq.
+ Change imec yielding and error 15 handling.


Changes in 20170903-phase3B1
----------------------------
+ FVW fix ~TRG label.
+ FVW prettier graph resizing.
+ FVW selected span readout.
+ Graph labels easier to read.
+ Audio fix scaling bug.
+ Audio fix freezing bug.
+ Improve Par2 Cancel operation.
+ Config dialog checks ColorTTL settings.
+ Data fetching more efficient.
+ Enhanced error handling and logging.
+ New stream synchronization features.


Changes in 20170901-phase3A
---------------------------
+ Fix Imec FIFO queue filling problem.
+ Improve nidq dual-device settings logic.


Changes in 20170814
-------------------
+ Fix new bug configuring only nidq stream.


Changes in 20170808
-------------------
+ Fix bug in TTL trigger.
+ Begin hidden multistream work.


Changes in 20170724
-------------------
+ Improve look and abort function of run startup dialog.
+ Fix bug in Timmed trigger when gate is closed-reopened.
+ Fix bug setting audio hipass filter from MATLAB.
+ Add MATLAB SetMetaData function.


Changes in 20170501
-------------------
+ Audio output for either stream.
+ Smoother quitting of runs upon Imec errors.
+ Menu items to Explore run dir, app dir.
+ Force gets 'Explore ImecProbeData' button.
+ Force '11 -> 9' button sets probe option.
+ FileViewer gets channel submenus.
+ Add MATLAB GettingStarted.txt.
+ Improve 'Metadata.html' discussion.


Changes in 20170315
-------------------
+ Fix FileViewer bug drawing nidq aux channels.
+ Current <G T> file labels added to Graphs window.
+ Better graph label legibility.
+ Better BinMax (spike drawing) signal-to-noise.
+ Include 'Metadata.html' explaining tags.
+ Include MATLAB code for parsing output files.
+ Remember screen layout.


Changes in 20161201
-------------------
+ Prettier dialogs that resize:
+    Config, IM-Force, IMROTable, ShankMaps, ChanMaps,
+    CmdServer, RemoteServer, Help,
+    AO, FVWOptions, BIST, PAR2.
+ Add shank viewer help text.
+ Graphs get save and AO indicators.
+ Right-click graphs to edit sort order.
+ Right-click graphs to edit saved channels.
+ Right-click graphs to colorize TTL events.
+ Sort graphs along shank: forward or reverse order.
+ User notes field on Config::Save tab.
+ User notes in metadata and FileViewer.
+ FileViewer Zoom in/out key commands.
+ FileViewer Zoom by ctrl-click & drag.
+ FileViewer Set Export Range now shift-click & drag.
+ FileViewer gets option to manually update graphs.
+ Extend TTL triggering to imec and nidq digital lines.
+ Improved graph time axis accuracy.


Changes in 20161101
-------------------
+ Increase FVW YScale to 999.
+ Gray config items if stream disabled.
+ Improve ShankMap editors.
+ Add ShankMap viewers.


Changes in 20160806
-------------------
+ Fix enabled state of "Edit On/Off" R-click item.
+ Fix filters to obey order: Flt:<T>:<S>:BinMax.


Changes in 20160805
-------------------
+ More predictable file starts in Timed trigger mode.
+ Rename graphing option "-DC" to "-<T>".
+ Add -<T> option to online NI stream.
+ Add default ShankMap backing spatial channel averaging.
+ Add -<S> option to online streams.
+ Add -<S> option to offline viewers.
+ Split Config SNS tab into Map and Save tabs.
+ Add ShankMap file support.
+ Add ShankMap editors.
+ Can order ChanMap according to ShankMap.
+ Imec internal refs: UI to turn off option 3 chans.
+ Edit imro or on/off during run with R-click in graphs.
+ Make sure R-click changes update ShankMap.


Changes in 20160703
-------------------
+ Imec v4.3.
+ Fix spelling error.
+ Update doc texts.
+ Better ChanMap editor.
+ FVW can hide all.
+ FVW can edit ChanMap.
+ FVW better channel selection.
+ FVW fix export chanMap.
+ FVW fix export time span.


Changes in 20160701
-------------------
+ Clean up imec startup.
+ Fix AO initialization.
+ Correct MATLAB docs.
+ Better default server addresses.
+ Meta file written before and after.
+ Meta gets firstCt time stamp.
+ File viewer opens any stream type.
+ Better FVW UI for graph and time ranges.
+ Faster FVW param setters and dragging.
+ FVW linking.
+ FVW help text.


Changes in 20160601
-------------------
+ MATLAB: GetFileStartCount.
+ Fix TTL trigger bug.


Changes in 20160515
-------------------
+ Imec v4.2.
+ Add option to disable LEDs.
+ Imec diagnostics (BIST) dialog.
+ Update imec start sequence.
+ More AO filter options.
+ Fix AO deadlock.
+ Render digital channels.
+ Nicer default values.


Changes in 20160511
-------------------
+ Add BinMax checkbox.
+ Fix AO dialog issue.
+ Split imec data files.
+ Gate start waits for sample arrival.
+ Improve stream sync in all triggers.
+ Add recording time display.
+ Update help texts.


Changes in 20160502
-------------------
+ Fix bug in TTL trigger type.
+ Better handling of "?" button.
+ Add remote sort order query.


Changes in 20160501
-------------------
+ Extensive support is added for broken EEPROMs:
+ Calibrate ADC and gain from imec data files.
+ Optionally skip any calibrations if data unavailable.
+ Help text: click 'Help' in Force dialog...
+ Or read 'Force_Help.html' in release package.
+ More accurate time displays.


Changes in 20160408
-------------------
+ Imec support at v4.1
+ (ADC calib, optional gain calib, sync input).
+ Imec setup panel gets force identity button.


Changes in 20160404
-------------------
+ Imec support remains at v3.5 (ADC calibration).
+ Reenable spike and TTL triggers.
+ Recast trigger pause as gate override.
+ MATLAB: SetTrgEnable -> SetRecordingEnable.


Changes in 20160401
-------------------
+ Imec support at v3.5 (ADC calibration).
+ Detect if USB NI device connected.
+ Multiple SpikeGLX can run but one owns NI.
+ Errors bring console window to top.
+ Config dialog gets disk space calculator.


Changes in 20160305
-------------------
+ Fix timing issue with USB NI-DAQ.
+ Disable imec ref disconnect functions.
+ Flesh out SN field of imro table.


Changes in 20160304
-------------------
+ Imec support at v3.4 (faster startup, improved reference signal routing).
+ Fix run startup bug.
+ Fix DC subtraction bug.
+ Fix NI digital channel issue.
+ Message if set bank out of range.
+ Right-click to edit Imec settings.


Changes in 20160302
-------------------
+ v20160101 removed from site.
+ Fix bug setting bank 0.
+ Start including SpikeGLX-MATLAB-SDK in release.
+ Updated help text/UserManual with discussion of performance topics.


Changes in 20160301
-------------------
+ SpikeGLX folder requires "libNeuropix_basestation_api.dll".
+ Configuration dialog gets {Devices, IM Setup} Tabs.
+ Updated run pause/resume options moved from Triggers Tab to Gates Tab.
+ Data are processed in two parallel "streams": imec stream, nidq stream.
+ Each stream generates own datafiles {.imec.bin, .imec.meta} {.nidq.bin, .nidq.meta}.
+ Some renaming of metadata items to distinguish stream type.
+ Each stream gets own channel map and own save-channel string.
+ Each stream gets own Graphs window viewer pane.
+ All graphing uses pages of stacked traces.
+ New Graphs window Run Toolbar layout.
+ NI viewer gets low-pass filter option.
+ MATLAB API modified to fetch from either imec or nidq stream.
+ Online help text rewritten.
+ Includes SpikeGLX.exe:       Real Imec and NI-DAQ acquisition.
+ Includes SpikeGLX_NISIM.exe: Real Imec and simulated NI-DAQ acquisition.


Temporarily disabled:
- Spike trigger option.
- TTL trigger option.
- AO for Imec channels.
- Offline file viewer for Imec files.
- Graphs window save-channel checkboxes.


Changes in 20160101
-------------------
- Fix bug: SpikeGLX hangs on remote setTrgEnable command.
- Fix bug: Remote setRunName doesn't handle _g_t tags correctly.
- Data files now get double extension '.nidq.bin', '.nidq.meta'.
- Offline file viewer enabled.


Original 20151231
-----------------
- Function set very close to original SpikeGL.



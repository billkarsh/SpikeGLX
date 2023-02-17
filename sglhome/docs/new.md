# What's New On The Site?

February 17, 2023

* CatGT 3.6 (Selective channel saving, median CAR, better g-series).

February 9, 2023

* Update PXI requirements.
* Update SpikeGLX_Datafile_Tools (fix how Python gets 2.0 probe gains).

February 5, 2023

* TPrime 1.8 (Add -offsets option).

February 2, 2023

* SpikeGLX 20230120-phase30 (Single-threaded probe programming).

January 20, 2023

* Updated SpikeGLX 20230101-phase30 with correct DLLs.

January 19, 2023

* SpikeGLX 20230101-phase30 (Fix TTL trigger, add UHD3 probes).
* Last minute API update to support probe NP1015.

January 18, 2023

* SpikeGLX 20221220-phase30 (Fix TTL trigger, add UHD3 probes).
* CatGT 3.5 (Support latest probes).
* C_Waves 2.1 (Support latest probes).

December 22, 2022

* Add survey mode video.

December 15, 2022

* SpikeGLX 20221212-phase30 (Fix passive UHD probe editing).
* Add IMRO editor video.

December 12, 2022

* SpikeGLX 20221031-phase30 (Fix imec sync SMA input).

October 12, 2022

* SpikeGLX 20221012-phase30 (Fix HS detection issue in 20220101).
* CatGT 3.4 (Fix -gfix on save channel subsets, add -startsecs).
* Update SpikeGLX help.

September 13, 2022

* SpikeGLX 20220101-phase30 (All new for 22!).

July 14, 2022

* CatGT 3.3 (Fix -maxsecs option).

July 1, 2022

* CatGT 3.2 (AP -> LF for 1.0 data).

June 27, 2022

* NIScaler 1.1 (Fix rollover of saturation voltage).

June 20, 2022

* CatGT 3.1 (Fix loccar channel exclusion).
* C_Waves 2.0 (Fix SNR channel exclusion).

June 4, 2022

* Add NIScaler to postprocessing tools.

May 18, 2022

* Update UserManual.
* Add description of remote control SDKs.

May 2, 2022

* Update help discussions of CatGT and Sync.
* Post SpikeGLX_QuickRef under Help menus.

April 21, 2022

* CatGT 3.0 (Onebox, new extractors, auto sync, fyi).

March 18, 2022

* New SGLXMetaToCoods.zip (Support more probes, fix ref channel ID).

March 3, 2022

* C_Waves 1.9 (Support UHD2 and current probes).

February 23, 2022

* CatGT 2.5 (Add concatenation offsets files).

February 17, 2022

* Post firmware making basestations compatible with X1 chassis slots.

February 2, 2022

* Publish {CatGT, TPrime, C_Waves} source code on github.

November 24, 2021

* CatGT 2.4 (Add option -gtlist).

October 12, 2021

* C_Waves 1.8 (Code same, update: ReadMe, runit script).

October 1, 2021

* CatGT 2.3 (pass1_force_ni_bin, fix supercat param dependency).

September 15, 2021

* CatGT 2.2 (Acausal Butterworth filter).
* Updated site help on CatGT.

July 29, 2021

* CatGT 2.1 (BF gets inarow parameter).

July 20, 2021

* CatGT 2.0 (XA seeks threshold-2 even if millisecs=0).

June 28, 2021

* TPrime 1.7 (Fix end-of-file parsing).

June 13, 2021

* SpikeGLX_Datafile_Tools updated for 2.0 probes.
* CatGT 1.9:
    - Fix link to fftw3 library.
    - Remove glitch at tshift block boundaries.
    - Option -gfix now exploits -tshift.
    - Option -chnexcl now specified per probe.
    - Option -chnexcl now modifies shankMap in output metadata.
    - Stream option -lf creates .lf. from .ap. for 2.0 probes.
    - Fix supercat premature completion bug.
    - Supercat observes -exported option.
    - Pass1 always writes new meta files for later supercat.
    - Add option -supercat_trim_edges.
    - Add option -supercat_skip_ni_bin.
    - Add option -maxsecs.
    - Add option -BF (bit-field decoder).

April 26, 2021

* CatGT 1.8 (Olivier Winter subsample time shifting).

March 18, 2021

* CatGT 1.7 (Suppress linux brace expansion).

March 15, 2021

* TPrime 1.6 (Fix potential crash).

February 12, 2021

* CatGT 1.6 (Fix g-series concatenation).

February 10, 2021

* CatGT 1.5 (supercat feature, better scripts).
* TPrime 1.5 (Better scripts).
* C_Waves 1.8 (Better scripts).

January 26, 2021

* CatGT 1.4.2 (Fix zerofillmax, add option -inarow to extractors).

January 22, 2021

* SpikeGLX 20201103-phase30 (Fix firmware update dialog).

January 16, 2021

* {CatGT, TPrime, C_Waves}: Linux package reposted with corrected runit.sh.

January 15, 2021

* {CatGT, TPrime, C_Waves}: Windows package reposted with corrected runit.bat.

January 14, 2021

* CatGT 1.4.1 (Linux, g-range, inverted extractors, flexible working dir).
* TPrime 1.4 (Linux, x64 headers, handle Fortran order, flexible working dir).
* C_Waves 1.7 (Linux, better SNR, flexible working dir).

January 11, 2021

* CatGT 1.4.0 (Linux, g-range, inverted extractors).
* TPrime 1.3 (Linux, x64 headers, handle Fortran order).
* C_Waves 1.6 (Linux, better SNR).
* Warn imec system not compatible with AMD computers.

November 05, 2020:

* C_Waves 1.5 (Smoother mean waveforms).
* Link to Metadata_30.md.

November 03, 2020:

* SpikeGLX 20201024-phase30 (Fix firmware updating).

October 24, 2020:

* SpikeGLX 20201012-phase30 (Support NP1010 and NP1100 probes).
* CatGT 1.3.0 (Support NP1010 probe).
* C_Waves 1.4 (Support NP1010 probe).

September 25, 2020:

* Release SpikeGLX 3.0.
* Retire SpikeGLX 1.0 and 2.0.
* C_Waves 1.3 (Supports FORTRAN order npy).

August 20, 2020:

* New document: help/"IMRO Table Anatomy."

August 13, 2020:

* Disable 2.0 download temporarily.

August 4, 2020:

* Fix links.

July 16, 2020:

* SpikeGLX 20200520-phase3B2 (Supports UHD-1, NHP, Imec v1.20).
* SpikeGLX 20200520-phase20  (New 2.0 probe names, Imec v2.14).
* CatGT 1.2.9 (Supports UHD-1, NHP).
* C_Waves 1.2 (Supports UHD-1, NHP).
* Rename module firmware files.

June 1, 2020:

* Add module firmware to Beta page.

May 12, 2020:

* SpikeGLX 20200309 (PXI versions get multidrive run splitting).
* CatGT 1.2.8 (Option -prb_miss_ok addded for multidrive output).
* New document: help/"Parsing Data Files."

April 3, 2020:

* CatGT 1.2.7 (Extractors get optional pulse duration tolerance).
* New document: help/"Noise: Learn How To Solder."

March 26, 2020:

* C_Waves 1.1 (Removes 4GB binary file size limit).

March 10, 2020:

* New SpikeGLX version (all phases).
* C_Waves 1.0 post-processing pipeline component added.

March 06, 2020:

* CatGT 1.2.6.
* TPrime 1.2.
* New versions: Post-processing Tools.
* Add site menu: "Help."
* Add site menu: "New."
* New document: help/"Sync: Aligning with Edges."


_fin_


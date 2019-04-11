% SYNOPSIS
% --------
%
% The @SpikeGL class is a MATLAB object with methods to access the
% SpikeGLX program via TCP/IP. SpikeGLX and MATLAB can run on the
% same machine (via loopback socket address 127.0.0.1 and port 4142)
% or across a network.
%
% This class provides extensive control over a running SpikeGLX process:
% starting and stopping a run, setting parameters, calling the Par2 and
% SHA1 tools, and so on.
%
% Users of this class merely need to construct an instance of a @SpikeGL
% object and all network communication with the SpikeGLX process is handled
% automatically.
%
% The network socket handle is used with the 'CalinsNetMex' mexFunction,
% which is a helper mexFunction that does all the actual socket
% communications for this class (since MATLAB lacks native network
% support).
%
% Instances of @SpikeGL are weakly stateful: merely keeping a handle to a
% network socket. It is ok to create and destroy several of these objects.
% Each network connection cleans up after itself after 10 seconds of
% inactivity. By the way, if your script has pauses longer than 10 seconds,
% and you reuse a handle that has timed out, the handle will automatically
% reestablish a connection and the script will likely continue without
% problems, but a warning will appear in the Command Window refelecting
% the timeout. Such warnings have a warningid, so you can suppress them
% by typing >> warning( 'off', 'CalinsNetMex:connectionClosed' ).
%
%
% EXAMPLES
% --------
%
% my_s = SpikeGL;   % connect to SpikeGLX running on local machine
%
% prms = GetParams( my_s ); % retrieve run params
%
% SetParams( my_s, struct('niMNChans1','0:5','niDualDevMode','false',...) );
%
% StartRun( my_s ); % starts data acquisition run using last-set params
%
% StopRun( my_s );  % stop run and clean up
%
% StreamID
% --------
%
% Several functions work for either the NI data stream or for any of the
% N enabled IMEC probe streams. StreamID = -1 selects NI. Values in the
% range [0,..,N-1] select an IMEC stream.
%
% FUNCTION REFERENCE
% ------------------
%
%     myobj = SpikeGL()
%     myobj = SpikeGL( host )
%     myobj = SpikeGL( host, port )
%
%                Construct a new @SpikeGL instance and immediately attempt
%                a network connection. If omitted, the defaults for host and
%                port are {'localhost, 4142}.
%
%    myobj = Close( myobj )
%
%                Close the network connection to SpikeGLX and release
%                associated MATLAB resources.
%
%    myobj = ConsoleHide( myobj )
%
%                Hide SpikeGLX console window to reduce screen clutter.
%
%    myobj = ConsoleShow( myobj )
%
%                Show the SpikeGLX console window.
%
%    params = EnumDataDir( myobj )
%
%                Retrieve a listing of files in the data directory.
%
%    [daqData,headCt] = Fetch( myObj, streamID, start_scan, scan_ct, channel_subset, downsample_ratio )
%
%                Get MxN matrix of stream data.
%                M = scan_ct = max samples to fetch.
%                N = channel count...
%                    If channel_subset is not specified, N = current
%                    SpikeGLX save-channel subset.
%                Fetching starts at index start_scan.
%                Data are int16 type.
%
%                downsample_ratio is an integer (default = 1).
%
%                Also returns headCt = index of first timepoint in matrix.
%
%    [daqData,headCt] = FetchLatest( myObj, streamID, scan_ct, channel_subset, downsample_ratio )
%
%                Get MxN matrix of the most recent stream data.
%                M = scan_ct = max samples to fetch.
%                N = channel count...
%                    If channel_subset is not specified, N = current
%                    SpikeGLX save-channel subset.
%
%                downsample_ratio is an integer (default = 1).
%
%                Also returns headCt = index of first timepoint in matrix.
%
%    chanCounts = GetAcqChanCounts( myobj, streamID )
%
%                Returns a vector containing the counts of 16-bit
%                words of each class being acquired.
%
%                streamID = -1: NI channels: {MN,MA,XA,DW}.
%                streamID >= 0: IM channels: {AP,LF,SY}.
%
%    dir = GetDataDir( myobj )
%
%                Get global run data directory.
%
%    startCt = GetFileStartCount( myobj, streamID )
%
%                Returns index of first scan in latest file,
%                or zero if not available.
%
%    n_probes = GetImProbeCount( myobj )
%
%                Returns count of enabled IMEC probes.
%
%    [SN,type] = GetImProbeSN( myobj, streamID )
%
%                Returns serial number string (SN) and integer type
%                of selected IMEC probe.
%
%    [Vmin,Vmax] = GetImVoltageRange( myobj, streamID )
%
%                Returns votlage range of selected IMEC probe.
%
%    params = GetParams( myobj )
%
%                Get the most recently used run parameters.
%                These are a struct of name/value pairs.
%
%    name = GetRunName( myobj )
%
%                Get run base name.
%
%    sampleRate = GetSampleRate( myobj, streamID )
%
%                Returns sample rate of selected stream in Hz,
%                or zero if not enabled.
%
%    channelSubset = GetSaveChans( myobj, streamID )
%
%                Returns a vector containing the indices of
%                channels being saved.
%
%    scanCount = GetScanCount( myobj, streamID )
%
%                Returns number of scans since current run started
%                or zero if not running.
%
%    time = GetTime( myobj )
%
%                Returns (double) number of seconds since SpikeGLX application
%                was launched.
%
%    version = GetVersion( myobj )
%
%                Get SpikeGLX version string.
%
%    boolval = IsConsoleHidden( myobj )
%
%                Returns 1 if console window is hidden, false otherwise.
%                The console window may be hidden/shown using ConsoleHide()
%                and ConsoleUnhide().
%
%    boolval = IsInitialized( myobj )
%
%                Return 1 if SpikeGLX has completed its startup
%                initialization and is ready to run.
%
%    boolval = IsRunning( myobj )
%
%                Returns 1 if SpikeGLX is currently acquiring data.
%
%    boolval = IsSaving( myobj )
%
%                Returns 1 if the software is currently running
%                AND saving data.
%
%    boolval = IsUserOrder( myobj, streamID )
%
%                Returns 1 if graphs currently sorted in user order.
%
%                This query is sent only to the main Graphs window.
%
%    dstSample = MapSample( myobj, dstStream, srcSample, srcStream )
%
%                Returns sample in dst stream corresponding to
%                given sample in src stream.
%
%    res = Par2( myobj, op, filename )
%
%                Create, Verify, or Repair Par2 redundancy files for
%                'filename'. Arguments:
%
%                op: a string that is either 'c', 'v', or 'r' for create,
%                verify or repair respectively.
%
%                filename: the .par2 or .bin file to which 'op' is appled.
%
%                Progress is reported to the command window.
%
%    myobj = SetAudioEnable( myobj, bool_flag )
%
%                Set audio output on/off. Note that this command has
%                no effect if not currently running.
%
%    myobj = SetAudioParams( myobj, group_string, params_struct )
%
%                Set subgroup of parameters for audio-out operation. Parameters
%                are a struct of name/value pairs. This call stops current output.
%                Call SetAudioEnable( myobj, 1 ) to restart it.
%
%    myobj = SetDataDir( myobj, dir )
%
%                Set global run data directory.
%
%    myobj = SetDigOut( myobj, bool_flag, channels )
%
%                Set digital output on/off. Channel strings have form:
%                'Dev6/port0/line2,Dev6/port0/line5'.
%
%    myobj = SetMetaData( myobj, metadata_struct )
%
%                If a run is in progress, set meta data to be added to the
%                next output file set. Meta data must be in the form of a
%                struct of name/value pairs.
%
%    myobj = SetNextFileName( myobj, 'name' )
%
%                If a run is in progress, set the full path and run name
%                for the next file set. For example, "Y:/mydir/../myrun."
%                To that, SpikeGLX appends the stream and extension, for
%                example, "Y:/mydir/../myrun.imec0.ap.bin." SpikeGLX will
%                not create any folders or add any _g_t tags, so the name
%                is under full user control. SpikeGLX clears your custom
%                name and reverts to auto-naming after the current files
%                are written. You must, therefore, call this function for
%                each file, set and you must set the name before a trigger
%                event for that file set.
%
%    myobj = SetParams( myobj, params_struct )
%
%                The inverse of GetParams.m, this sets run parameters.
%                Alternatively, you can pass the parameters to StartRun()
%                which calls this in turn. Run parameters are a struct of
%                name/value pairs. The call will fail with an error if a
%                run is currently in progress.
%
%    myobj = SetRecordingEnable( myobj, bool_flag )
%
%                Set triggering (file writing) on/off for current run.
%                This command has no effect if not running.
%
%    myobj = SetRunName( myobj, 'name' )
%
%                Set the run name for the next time files are created
%                (either by SetTrgEnable() or by StartRun()).
%
%    myobj = StartRun( myobj )
%    myobj = StartRun( myobj, params )
%    myobj = StartRun( myobj, runName )
%
%                Start data acquisition run. Optional second argument (params)
%                is a struct of name/value pairs as returned from GetParams.m.
%                Alternatively, the second argument can be a string (runName).
%                Last-used parameters remain in effect if not specified here.
%                An error is flagged if already running or a parameter is bad.
%
%    myobj = StopRun( myobj )
%
%                Unconditionally stop current run, close data files
%                and return to idle state.
%
%    res = VerifySha1( myobj, filename )
%
%                Verifies the SHA1 sum of the file specified by filename.
%                If filename is relative, it is appended to the run dir.
%                Absolute path/filenames are also supported. Since this is
%                a potentially long operation, it uses the 'disp' command
%                to print progress information to the MATLAB console. The
%                returned value is 1 if verified, 0 otherwise.


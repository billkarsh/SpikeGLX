% SYNOPSIS
% --------
%
% The @SpikeGL class is a MATLAB object with methods to access the
% SpikeGLX program via TCP/IP. SpikeGLX and MATLAB can run on the
% same machine (transparently via a loopback socket) or across a
% network.
%
% This class provides nearly total control over a running SpikeGLX
% process -- starting and stopping a run, setting parameters, calling
% the Par2 and SHA1 tools, and so on.
%
% Instances of @SpikeGL are weakly stateful: merely keeping a handle to a
% network socket. As such, it is ok to constantly create and destroy these
% objects. Each network connection cleans up after itself after 10 seconds
% of inactivity.
%
% The network socket handle is used with the 'CalinsNetMex' mexFunction,
% which is a helper mexFunction that does all the actual socket
% communications for this class (since MATLAB lacks native network
% support).
%
% Users of this class merely need to construct an instance of a @SpikeGL
% object and all network communication with the SpikeGLX process is handled
% automatically.
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
%    params = EnumRunDir( myobj )
%
%                Retrieve a listing of files in the run directory.
%
%    chanCounts = GetAcqChanCounts( myobj )
%
%                Returns a vector containing the counts of 16-bit
%                words of each class being acquired {AP,LF,SY,MN,MA,XA,DW}.
%
%    channelSubset = GetSaveChansIm( myobj ),
%                    GetSaveChansNi( myobj )
%
%                Returns a vector containing the indices of
%                channels being saved.
%
%    [daqData,headCt] = FetchIm( myObj, start_scan, scan_ct, channel_subset, downsample_factor ),
%                       FetchNi( myObj, start_scan, scan_ct, channel_subset, downsample_factor )
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
%    [daqData,headCt] = FetchLatestIm( myObj, NUM, channel_subset, downsample_ratio ),
%                       FetchLatestNi( myObj, NUM, channel_subset, downsample_ratio )
%
%                Get MxN matrix of the most recent stream data.
%                M = NUM = max samples to fetch.
%                N = channel count...
%                    If channel_subset is not specified, N = current
%                    SpikeGLX save-channel subset.
%
%                downsample_ratio is an integer (default = 1).
%
%                Also returns headCt = index of first timepoint in matrix.
%
%    params = GetParams( myobj )
%
%                Get the most recently used run parameters.
%                These are a struct of name/value pairs.
%
%    dir = GetRunDir( myobj )
%
%                Get global run data directory.
%
%    name = GetRunName( myobj )
%
%                Get run base name.
%
%    scanCount = GetScanCountIm( myobj ),
%                GetScanCountNi( myobj )
%
%                Returns number of scans since current run started
%                or zero if not running.
%
%    startCt = GetFileStartCountIm( myobj ),
%              GetFileStartCountNi( myobj )
%
%                Returns index of first scan in latest file,
%                or zero if not available.
%
%    time = GetTime( myobj )
%
%                Returns (double) number of seconds since SpikeGLX application
%                was launched. This queries the high precision timer, so has
%                better than microsecond resolution.
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
%    boolval = IsUserOrderIm( myobj ),
%              IsUserOrderNi( myobj )
%
%                Returns 1 if graphs currently sorted in user order.
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
%    myobj = SetAOEnable( myobj, bool_flag )
%
%                Set analog output on/off. Note that this command has
%                no effect if not currently running.
%
%    myobj = SetAOParams( myobj, params_struct )
%
%                Set parameters for analog-out operation. Parameters are
%                a struct of name/value pairs. This call stops current AO.
%                Call SetAOEnable( myobj, 1 ) to restart it.
%
%    myobj = SetDigOut( myobj, bool_flag, channel )
%
%                Set digital output on/off. Channel strings have form:
%                'Dev6/port0/line2'.
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
%    myobj = SetRunDir( myobj, dir )
%
%                Set global run data directory.
%
%    myobj = SetRunName( myobj, 'name' )
%
%                Set the run name for the next time files are created
%                (either by SetTrgEnable() or by StartRun()).
%
%    myobj = StartRun( myobj)
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
%                If filename is relative, it is interpreted as being
%                relative to the run dir. Absolute filenames (starting
%                with a '/') are supported as well. Since this is a long
%                operation, this functions uses the 'disp' command to print
%                progress information to the MATLAB console. The returned
%                value is 1 if verified, 0 otherwise.

% -----------------------------------------
% Variety of remote calls with timing stats
% -----------------------------------------
%
function testSpikeGL

% Create connection (edit the IP address)
hSGL = SpikeGL('10.101.20.29');

% Don't call repeatedly if not running
if IsRunning(hSGL)
    numCalls = 100;
else
    numCalls = 1;
end

% Init the timing measurements
getTimes = zeros(numCalls, 1);

% -----------------------------------------------
% Init parameters outside timing loop (as needed)
% -----------------------------------------------

% subset = GetSaveChansIm(hSGL);
% subset = subset(1:64);
% chans  = [10,66,192,193,194,195];

% ------------------------
% Start of the timing loop
% ------------------------

for i=1:numCalls

% ---------------------
% tic = start the clock
% ---------------------

    t0 = tic;

% ----------------------
% Demo setting AO params
% ----------------------

%     prm = struct();
%     prm.bandPass = 'true';
%     prm.autoStart = 'true';
%     prm.device = 'Dev1';
%     prm.rangeMin = -5;
%     prm.rangeMax = 5;
%     prm.clock = 'PFI2';
%     prm.channels = '0=192';
%     prm.volume = 2;
%     SetAOParams(hSGL, prm);
%     SetAOEnable(hSGL, 0);
%     SetTrgEnable(hSGL, 1);

% -------------------------------
% Variety of set/get calls to try
% -------------------------------

%     param = GetParams(hSGL);
%     SetParams(hSGL, param);

%     count = GetScanCountIm(hSGL)
%     count = GetScanCountNi(hSGL)
%     file = GetRunDir(hSGL)
%     IsConsoleHidden(hSGL)
%     ConsoleShow(hSGL);
%     EnumRunDir(hSGL)
%     Par2(hSGL, 'v', 'C:/SGL_DATA/myRun_g0_t0.nidq.bin');
%     VerifySha1(hSGL, 'C:/SGL_DATA/myRun_g0_t0.nidq.bin')
%     StopRun(hSGL);
%     StartRun(hSGL);
%     SetDigOut(hSGL, 0, 'Dev6/port0/line0')
%     SetRunName(hSGL, 'myRun_g5_t5');
%     SetTrgEnable(hSGL, 1);
%     IsSaving(hSGL)
%     file = EnumRunDir(hSGL)
%     a = GetSaveChansIm(hSGL)
%     a = GetSaveChansNi(hSGL)
%     a = GetAcqChanCounts(hSGL)

% -----------------------
% Fetch data for graphing
% -----------------------

%     mat = FetchLatestIm(hSGL, 2000);

% --------------------
% toc = stop the clock
% --------------------

    getTimes(i) = toc(t0);

% ---------------
% Progress report
% ---------------

    if mod(i, 50) == 0
        fprintf('Completed %d calls...\n', i);
    end

% -----------------------
% Graph fetched data here
% -----------------------

%     showdata(mat);

end % timing loop

% ------------
% Timing stats
% ------------

fprintf('GetParams time -- mean: %g ms\tstd: %g ms\n', ...
    1000*mean(getTimes), 1000*std(getTimes));

end % testSpikeGL


% ----------------------
% Tiny graphing function
% ----------------------
%
function showdata(mat)
    x = 1:2000;
    y = mat(:,1);
    figure(1);
    % set(gcf, 'doublebuffer', 'on');
    p = plot(x, y);
    set(p, 'XData', x, 'YData', y);
    drawnow;
end % showdata



% scanCount = GetScanCountNi( myobj )
%
%     Returns number of NI-DAQ scans since current run started
%     or zero if not running.
%
function [ret] = GetScanCountNi( s )

    ret = str2double( DoQueryCmd( s, 'GETSCANCOUNTNI' ) );
end

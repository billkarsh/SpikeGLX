% scanCount = GetScanCount( myobj )
%
%     Returns number of scans since current run started
%     or zero if not running.
%
function [ret] = GetScanCount( s )

    ret = str2double( DoQueryCmd( s, 'GETSCANCOUNT' ) );
end

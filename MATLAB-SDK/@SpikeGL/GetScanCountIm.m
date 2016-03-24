% scanCount = GetScanCountIm( myobj )
%
%     Returns number of IMEC scans since current run started
%     or zero if not running.
%
function [ret] = GetScanCountIm( s )

    ret = str2double( DoQueryCmd( s, 'GETSCANCOUNTIM' ) );
end

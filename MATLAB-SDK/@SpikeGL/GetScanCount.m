% scanCount = GetScanCount( myobj, streamID )
%
%     Returns number of scans since current run started
%     or zero if not running.
%
function [ret] = GetScanCount( s, streamID )

    ret = str2double( DoQueryCmd( s, sprintf( 'GETSCANCOUNT %d', streamID ) ) );
end

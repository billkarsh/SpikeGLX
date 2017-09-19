% scanCount = GetScanCountIm( myobj, streamID )
%
%     Returns number of IMEC scans since current run started
%     or zero if not running.
%
function [ret] = GetScanCountIm( s, streamID )

    ret = str2double( DoQueryCmd( s, sprintf( 'GETSCANCOUNTIM %d', streamID ) ) );
end

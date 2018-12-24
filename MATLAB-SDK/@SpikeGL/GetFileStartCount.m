% startCt = GetFileStartCount( myobj, streamID )
%
%     Returns index of first scan in latest file,
%     or zero if not available.
%
function [ret] = GetFileStartCount( s, streamID )

    ret = str2double( DoQueryCmd( s, sprintf( 'GETFILESTART %d', streamID ) ) );
end

% startCt = GetFileStartCountIm( myobj, streamID )
%
%     Returns index of first IMEC scan in latest file,
%     or zero if not available.
%
function [ret] = GetFileStartCountIm( s, streamID )

    ret = str2double( DoQueryCmd( s, sprintf( 'GETFILESTARTIM %d', streamID ) ) );
end

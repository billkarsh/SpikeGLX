% startCt = GetFileStartCountIm( myobj )
%
%     Returns index of first IMEC scan in latest file,
%     or zero if not available.
%
function [ret] = GetFileStartCountIm( s )

    ret = str2double( DoQueryCmd( s, 'GETFILESTARTIM' ) );
end

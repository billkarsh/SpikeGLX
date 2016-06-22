% startCt = GetFileStartCountNi( myobj )
%
%     Returns index of first NI-DAQ scan in latest file,
%     or zero if not available.
%
function [ret] = GetFileStartCountNi( s )

    ret = str2double( DoQueryCmd( s, 'GETFILESTARTNI' ) );
end

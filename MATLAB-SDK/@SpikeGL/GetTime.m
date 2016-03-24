% time = GetTime( myobj )
%
%     Returns (double) number of seconds since SpikeGLX application
%     was launched. This queries the high precision timer, so has
%     better than microsecond resolution.
%
function [ret] = GetTime( s )

    ret = sscanf( DoQueryCmd( s, 'GETTIME' ), '%g' );
end

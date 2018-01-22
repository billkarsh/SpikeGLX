% time = GetTime( myobj )
%
%     Returns (double) number of seconds since SpikeGLX application
%     was launched.
%
function [ret] = GetTime( s )

    ret = sscanf( DoQueryCmd( s, 'GETTIME' ), '%g' );
end

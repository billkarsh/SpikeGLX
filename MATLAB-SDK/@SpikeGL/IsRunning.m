% boolval = IsRunning( myobj )
%
%     Returns 1 if SpikeGLX is currently acquiring data.
%
function [ret] = IsRunning( s )

    ret = sscanf( DoQueryCmd( s, 'ISRUNNING' ), '%d' );
end

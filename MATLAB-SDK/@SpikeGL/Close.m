% myobj = Close( myobj )
%
%     Close the network connection to SpikeGLX and release
%     associated MATLAB resources.
%
function [s] = Close( s )

    CalinsNetMex( 'disconnect', s.handle );
    CalinsNetMex( 'destroy', s.handle );
    s.handle = -1;
end

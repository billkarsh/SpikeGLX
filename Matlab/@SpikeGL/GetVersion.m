% version = GetVersion( myobj )
%
%     Get SpikeGLX version string.
%
function [ret] = GetVersion( s )

    ret = DoQueryCmd( s, 'GETVERSION' );
end

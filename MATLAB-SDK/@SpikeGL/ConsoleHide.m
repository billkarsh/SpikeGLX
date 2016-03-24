% myobj = ConsoleHide( myobj )
%
%     Hide SpikeGLX console window to reduce screen clutter.
%
function [s] = ConsoleHide( s )

    s = DoSimpleCmd( s, 'CONSOLEHIDE' );
end

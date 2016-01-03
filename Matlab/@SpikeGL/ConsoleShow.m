% myobj = ConsoleShow( myobj )
%
%     Show SpikeGLX console window.
%
function [s] = ConsoleShow( s )

    s = DoSimpleCmd( s, 'CONSOLESHOW' );
end

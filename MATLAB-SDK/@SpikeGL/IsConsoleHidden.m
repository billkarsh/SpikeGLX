% boolval = IsConsoleHidden( myobj )
%
%     Returns 1 if console window is hidden, false otherwise.
%     The console window may be hidden/shown using ConsoleHide()
%     and ConsoleUnhide().
%
function [ret] = IsConsoleHidden( s )

    ret = sscanf( DoQueryCmd( s, 'ISCONSOLEHIDDEN' ), '%d' );
end

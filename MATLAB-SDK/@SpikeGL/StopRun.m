% myobj = StopRun( myobj )
%
%     Unconditionally stop current run, close data files
%     and return to idle state.
%
function [s] = StopRun( s )

    DoSimpleCmd( s, 'STOPRUN' );
end

% myobj = SetRecordingEnable( myobj, bool_flag )
%
%     Set triggering (file writing) on/off for current run.
%     This command has no effect if not running.
%
function [s] = SetRecordingEnable( s, b )

    if( ~isnumeric( b ) )
        error( 'SetRecordingEnable arg 2 should be a Boolean value {0,1}.' );
    end

    if( ~IsRunning( s ) )
        warning( 'Not running, SetRecordingEnable command ignored.' );
        return;
    end

    DoSimpleCmd( s, sprintf( 'SETRECORDENAB %d', b ) );
end

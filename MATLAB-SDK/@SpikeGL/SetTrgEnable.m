% myobj = SetTrgEnable( myobj, bool_flag )
%
%     Set triggering (file writing) on/off for current run.
%     This command has no effect if not running.
%
function [s] = SetTrgEnable( s, b )

    if( ~isnumeric( b ) )
        error( 'SetTrgEnable arg 2 should be a Boolean value {0,1}.' );
    end

    if( ~IsRunning( s ) )
        warning( 'Not running, SetTrgEnable command ignored.' );
        return;
    end

    DoSimpleCmd( s, sprintf( 'SETTRGENAB %d', b ) );
end

% myobj = SetAOEnable( myobj, bool_flag )
%
%     Set analog output on/off. Note that this command has
%     no effect if not currently running.
%
function [s] = SetAOEnable( s, b )

    if( ~isnumeric( b ) )
        error( 'SetAOEnable arg 2 should be a Boolean value {0,1}.' );
    end

    if( ~IsRunning( s ) )
        warning( 'Not running, SetAOEnable command ignored.' );
        return;
    end

    DoSimpleCmd( s, sprintf( 'SETAOENABLE %d', b ) );
end

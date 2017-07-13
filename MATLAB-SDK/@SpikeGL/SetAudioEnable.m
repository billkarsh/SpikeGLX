% myobj = SetAudioEnable( myobj, bool_flag )
%
%     Set audio output on/off. Note that this command has
%     no effect if not currently running.
%
function [s] = SetAudioEnable( s, b )

    if( ~isnumeric( b ) )
        error( 'SetAudioEnable arg 2 should be a Boolean value {0,1}.' );
    end

    if( ~IsRunning( s ) )
        warning( 'Not running, SetAudioEnable command ignored.' );
        return;
    end

    DoSimpleCmd( s, sprintf( 'SETAUDIOENABLE %d', b ) );
end

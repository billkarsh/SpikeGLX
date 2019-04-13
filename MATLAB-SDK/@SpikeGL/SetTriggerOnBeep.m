% myobj = SetTriggerOnBeep( myobj, hertz, millisec )
%
%     Set frequency and duration of Windows beep signalling
%     file creation. hertz=0 disables the beep. Command has
%     no effect if not currently running.
%
function [s] = SetTriggerOnBeep( s, hertz, millisec )

    if( ~IsRunning( s ) )
        warning( 'Not running, SetTriggerOnBeep command ignored.' );
        return;
    end

    DoSimpleCmd( s, sprintf( 'SETTRIGGERONBEEP %d %d', hertz, millisec ) );
end

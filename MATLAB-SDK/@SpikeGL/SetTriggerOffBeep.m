% myobj = SetTriggerOffBeep( myobj, hertz, millisec )
%
%     Set frequency and duration of Windows beep signalling
%     file closure. hertz=0 disables the beep. Command has
%     no effect if not currently running.
%
function [s] = SetTriggerOffBeep( s, hertz, millisec )

    if( ~IsRunning( s ) )
        warning( 'Not running, SetTriggerOffBeep command ignored.' );
        return;
    end

    DoSimpleCmd( s, sprintf( 'SETTRIGGEROFFBEEP %d %d', hertz, millisec ) );
end

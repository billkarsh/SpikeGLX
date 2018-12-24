% sampleRate = GetSampleRate( myobj, streamID )
%
%     Returns sample rate of selected stream in Hz,
%     or zero if not enabled.
%
function [ret] = GetSampleRate( s, streamID )

    ret = str2double( DoQueryCmd( s, sprintf( 'GETSAMPLERATE %d', streamID ) ) );
end

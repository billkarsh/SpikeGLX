% [Vmin,Vmax] = GetImVoltageRange( myobj, streamID )
%
%     Returns votlage range of selected IMEC probe.
%
function [Vmin,Vmax] = GetImVoltageRange( s, streamID )

    ret = DoQueryCmd( s, sprintf( 'GETIMVOLTAGERANGE %d', streamID ) );
    C   = textscan( ret, '%g %g' );

    Vmin = C{1};
    Vmax = C{2};
end

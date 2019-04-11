% [SN,type] = GetImProbeSN( myobj, streamID )
%
%     Returns serial number string (SN) and integer type
%     of selected IMEC probe.
%
function [SN,type] = GetImProbeSN( s, streamID )

    ret = DoQueryCmd( s, sprintf( 'GETIMPROBESN %d', streamID ) );
    C   = textscan( ret, '%[^ ] %d' );

    SN   = C{1};
    type = C{2};
end

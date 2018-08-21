% [SN,option] = GetImProbeSN( myobj )
%
%     Returns serial number string (SN) and integer option
%     of current IMEC probe.
%
function [SN,option] = GetImProbeSN( s )

    C = textscan( DoQueryCmd( s, 'GETIMPROBESN' ), '%[^ ] %d' );

    SN      = C{1};
    option  = C{2};
end

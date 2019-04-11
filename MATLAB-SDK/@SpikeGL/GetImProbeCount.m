% n_probes = GetImProbeCount( myobj )
%
%     Returns count of enabled IMEC probes.
%
function [ret] = GetImProbeCount( s )

    ret = sscanf( DoQueryCmd( s, 'GETIMPROBECOUNT' ), '%d' );
end

% dstSample = MapSample( myobj, dstStream, srcSample, srcStream )
%
%     Returns sample in dst stream corresponding to
%     given sample in src stream.
%
function [ret] = MapSample( s, dstStream, srcSample, srcStream )

    ret = str2double( DoQueryCmd( s, sprintf( 'MAPSAMPLE %d %ld %d', ...
            dstStream, srcSample, srcStream ) ) );
end

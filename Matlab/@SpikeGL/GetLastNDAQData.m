% daqData = GetLastNDAQData( myObj, NUM, channel_subset, downsample_ratio )
%
%     Get MxN matrix of the most recent (M = NUM) samples
%     for N channels. If channel_subset is not specified,
%     N = all acquired channels. NUM is a maximum count.
%
%     downsample_ratio is an integer (default = 1).
%
function [mat] = GetLastNDAQData( s, num, varargin )

    if( nargin >= 3 )
        subset = varargin{1};
    else
        subset = GetChannelSubset( s );
    end

    dwnsmp = 1;

    if( nargin >= 4 )

        dwnsmp = varargin{2};

        if ( ~isnumeric( dwnsmp ) || length( dwnsmp ) > 1 )
            error( 'Downsample factor must be a single numeric value' );
        end
    end

    scanCt = GetScanCount( s );

    if( num > scanCt )
        num = scanCt;
    end

    mat = GetDAQData( s, scanCt-num, num, subset, dwnsmp );
end

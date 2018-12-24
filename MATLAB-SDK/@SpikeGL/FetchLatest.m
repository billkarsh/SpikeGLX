% [daqData,headCt] = FetchLatest( myObj, streamID, scan_ct, channel_subset, downsample_ratio )
%
%     Get MxN matrix of the most recent stream data.
%     M = scan_ct = max samples to fetch.
%     N = channel count...
%         If channel_subset is not specified, N = current
%         SpikeGLX save-channel subset.
%
%     downsample_ratio is an integer (default = 1).
%
%     Also returns headCt = index of first timepoint in matrix.
%
function [mat,headCt] = FetchLatest( s, streamID, scan_ct, varargin )

    if( nargin < 3 )
        error( 'FetchLatest requires at least three arguments' );
    else if( nargin >= 4 )
        subset = varargin{1};
    else
        subset = GetSaveChans( s, streamID );
    end

    dwnsmp = 1;

    if( nargin >= 5 )

        dwnsmp = varargin{2};

        if ( ~isnumeric( dwnsmp ) || length( dwnsmp ) > 1 )
            error( 'Downsample factor must be a single numeric value' );
        end
    end

    max_ct = GetScanCount( s, streamID );

    if( scan_ct > max_ct )
        scan_ct = max_ct;
    end

    [mat,headCt] = Fetch( s, streamID, max_ct-scan_ct, scan_ct, subset, dwnsmp );
end

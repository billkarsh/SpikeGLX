% [daqData,headCt] = GetDAQData( myObj, start_scan, scan_ct, channel_subset, downsample_factor )
%
%     Get MxN matrix of (M = scan_ct) samples for N channels,
%     starting at scan with index start_scan. scan_ct is taken
%     as a maximum value. N is all channels unless channel_subset
%     is specified. Data are int16 type.
%
%     downsample_ratio is an integer (default = 1).
%
%     Also returns headCt = index of first timepoint in matrix.
%
function [mat,headCt] = GetDAQData( s, start_scan, scan_ct, varargin )

    if( ~isnumeric( start_scan ) || ~size( start_scan, 1 ) )
        error( 'Invalid scan_start parameter' );
    end

    if( ~isnumeric( scan_ct ) || ~size( scan_ct, 1 ) )
        error( 'Invalid scan_ct parameter' );
    end

    ChkConn( s );

    % subset has pattern id1#id2#...
    if( nargin >= 4 )
        subset = sprintf( '%d#', varargin{1} );
    else
        subset = sprintf( '%d#', GetChannelSubset( s ) );
    end

    dwnsmp = 1;

    if( nargin >= 5 )

        dwnsmp = varargin{2};

        if( ~isnumeric( dwnsmp ) || length( dwnsmp ) > 1 )
            error( 'Downsample factor must be a single numeric value' );
        end
    end

    ok = CalinsNetMex( 'sendString', s.handle, ...
            sprintf( 'GETDAQDATA %ld %d %s %d\n', ...
            start_scan, scan_ct, subset, dwnsmp ) );

    line = CalinsNetMex( 'readLine', s.handle );

    if( strfind( line, 'ERROR' ) == 1 )
        error( line );
        return;
    end

    % cells       = strsplit( line );
    cells       = strread( line, '%s' );
    mat_dims	= [str2num(cells{2}) str2num(cells{3})];
    headCt      = str2num(cells{4});

    if( ~isnumeric( mat_dims ) || ~size( mat_dims, 2 ) )
        error( 'Invalid matrix dimensions.' );
    end

    mat = CalinsNetMex( 'readMatrix', s.handle, 'int16', mat_dims );

    % transpose
    mat = mat';

    ReceiveOK( s, 'GetDAQData' );
end

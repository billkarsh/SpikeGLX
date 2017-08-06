% myobj = SetMetaData( myobj, metadata_struct )
%
%     If a run is in progress, set meta data to be added to the
%     next output file set. Meta data must be in the form of a
%     struct of name/value pairs.
%
function [s] = SetMetaData( s, meta )

    if( ~isstruct( meta ) )
        error( 'Argument to SetMetaData must be a struct.' );
    end

    ChkConn( s );

    ok = CalinsNetMex( 'sendString', s.handle, sprintf( 'SETMETADATA\n' ) );
    ReceiveREADY( s, 'SETMETADATA\n' );

    names = fieldnames( meta );

    for i = 1:length( names )

        f = meta.(names{i});

        if( isnumeric( f ) && isscalar( f ) )
            line = sprintf( '%s = %g\n', names{i}, f );
        elseif ( ischar( f ) )
            line = sprintf( '%s = %s\n', names{i}, f );
        else
            error( 'Field %s must be numeric scalar or a string', names{i} );
        end

        ok = CalinsNetMex( 'sendString', s.handle, line );
    end

    % end with blank line
    ok = CalinsNetMex( 'sendString', s.handle, sprintf( '\n' ) );

    ReceiveOK( s, 'SETMETADATA\n' );
end

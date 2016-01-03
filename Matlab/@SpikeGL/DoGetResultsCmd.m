% THIS FUNCTION IS PRIVATE AND SHOULD NOT BE CALLED BY OUTSIDE CODE!
%
function [res] = DoGetResultsCmd( s, cmd )

    ChkConn( s );

    ok = CalinsNetMex( 'sendString', s.handle, sprintf( '%s\n', cmd ) );

    res = cell( 0 );

    while( 1 )

        line = CalinsNetMex( 'readLine', s.handle );

        if( strcmp( line, 'OK' ) )
            break;
        end

        if( strfind( line, 'ERROR' ) == 1 )
            error( 'Got reply [%s] for cmd [%s]', line, cmd );
        end

        if( ~isempty( line ) )
            res = [res; line];
        end
    end
end

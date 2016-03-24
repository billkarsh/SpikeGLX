% THIS FUNCTION IS PRIVATE AND SHOULD NOT BE CALLED BY OUTSIDE CODE!
%
function [res] = DoQueryCmd( sm, cmd )

    ChkConn( sm );

    ok = CalinsNetMex( 'sendstring', sm.handle, sprintf( '%s\n', cmd ) );

    if( isempty( ok ) )
        error( '[%s] error: cannot send string.', cmd );
    end

    lines = CalinsNetMex( 'readlines', sm.handle );
    [m,n] = size( lines );

    if( m < 1 || n < 1 )
        error( '[%s] error: empty result; probably disconnected.', cmd );
    end

    if( ~isempty( strfind( lines( m, 1:n ), 'ERROR' ) ) )
        error( 'Query [%s] gave response [%s].', cmd, lines( m, 1:n ) );
    elseif( m < 2 || isempty( strfind( lines( 2, 1:n ), 'OK' ) ) )
        ReceiveOK( sm, cmd );
    end

    res = lines( 1, 1:n );
end

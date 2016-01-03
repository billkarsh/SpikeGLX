% THIS FUNCTION IS PRIVATE AND SHOULD NOT BE CALLED BY OUTSIDE CODE!
%
function [sm] = ChkConn( sm )

    if( sm.in_chkconn )
        return;
    end

    if( sm.handle == -1 )

        sm.handle = CalinsNetMex( 'create', sm.host, sm.port );

        if( isempty( CalinsNetMex( 'connect', sm.handle ) ) )
            error( 'Unable to connect to server.' );
        end

        sm.in_chkconn	= 1;
        sm.ver          = DoQueryCmd( sm, 'GETVERSION' );

    else

        ok = CalinsNetMex( 'sendstring', sm.handle, sprintf( 'NOOP\n' ) );

        if( isempty( ok ) || isempty( CalinsNetMex( 'readline', sm.handle ) ) )

            if( isempty( CalinsNetMex( 'connect', sm.handle ) ) )
                error( 'Still unable to connect to server.' );
            end
        end

        sm.in_chkconn = 1;
    end
end

% myobj = SetAOParams( myobj, params_struct )
%
%     Set parameters for analog-out operation. Parameters are
%     a struct of name/value pairs. This call stops current AO.
%     Call SetAOEnable( myobj, 1 ) to restart it.
%
function [s] = SetAOParams( s, params )

    if( ~isstruct( params ) )
        error( 'Argument to SetAOParams must be a struct.' );
    end

    ChkConn( s );

    ok = CalinsNetMex( 'sendString', s.handle, sprintf( 'SETAOPARAMS\n' ) );
    ReceiveREADY( s, 'SETAOPARAMS\n' );

    names = fieldnames( params );

    for i = 1:length( names )

        f = params.(names{i});

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

    ReceiveOK( s, 'SETAOPARAMS\n' );
end

% myobj = SetParams( myobj, params_struct )
%
%     The inverse of GetParams.m, this sets run parameters.
%     Alternatively, you can pass the parameters to StartRun()
%     which calls this in turn. Run parameters are a struct of
%     name/value pairs. The call will fail with an error if a
%     run is currently in progress.
%
function [s] = SetParams( s, params )

    if( ~isstruct( params ) )
        error( 'Argument to SetParams must be a struct.' );
    end

    ChkConn( s );

    if( IsRunning( s ) )
        error( 'Cannot set params while running. Call StopRun() first.' );
    end

    ok = CalinsNetMex( 'sendString', s.handle, sprintf( 'SETPARAMS\n' ) );
    ReceiveREADY( s, 'SETPARAMS' );

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

    ReceiveOK( s, 'SETPARAMS' );
end

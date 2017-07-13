% myobj = SetAudioParams( myobj, params_struct )
%
%     Set parameters for audio-out operation. Parameters are a
%     struct of name/value pairs. This call stops current output.
%     Call SetAudioEnable( myobj, 1 ) to restart it.
%
function [s] = SetAudioParams( s, params )

    if( ~isstruct( params ) )
        error( 'Argument to SetAudioParams must be a struct.' );
    end

    ChkConn( s );

    ok = CalinsNetMex( 'sendString', s.handle, sprintf( 'SETAUDIOPARAMS\n' ) );
    ReceiveREADY( s, 'SETAUDIOPARAMS\n' );

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

    ReceiveOK( s, 'SETAUDIOPARAMS\n' );
end

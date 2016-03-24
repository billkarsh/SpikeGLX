% res = Par2( myobj, op, filename )
%
%     Create, Verify, or Repair Par2 redundancy files for
%     'filename'. Arguments:
%
%     op: a string that is either 'c', 'v', or 'r' for create,
%     verify or repair respectively.
%
%     filename: the .par2 or .bin file to which 'op' is appled.
%
%     Progress is reported to the command window.
%
function [res] = Par2( s, op, file )

    res = 0;

    if( ~strcmp( op, 'v' ) && ~strcmp( op, 'r' ) && ~strcmp( op, 'c' ) )
        error( 'Op must be one of ''v'', ''r'' or ''c''.' );
    end

    ChkConn( s );

    if ( IsRunning( s ) )
        error( 'Cannot use Par2 while running.' );
    end

    ok = CalinsNetMex( 'sendString', s.handle, sprintf( 'PAR2 %s %s\n', op, file ) );

    while( 1 )

        line = CalinsNetMex( 'readLine', s.handle );

        if( strcmp( line,'OK' ) )
            res = 1;
            break;
        end

        if( strfind( line, 'ERROR' ) == 1 )
            fprintf( 'Par2 Error: %s\n', line( 7:length( line ) ) );
            break;
        end

        if( ~isempty( line ) )
            fprintf( '%s\n', line );
        end
    end
end

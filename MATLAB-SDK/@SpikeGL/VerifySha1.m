% res = VerifySha1( myobj, filename )
%
%     Verifies the SHA1 sum of the file specified by filename.
%     If filename is relative, it is appended to the run dir.
%     Absolute path/filenames are also supported. Since this is
%     a potentially long operation, it uses the 'disp' command
%     to print progress information to the MATLAB console. The
%     returned value is 1 if verified, 0 otherwise.
%
function [res] = VerifySha1( s, file )

    res = 0;

    ChkConn( s );

    ok = CalinsNetMex( 'sendString', s.handle, sprintf( 'VERIFYSHA1 %s\n', file ) );

    while( 1 )

        line = CalinsNetMex( 'readLine', s.handle );

        if( strcmp( line,'OK' ) )
            res = 1;
            break;
        end

        if( strfind( line, 'ERROR' ) == 1 )
            fprintf( 'Verify failed: %s\n', line( 7:length( line ) ) );
            break;
        end

        if( ~isempty( line ) )
            fprintf( 'Verify progress: %s%%\n', line );
        end
    end
end

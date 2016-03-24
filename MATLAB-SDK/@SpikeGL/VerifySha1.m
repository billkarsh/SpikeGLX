% res = VerifySha1( myobj, filename )
%
%     Verifies the SHA1 sum of the file specified by filename.
%     If filename is relative, it is interpreted as being
%     relative to the run dir. Absolute filenames (starting
%     with a '/') are supported as well. Since this is a long
%     operation, this functions uses the 'disp' command to print
%     progress information to the MATLAB console. The returned
%     value is 1 if verified, 0 otherwise.
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

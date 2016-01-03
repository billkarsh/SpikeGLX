% THIS FUNCTION IS PRIVATE AND SHOULD NOT BE CALLED BY OUTSIDE CODE!
%
function [] = ReceiveOK( sm, cmd )

    line = CalinsNetMex( 'readline', sm.handle );

    if( isempty( strfind( line, 'OK' ) ) )
        error( 'After cmd [%s] got [%s] but expected ''OK''.\n', cmd, line );
    end
end

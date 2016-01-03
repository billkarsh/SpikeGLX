% THIS FUNCTION IS PRIVATE AND SHOULD NOT BE CALLED BY OUTSIDE CODE!
%
function [] = ReceiveREADY( sm, cmd )

    line = CalinsNetMex( 'readline', sm.handle );

    if( isempty( strfind( line, 'READY' ) ) )
        error( 'After cmd [%s] got [%s] but expected ''READY''.\n', cmd, line );
    end
end

% myobj = SetDigOut( myobj, bool_flag, channel )
%
%     Set digital output on/off. Channel strings have form:
%     'Dev6/port0/line2'.
%
function [s] = SetDigOut( s, b, channel )

    if( ~isnumeric( b ) )
        error( 'SetDigOut arg 2 should be a Boolean value {0,1}.' );
    end

    DoSimpleCmd( s, sprintf( 'SETDIGOUT %d %s', b, channel ) );
end

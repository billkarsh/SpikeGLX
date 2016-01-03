% myobj = SetRunName( myobj, 'name' )
%
%     Set the run name for the next time files are created
%     (either by SetTrgEnable() or by StartRun()).
%
function [s] = SetRunName( s, name )

    if( ~ischar( name ) )
        error( 'Argument to SetRunName must be a string.' );
    end

    DoSimpleCmd( s, sprintf( 'SETRUNNAME %s', name ) );
end

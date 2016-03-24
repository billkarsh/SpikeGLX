% myobj = SetRunDir( myobj, dir )
%
%     Set global run data directory.
%
function [s] = SetRunDir( s, dir )

    if( ~ischar( dir ) )
        error( 'SetRunDir ''dir'' argument must be a string.' );
    end

    DoSimpleCmd( s, sprintf( 'SETRUNDIR %s', dir ) );
end

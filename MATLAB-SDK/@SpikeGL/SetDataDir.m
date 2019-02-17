% myobj = SetDataDir( myobj, dir )
%
%     Set global run data directory.
%
function [s] = SetDataDir( s, dir )

    if( ~ischar( dir ) )
        error( 'SetDataDir ''dir'' argument must be a string.' );
    end

    DoSimpleCmd( s, sprintf( 'SETDATADIR %s', dir ) );
end

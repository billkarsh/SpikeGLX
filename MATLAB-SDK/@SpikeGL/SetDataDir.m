% myobj = SetDataDir( myobj, i, dir )
%
%     Set ith global data directory.
%     Set required parameter i to zero for main data directory.
%
function [s] = SetDataDir( s, i, dir )

    if( ~ischar( dir ) )
        error( 'SetDataDir ''dir'' argument must be a string.' );
    end

    DoSimpleCmd( s, sprintf( 'SETDATADIR %d %s', i, dir ) );
end

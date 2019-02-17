% dir = GetDataDir( myobj )
%
%     Get global run data directory.
%
function [ret] = GetDataDir( s )

    ret = DoQueryCmd( s, 'GETDATADIR' );
end

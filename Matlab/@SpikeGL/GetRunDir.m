% dir = GetRunDir( myobj )
%
%     Get global run data directory.
%
function [ret] = GetRunDir( s )

    ret = DoQueryCmd( s, 'GETRUNDIR' );
end

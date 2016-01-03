% params = EnumRunDir( myobj )
%
%     Retrieve a listing of files in the run directory.
%
function [ret] = EnumRunDir( s )

    ret = DoGetResultsCmd( s, 'ENUMRUNDIR' );
end

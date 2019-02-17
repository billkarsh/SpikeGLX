% params = EnumDataDir( myobj )
%
%     Retrieve a listing of files in the data directory.
%
function [ret] = EnumDataDir( s )

    ret = DoGetResultsCmd( s, 'ENUMDATADIR' );
end

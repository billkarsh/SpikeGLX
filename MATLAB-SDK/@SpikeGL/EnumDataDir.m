% params = EnumDataDir( myobj, i )
%
%     Retrieve a listing of files in the ith data directory.
%     Get main data directory by setting i=0 or omitting it.
%
function [ret] = EnumDataDir( s, varargin )

    i = 0;

    if( nargin > 1 )
        i = varargin{1};
    end

    ret = DoGetResultsCmd( s, sprintf( 'ENUMDATADIR %d', i ) );
end

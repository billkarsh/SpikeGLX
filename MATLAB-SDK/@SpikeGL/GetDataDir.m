% dir = GetDataDir( myobj, i )
%
%     Get ith global data directory.
%     Get main data directory by setting i=0 or omitting it.
%
function [ret] = GetDataDir( s, varargin )

    i = 0;

    if( nargin > 1 )
        i = varargin{1};
    end

    ret = DoQueryCmd( s, sprintf( 'GETDATADIR %d', i ) );
end

% myobj = StartRun( myobj )
% myobj = StartRun( myobj, params )
% myobj = StartRun( myobj, runName )
%
%     Start data acquisition run. Optional second argument (params)
%     is a struct of name/value pairs as returned from GetParams.m.
%     Alternatively, the second argument can be a string (runName).
%     Last-used parameters remain in effect if not specified here.
%     An error is flagged if already running or a parameter is bad.
%
function [s] = StartRun( varargin )

    s      = varargin{1};
    rname  = [];
    params = [];

    if( nargin > 1 )

        arg2 = varargin{2};

        if( ischar( arg2 ) )
            rname = arg2;
        elseif( isstruct( arg2 ) )
            params = arg2;
        else
            error( 'Invalid argument type for second argument. Must be a string or struct.' );
        end
    end

    if( IsRunning( s ) )
        error( 'Already running.' );
    end

    if( isempty( params ) )
        params = GetParams( s );
    end

    if( ~isempty( rname ) )
        params.snsRunName = rname;
    end

    SetParams( s, params );

    DoSimpleCmd( s, sprintf( 'STARTRUN' ) );
end

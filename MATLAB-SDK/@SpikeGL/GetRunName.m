% name = GetRunName( myobj )
%
%     Get run base name.
%
function [name] = GetRunName( s )

    p = GetParams( s );
    name = p.runName;
end

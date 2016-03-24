% boolval = IsSaving( myobj )
%
%     Returns 1 if the software is currently running
%     AND saving data.
%
function [ret] = IsSaving( s )

    ret = sscanf( DoQueryCmd( s, 'ISSAVING' ), '%d' );
end

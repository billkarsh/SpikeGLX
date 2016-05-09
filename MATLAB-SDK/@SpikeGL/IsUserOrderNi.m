% boolval = IsUserOrderNi( myobj )
%
%     Returns 1 if graphs currently sorted in user order.
%
function [ret] = IsUserOrderNi( s )

    ret = sscanf( DoQueryCmd( s, 'ISUSRORDERNI' ), '%d' );
end

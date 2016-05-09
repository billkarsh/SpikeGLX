% boolval = IsUserOrderIm( myobj )
%
%     Returns 1 if graphs currently sorted in user order.
%
function [ret] = IsUserOrderIm( s )

    ret = sscanf( DoQueryCmd( s, 'ISUSRORDERIM' ), '%d' );
end

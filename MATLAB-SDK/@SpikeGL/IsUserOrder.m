% boolval = IsUserOrder( myobj, streamID )
%
%     Returns 1 if graphs currently sorted in user order.
%
%     This query is sent only to the main Graphs window.
%
function [ret] = IsUserOrder( s, streamID )

    ret = sscanf( DoQueryCmd( s, sprintf( 'ISUSRORDER %d', streamID ) ), '%d' );
end

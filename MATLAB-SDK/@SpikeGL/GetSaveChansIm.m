% channelSubset = GetSaveChansIm( myobj, streamID )
%
%     Returns a vector containing the indices of
%     IMEC channels being saved.
%
function [ret] = GetSaveChansIm( s, streamID )

    ret = str2num( DoQueryCmd( s, sprintf( 'GETSAVECHANSIM %d', streamID ) ) );
end

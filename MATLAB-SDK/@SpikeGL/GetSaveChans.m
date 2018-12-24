% channelSubset = GetSaveChans( myobj, streamID )
%
%     Returns a vector containing the indices of
%     IMEC channels being saved.
%
function [ret] = GetSaveChans( s, streamID )

    ret = str2num( DoQueryCmd( s, sprintf( 'GETSAVECHANS %d', streamID ) ) );
end

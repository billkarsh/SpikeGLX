% channelSubset = GetChannelSubset( myobj )
%
%     Returns a vector containing the indices of
%     channels being saved.
%
function [ret] = GetChannelSubset( s )

    ret = str2num( DoQueryCmd( s, 'GETCHANNELSUBSET' ) );
end

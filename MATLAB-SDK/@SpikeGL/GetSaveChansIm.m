% channelSubset = GetSaveChansIm( myobj )
%
%     Returns a vector containing the indices of
%     IMEC channels being saved.
%
function [ret] = GetSaveChansIm( s )

    ret = str2num( DoQueryCmd( s, 'GETSAVECHANSIM' ) );
end

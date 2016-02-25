% channelSubset = GetSavedChansIm( myobj )
%
%     Returns a vector containing the indices of
%     IMEC channels being saved.
%
function [ret] = GetSavedChansIm( s )

    ret = str2num( DoQueryCmd( s, 'GETSAVECHANSIM' ) );
end

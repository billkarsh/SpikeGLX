% channelSubset = GetSavedChansNi( myobj )
%
%     Returns a vector containing the indices of
%     Ni-DAQ channels being saved.
%
function [ret] = GetSavedChansNi( s )

    ret = str2num( DoQueryCmd( s, 'GETSAVECHANSNI' ) );
end

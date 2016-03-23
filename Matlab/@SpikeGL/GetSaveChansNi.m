% channelSubset = GetSaveChansNi( myobj )
%
%     Returns a vector containing the indices of
%     Ni-DAQ channels being saved.
%
function [ret] = GetSaveChansNi( s )

    ret = str2num( DoQueryCmd( s, 'GETSAVECHANSNI' ) );
end

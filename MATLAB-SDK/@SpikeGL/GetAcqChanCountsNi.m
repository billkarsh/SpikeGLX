% chanCounts = GetAcqChanCountsNi( myobj )
%
%     Returns a vector containing the counts of 16-bit
%     words of each class being acquired {MN,MA,XA,DW}.
%
function [ret] = GetAcqChanCountsNi( s )

    ret = str2num( DoQueryCmd( s, 'GETACQCHANCOUNTSNI' ) );
end

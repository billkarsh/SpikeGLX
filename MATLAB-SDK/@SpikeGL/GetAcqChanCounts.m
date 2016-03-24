% chanCounts = GetAcqChanCounts( myobj )
%
%     Returns a vector containing the counts of 16-bit
%     words of each class being acquired {AP,LF,SY,MN,MA,XA,DW}.
%
function [ret] = GetAcqChanCounts( s )

    ret = str2num( DoQueryCmd( s, 'GETACQCHANCOUNTS' ) );
end

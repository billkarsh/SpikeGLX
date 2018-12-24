% chanCounts = GetAcqChanCounts( myobj, streamID  )
%
%     Returns a vector containing the counts of 16-bit
%     words of each class being acquired.
%
%     streamID = -1: NI channels: {MN,MA,XA,DW}.
%     streamID >= 0: IM channels: {AP,LF,SY}.
%
function [ret] = GetAcqChanCounts( s, streamID )

    ret = str2num( DoQueryCmd( s, sprintf( 'GETACQCHANCOUNTS %d', streamID ) ) );
end

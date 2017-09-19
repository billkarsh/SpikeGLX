% chanCounts = GetAcqChanCountsIm( myobj, streamID  )
%
%     Returns a vector containing the counts of 16-bit
%     words of each class being acquired {AP,LF,SY}.
%
function [ret] = GetAcqChanCountsIm( s, streamID )

    ret = str2num( DoQueryCmd( s, sprintf( 'GETACQCHANCOUNTSIM %d', streamID ) ) );
end

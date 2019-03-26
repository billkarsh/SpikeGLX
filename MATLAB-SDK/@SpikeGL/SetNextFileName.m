% myobj = SetNextFileName( myobj, 'name' )
%
%     If a run is in progress, set the full path and run name
%     for the next file set. For example, "Y:/mydir/../myrun."
%     To that, SpikeGLX appends the stream and extension, for
%     example, "Y:/mydir/../myrun.imec0.ap.bin." SpikeGLX will
%     not create any folders or add any _g_t tags, so the name
%     is under full user control. SpikeGLX clears your custom
%     name and reverts to auto-naming after the current files
%     are written. You must, therefore, call this function for
%     each file, set and you must set the name before a trigger
%     event for that file set.
%
function [s] = SetNextFileName( s, name )

    if( ~ischar( name ) )
        error( 'Argument to SetNextFileName must be a string.' );
    end

    DoSimpleCmd( s, sprintf( 'SETNEXTFILENAME %s', name ) );
end

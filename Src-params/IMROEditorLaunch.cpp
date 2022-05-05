
#include "IMROEditorLaunch.h"




/* ---------------------------------------------------------------- */
/* IMROEditorLaunch ----------------------------------------------- */
/* ---------------------------------------------------------------- */

bool IMROEditorLaunch(
    QObject         *parent,
    QString         &outFile,
    const QString   &file,
    int             selectRow,
    int             type )
{
    bool    changed = false;

    if( type == 1110 ) {
        IMROEditor_T1110    ED( parent );
        changed = ED.edit( outFile, file, selectRow );
    }
    else if( type == 21 ) {
        IMROEditor_T21      ED( parent );
        changed = ED.edit( outFile, file, selectRow );
    }
    else if( type == 24 ) {
        IMROEditor_T24      ED( parent );
        changed = ED.Edit( outFile, file, selectRow );
    }
    else {
        IMROEditor_T0       ED( parent, type );
        changed = ED.edit( outFile, file, selectRow );
    }

    return changed;
}




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

    if( type != 21 && type != 24 ) {
        IMROEditor_T0   ED( parent, type );
        changed = ED.Edit( outFile, file, selectRow );
    }

    return changed;
}



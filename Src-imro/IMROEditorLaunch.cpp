
#include "IMROEditorLaunch.h"




/* ---------------------------------------------------------------- */
/* IMROEditorLaunch ----------------------------------------------- */
/* ---------------------------------------------------------------- */

bool IMROEditorLaunch(
    QObject         *parent,
    QString         &outFile,
    const QString   &file,
    const QString   &pn,
    int             selectRow )
{
    bool    changed = false;

    if( pn == "NP1110" ) {
        IMROEditor_T1110    ED( parent, pn );
        changed = ED.edit( outFile, file, selectRow );
    }
    else if( pn == "NP2000" ) {
        IMROEditor_T21      ED( parent, pn );
        changed = ED.edit( outFile, file, selectRow );
    }
    else if( pn == "NP2010" ) {
        IMROEditor_T24      ED( parent, pn );
        changed = ED.Edit( outFile, file, selectRow );
    }
    else {
        IMROEditor_T0       ED( parent, pn );
        changed = ED.edit( outFile, file, selectRow );
    }

    return changed;
}



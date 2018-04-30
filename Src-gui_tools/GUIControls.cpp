
#include "GUIControls.h"
#include "SignalBlocker.h"

#include <QComboBox>


/* ---------------------------------------------------------------- */
/* Functions ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Fill standard stream CB with extant choices {nidq, imec0, imec1, ...}.
//
void FillExtantStreamCB( QComboBox *CB, bool isNidq, int nProbes )
{
    SignalBlocker   b(CB);

    CB->clear();

    if( isNidq )
        CB->addItem( "nidq" );

    for( int ip = 0; ip < nProbes; ++ip )
        CB->addItem( QString("imec%1").arg( ip ) );
}


// Select selStream item in standard stream CB.
//
// Return true if found in control.
//
bool SelStreamCBItem( QComboBox *CB, const QString &selStream )
{
    SignalBlocker   b(CB);

    int     sel     = CB->findText( selStream );
    bool    found   = sel >= 0;

    if( !found )
        sel = 0;

    CB->setCurrentIndex( sel );

    return found;
}



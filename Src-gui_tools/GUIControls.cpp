
#include "GUIControls.h"
#include "SignalBlocker.h"

#include <QComboBox>


/* ---------------------------------------------------------------- */
/* Functions ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Fill standard stream CB with choices {nidq, imec0, imec1, ...}.
//
void FillStreamCB( QComboBox *CB, int nProbes )
{
    SignalBlocker   b(CB);

    CB->clear();
    CB->addItem( "nidq" );

    for( int ip = 0; ip < nProbes; ++ip )
        CB->addItem( QString("imec%1").arg( ip ) );
}


// Select selStream item in standard stream CB.
//
void SelStreamCBItem( QComboBox *CB, QString selStream )
{
    SignalBlocker   b(CB);

    int sel = CB->findText( selStream );

    if( sel < 0 )
        sel = 0;

    CB->setCurrentIndex( sel );
}



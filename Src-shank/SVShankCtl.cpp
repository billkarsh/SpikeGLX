
#include "ui_ShankWindow.h"

#include "Util.h"
#include "SVShankCtl.h"
#include "DAQ.h"

#include <QCloseEvent>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

SVShankCtl::SVShankCtl( const DAQ::Params &p, int jpanel, QWidget *parent )
    :   ShankCtlBase(parent, false), p(p), svTab(0), jpanel(jpanel)
{
}


SVShankCtl::~SVShankCtl()
{
    if( svTab ) {
        delete svTab;
        svTab = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void SVShankCtl::parInit( int js, int ip )
{
    baseInit( (js == jsIM ? p.im.prbj[ip].roTbl : 0), true );

    ConnectUI( view(), SIGNAL(cursorOver(int,bool)), this, SLOT(cursorOver(int,bool)) );
    ConnectUI( view(), SIGNAL(lbutClicked(int,bool)), this, SLOT(lbutClicked(int,bool)) );

// Tabs

    if( js == jsIM )
        seTab->setCurrent( p.im.prbj[ip].imroFile );

    if( seTab )
        seTab->renameApplyRevert();

    svTab = new SVShankViewTab( this, scUI->svTab, p, js, ip );

    ConnectUI( this, SIGNAL(viewTabSyncYPix(int)), svTab, SLOT(syncYPix(int)) );

    loadSettings();

    svTab->init();

    mapChanged();
    selChan( 0, "" );

// Window

    restoreScreenState();
}


void SVShankCtl::closeEvent( QCloseEvent *e )
{
    QDialog::closeEvent( e );

    if( e->isAccepted() ) {

        // reset for next showing of window
        svTab->winClosed();

        saveScreenState();
        emit closed( this );
    }
}



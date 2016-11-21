
#include "ui_ShankWindow.h"

#include "ShankCtl.h"
#include "Util.h"
#include "MainApp.h"
#include "ShankMap.h"

#include <QCloseEvent>

/* ---------------------------------------------------------------- */
/* ShankCtl ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankCtl::ShankCtl( DAQ::Params &p, QWidget *parent )
    :   QWidget(parent), p(p)
{
}


ShankCtl::~ShankCtl()
{
    emit closed( this );

    if( scUI ) {
        delete scUI;
        scUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtl::showDialog()
{
    show();
    scUI->scroll->scrollToSelected();
    mainApp()->modelessOpened( this );
}


void ShankCtl::update()
{
    scUI->scroll->theV->updateNow();
}


void ShankCtl::selChan( int ic, const QString &name )
{
    const ShankMap  *M = scUI->scroll->theV->getSmap();

    if( M && ic < M->e.size() ) {

        scUI->scroll->theV->setSel( ic );
        scUI->chanBut->setText( name );
    }
}


void ShankCtl::layoutChanged()
{
    scUI->scroll->theV->setShankMap( scUI->scroll->theV->getSmap() );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtl::chanButClicked()
{
    scUI->scroll->scrollToSelected();
}


void ShankCtl::ypixChanged( int y )
{
    set.yPix = y;
    scUI->scroll->setRowPix( y );
    saveSettings();
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void ShankCtl::baseInit()
{
    loadSettings();

    scUI = new Ui::ShankWindow;
    scUI->setupUi( this );

    scUI->ypixSB->setValue( set.yPix );
    scUI->scroll->theV->initRowPix( set.yPix );

    ConnectUI( scUI->ypixSB, SIGNAL(valueChanged(int)), this, SLOT(ypixChanged(int)) );
    ConnectUI( scUI->chanBut, SIGNAL(clicked()), this, SLOT(chanButClicked()) );

    setAttribute( Qt::WA_DeleteOnClose, false );
}


void ShankCtl::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
    }
    else
        QWidget::keyPressEvent( e );
}


void ShankCtl::closeEvent( QCloseEvent *e )
{
    QWidget::closeEvent( e );

    if( e->isAccepted() )
        emit closed( this );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */



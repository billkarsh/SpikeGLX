
#include "ui_ShankEditTab.h"

#include "Util.h"
#include "ShankCtlBase.h"
#include "ShankView.h"
#include "SignalBlocker.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QSettings>
#include <QThread>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ShankEditTab::ShankEditTab(
    ShankCtlBase    *SC,
    QWidget         *tab,
    const IMROTbl   *inR )
    :   QObject(0), SC(SC), seTabUI(0), mClear(0),
        R0(IMROTbl::alloc( inR->pn )),
        Rfile(IMROTbl::alloc( inR->pn )),
        R(IMROTbl::alloc( inR->pn ))
{
    seTabUI = new Ui::ShankEditTab;
    seTabUI->setupUi( tab );

    loadSettings();
    initItems();

    R0->copyFrom( inR );
    R->copyFrom( inR );
    R->edit_init();
    R2GUI();

    ConnectUI( seTabUI->ypixSB, SIGNAL(valueChanged(int)), this, SLOT(ypixChanged(int)) );
    ConnectUI( seTabUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( seTabUI->defBut, SIGNAL(clicked()), this, SLOT(defBut()) );
    ConnectUI( seTabUI->clearBut, SIGNAL(clicked()), this, SLOT(clearAll()) );
    ConnectUI( seTabUI->saveBut, SIGNAL(clicked()), this, SLOT(saveBut()) );
    ConnectUI( seTabUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );
    ConnectUI( seTabUI->okBut, SIGNAL(clicked()), this, SLOT(okBut()) );
    ConnectUI( seTabUI->cancelBut, SIGNAL(clicked()), this, SLOT(cancelBut()) );
}


ShankEditTab::~ShankEditTab()
{
    delete R0;
    delete Rfile;
    delete R;

    if( mClear ) {
        delete mClear;
        mClear = 0;
    }

    if( seTabUI ) {
        delete seTabUI;
        seTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankEditTab::hideCurrent()
{
    seTabUI->curTag->hide();
    seTabUI->curLbl->hide();
}


void ShankEditTab::setCurrent( const QString &curFile )
{
    filename = curFile;

    seTabUI->curLbl->setText(
        curFile.isEmpty() ? "Default" :
        QFileInfo(curFile).fileName() );
}


void ShankEditTab::renameOriginal()
{
    seTabUI->curTag->setText( "Original" );
}


void ShankEditTab::hideOKBut()
{
    seTabUI->okBut->hide();
}


void ShankEditTab::renameApplyRevert()
{
    seTabUI->okBut->setText( "Apply" );
    seTabUI->cancelBut->setText( "Revert" );
}


void ShankEditTab::syncYPix( int y )
{
    SignalBlocker   b0(seTabUI->ypixSB);
    seTabUI->ypixSB->setValue( y );
}


void ShankEditTab::gridHover( int s, int r )
{
    if( s < 0 ) {
        SC->setStatus( "" );
        return;
    }

    SC->setStatus( QString("Grid Row %1").arg( r ) );
}


void ShankEditTab::gridClicked( int s, int r, bool shift )
{
    if( !canEdit ) {
        beep( "Non-standard IMRO; can not be edited" );
        return;
    }

    int nb = vR.size();

// Clear if shift

    if( shift ) {

        for( int ib = 0; ib < nb; ++ib ) {

            IMRO_ROI    &B = vR[ib];

            if( s == B.s && r >= B.r0 && r < B.rLim ) {

                vR.erase( vR.begin() + ib );
                R->edit_exclude( vX, vR );
                updateSums( s );
                getSelObj( s )->setText( "" );
                color();
                SC->setStatus( "" );
                return;
            }
        }
        return;
    }

// Full?

    if( G.nBase == 1 ) {
        if( getSum( 0 ) >= getRqd( 0 ) ) {
            beep( "All channels set" );
            return;
        }
    }
    else {
        if( getSum( s ) >= getRqd( s ) ) {
            beep( "All channels on shank set" );
            return;
        }
    }

// Only one possible?

    if( R->nBanks() == 1 ) {
        vX.clear();
        R->edit_defaultROI( vR );
        enableItems( true );
        color();
        SC->setStatus( "Probe requires default pattern" );
        return;
    }

// Where is the click?

    int gap0, gapLim, code = where( gap0, gapLim, s, r );

// Forbidden?

    if( code < 0 ) {
        beep( "Forbidden" );
        return;
    }

// Place into gap?

    if( code == 0 ) {
        IMRO_ROI    C;
        clickHere( C, gap0, gapLim, s, r );
        vR.push_back( C );
        getSelObj( s )->
        setText( QString("%1").arg( (C.rLim - C.r0)*R->nCol_hwr() ) );
    }

// Selected?

    if( code == 1 || code == 2 ) {

        int ib = selected( gap0, gapLim, code, s );

        if( ib >= 0 ) {
            const IMRO_ROI  &B = vR[ib];
            getSelObj( s )->
            setText( QString("%1").arg( (B.rLim - B.r0)*R->nCol_hwr() ) );
        }
    }

// Update

    updateSums( s );

    if( isDone() )
        vX.clear();
    else
        R->edit_exclude( vX, vR );

    color();
    SC->setStatus( "" );
}


void ShankEditTab::beep( const QString &msg )
{
    SC->setStatus( msg );
    ::Beep( 440, 200 );
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankEditTab::ypixChanged( int y )
{
    SC->scroll()->setRowPix( y );
    SC->syncYPix( y );
}


void ShankEditTab::loadBut()
{
// Get name

    QString fn = QFileDialog::getOpenFileName(
                    SC,
                    "Load IMEC readout table",
                    lastDir,
                    "Imro files (*.imro)" );

    if( !fn.length() )
        return;

    lastDir = QFileInfo( fn ).absolutePath();

// Load file

    QString msg;

    if( R->loadFile( msg, fn ) ) {
        saveSettings();
        R2GUI();
        Rfile->copyFrom( R );
        filename = fn;
    }

    if( !msg.isEmpty() )
        beep( msg );
    else
        SC->setStatus( msg );
}


void ShankEditTab::defBut()
{
    vX.clear();
    R->edit_defaultROI( vR );

    enableItems( true );

    IMRO_Attr   A = R->edit_Attr_def();
    seTabUI->rfCB->setCurrentIndex( A.refIdx );
    seTabUI->apCB->setCurrentIndex( A.apgIdx );
    seTabUI->lfCB->setCurrentIndex( A.lfgIdx );
    seTabUI->hpCB->setCurrentIndex( A.hpfIdx );
    color();
}


void ShankEditTab::clearAll()
{
    vX.clear();
    vR.clear();
    enableItems( true, true );
    color();
}


void ShankEditTab::clearShank0()
{
    clearShank( 0 );
}


void ShankEditTab::clearShank1()
{
    clearShank( 1 );
}


void ShankEditTab::clearShank2()
{
    clearShank( 2 );
}


void ShankEditTab::clearShank3()
{
    clearShank( 3 );
}


bool ShankEditTab::saveBut()
{
// Editable?

    if( !canEdit ) {
        beep( "Non-standard IMRO; can not be saved" );
        return false;
    }

// Finished?

    if( !GUI2R() )
        return false;

// Default?

    if( isDefault() ) {
        beep( "No need to save default IMRO" );
        return false;
    }

// Save where?

    QString fn = QFileDialog::getSaveFileName(
                    SC,
                    "Save IMEC readout table",
                    lastDir,
                    "Imro files (*.imro)" );

    if( fn.length() ) {

        lastDir = QFileInfo( fn ).absolutePath();
        saveSettings();

        QString msg;
        bool    ok;

        if( (ok = R->saveFile( msg, fn )) ) {
            Rfile->copyFrom( R );
            filename = fn;
        }

        if( !msg.isEmpty() )
            beep( msg );
        else
            SC->setStatus( msg );

        return ok;
    }

    return false;
}


void ShankEditTab::helpBut()
{
    showHelp( "ShankView_Help" );
}


void ShankEditTab::okBut()
{
    QString fn;

// Editable?

    if( !canEdit ) {

        if( *R == *R0 ) {
            beep( "Same as current...no action" );
            return;
        }

        fn = filename;
        goto update;
    }

// Finished?

    if( !GUI2R() )
        return;

// Changed?

    if( *R == *R0 ) {
        if( seTabUI->okBut->isVisible() && seTabUI->okBut->text() == "OK" ) {
            emit SC->modal_done( SC, filename, true );
            return;
        }
        beep( "Same as current...no action" );
        return;
    }

// Default?

    if( isDefault() )
        goto update;

// Saved?

    if( *R != *Rfile ) {
        if( !saveBut() )
            return;
    }

    fn = filename;

update:
    R0->copyFrom( R );
    setCurrent( fn );
    emit imroChanged( fn );
    emit SC->modal_done( SC, fn, true );
}


void ShankEditTab::cancelBut()
{
    R->copyFrom( R0 );
    R2GUI();

    emit SC->modal_done( SC, QString(), false );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankEditTab::initItems()
{
    G = R->edit_GUI();

// probe label

    seTabUI->prbLbl->setText( QString("%1").arg( R->pn ) );

// Boxes

    initClearMenu();
    initBoxes();

// Reference

    for( int i = 0, nr = G.refs.size(); i < nr; ++i )
        seTabUI->rfCB->addItem( G.refs[i] );

// Gains, highpass

    for( int i = 0, ng = G.gains.size(); i < ng; ++i ) {
        QString s = QString("%1").arg( G.gains[i] );
        seTabUI->apCB->addItem( s );
        seTabUI->lfCB->addItem( s );
    }

    seTabUI->apCB->setEnabled( G.apEnab );
    seTabUI->lfCB->setEnabled( G.lfEnab );
    seTabUI->hpCB->setEnabled( G.hpEnab );

    enableItems( true );
}


void ShankEditTab::initClearMenu()
{
    if( G.nBase == 4 ) {

        mClear = new QMenu();

        QAction *A;

        A = new QAction( "All Shanks", this );
        ConnectUI( A, SIGNAL(triggered()), this, SLOT(clearAll()) );
        mClear->addAction( A );

        A = new QAction( this );
        A->setSeparator( true );
        mClear->addAction( A );

        A = new QAction( "Shank 0", this );
        ConnectUI( A, SIGNAL(triggered()), this, SLOT(clearShank0()) );
        mClear->addAction( A );

        A = new QAction( "Shank 1", this );
        ConnectUI( A, SIGNAL(triggered()), this, SLOT(clearShank1()) );
        mClear->addAction( A );

        A = new QAction( "Shank 2", this );
        ConnectUI( A, SIGNAL(triggered()), this, SLOT(clearShank2()) );
        mClear->addAction( A );

        A = new QAction( "Shank 3", this );
        ConnectUI( A, SIGNAL(triggered()), this, SLOT(clearShank3()) );
        mClear->addAction( A );
    }
}


void ShankEditTab::initBoxes()
{
    QString s;
    int     cpb = R->nChanPerBank();

    seTabUI->rowsSB->setMinimum( 1 );
    seTabUI->rowsSB->setMaximum( cpb / R->nCol_hwr() );
    seTabUI->rowsSB->setValue( cpb / R->nCol_hwr() );

    if( G.nBase == 1 ) {
        seTabUI->rqd0->setText( QString("%1").arg( R->nAP() ) );
        seTabUI->sel1->hide();
        seTabUI->sel2->hide();
        seTabUI->sel3->hide();
        seTabUI->sum1->hide();
        seTabUI->sum2->hide();
        seTabUI->sum3->hide();
        seTabUI->rqd1->hide();
        seTabUI->rqd2->hide();
        seTabUI->rqd3->hide();
    }
    else {
        s = QString("%1").arg( cpb );
        seTabUI->rqd0->setText( s );
        seTabUI->rqd1->setText( s );
        seTabUI->rqd2->setText( s );
        seTabUI->rqd3->setText( s );
    }
}


void ShankEditTab::enableItems( bool enabled, bool clear )
{
    canEdit = enabled;

    SC->setStatus( "" );

    seTabUI->sel0->setText( "" );
    seTabUI->sel1->setText( "" );
    seTabUI->sel2->setText( "" );
    seTabUI->sel3->setText( "" );

    if( !enabled || !clear ) {
        seTabUI->sum0->setText( seTabUI->rqd0->text() );
        seTabUI->sum1->setText( seTabUI->rqd1->text() );
        seTabUI->sum2->setText( seTabUI->rqd2->text() );
        seTabUI->sum3->setText( seTabUI->rqd3->text() );
    }
    else {
        seTabUI->sum0->setText( "0" );
        seTabUI->sum1->setText( "0" );
        seTabUI->sum2->setText( "0" );
        seTabUI->sum3->setText( "0" );
    }

    if( enabled ) {

        if( G.nBase == 4 ) {
            seTabUI->clearBut->setText( "Clear" );
            seTabUI->clearBut->setMenu( mClear );
        }

        seTabUI->rfCB->setEnabled( true );
        seTabUI->apCB->setEnabled( G.apEnab );
        seTabUI->lfCB->setEnabled( G.lfEnab );
        seTabUI->hpCB->setEnabled( G.hpEnab );
    }
    else {
        if( G.nBase == 4 ) {
            seTabUI->clearBut->setText( "Clear All" );
            seTabUI->clearBut->setMenu( 0 );
        }

        seTabUI->rfCB->setEnabled( false );
        seTabUI->apCB->setEnabled( false );
        seTabUI->lfCB->setEnabled( false );
        seTabUI->hpCB->setEnabled( false );
    }
}


void ShankEditTab::clearShank( int s )
{
    for( int nb = vR.size(), ib = nb - 1; ib >= 0; --ib ) {

        if( s == vR[ib].s )
            vR.erase( vR.begin() + ib );
    }

    R->edit_exclude( vX, vR );
    getSelObj( s )->setText( "" );
    getSumObj( s )->setText( "0" );
    SC->setStatus( "" );
    color();
}


void ShankEditTab::R2GUI()
{
    R->edit_tbl2ROI( vR );
    enableItems( R->edit_isCanonical( vR ) );

    vX.clear();

    if( !canEdit ) {
        beep( "Non-standard IMRO; can not be edited" );
        guiBreathe();
        QThread::msleep( 1500 );
    }

    IMRO_Attr   A = R->edit_Attr_cur();
    seTabUI->rfCB->setCurrentIndex( A.refIdx );
    seTabUI->apCB->setCurrentIndex( A.apgIdx );
    seTabUI->lfCB->setCurrentIndex( A.lfgIdx );
    seTabUI->hpCB->setCurrentIndex( A.hpfIdx );
    color();
}


bool ShankEditTab::GUI2R()
{
    if( !isDone() ) {
        beep( "Some channels not set (see counts above)" );
        return false;
    }

    IMRO_Attr   A(
        seTabUI->rfCB->currentIndex(),
        seTabUI->apCB->currentIndex(),
        seTabUI->lfCB->currentIndex(),
        seTabUI->hpCB->currentIndex() );

    R->edit_ROI2tbl( vR, A );

    return true;
}


bool ShankEditTab::isDone()
{
    if( G.nBase == 1 )
        return getSum( 0 ) >= getRqd( 0 );
    else {
        return getSum( 0 ) >= getRqd( 0 ) &&
               getSum( 1 ) >= getRqd( 1 ) &&
               getSum( 2 ) >= getRqd( 2 ) &&
               getSum( 3 ) >= getRqd( 3 );
    }
}


bool ShankEditTab::isDefault()
{
    bool    isdef;

    IMROTbl *Rdef = IMROTbl::alloc( R->pn );
        Rdef->fillDefault();
        isdef = (*R == *Rdef);
    delete Rdef;

    return isdef;
}


// Return:
// -1: click in forbidden range.
//  0: click in gap, range given.
//  1: extend low  part within given range.
//  2: extend high part within given range.
//
int ShankEditTab::where( int &gap0, int &gapLim, int s, int r )
{
    std::vector<IMRO_ROI>   vT;
    buildTestBoxes( vT, s );

// Default range
    gap0    = 0;
    gapLim  = R->nRow();

    for( int ib = 0, nb = vT.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vT[ib];

        if( B.s < s )
            continue;
        else if( B.s > s )
            break;

        if( B.rLim <= r ) {
            // box fully below click
            gap0 = B.rLim;
        }
        else if( B.r0 > r ) {
            // box fully above
            gapLim = B.r0;
            return 0;
        }
        else if( B.cLim == -2 ) {
            // in forbidden box
            return -1;
        }
        else {
            // in extensible box
            int mid = (B.r0 + B.rLim) / 2;

            if( r <= mid ) {
                gapLim = B.rLim - 1;
                return 1;
            }
            else {
                gap0 = B.r0 + 1;
                return 2;
            }
        }
    }

// Shank clear
    return 0;
}


// Return index of clicked box, or -1.
//
int ShankEditTab::selected( int gap0, int gapLim, int code, int s )
{
    --gap0;
    ++gapLim;

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {
        const IMRO_ROI  &B = vR[ib];
        if( B.s < s )
            continue;
        if( code == 1 && B.rLim == gapLim )
            return ib;
        if( code == 2 && B.r0 == gap0 )
            return ib;
    }

    return -1;
}


void ShankEditTab::clickHere( IMRO_ROI &C, int gap0, int gapLim, int s, int r )
{
    int maxRows = qMin( seTabUI->rowsSB->value(), shortfall( s ) );

    C.s = s;

// If box doesn't fit, then make = gap

    if( maxRows >= gapLim - gap0 ) {
        C.r0   = gap0;
        C.rLim = gapLim;
        return;
    }

// Box fits, center at click

    C.r0   = r - maxRows/2;
    C.rLim = C.r0 + maxRows;

// But slide to stay within gapC.rLim = C.r0 + maxRows;

    if( C.r0 < gap0 ) {
        C.r0   = gap0;
        C.rLim = C.r0 + maxRows;
    }
    else if( C.rLim > gapLim ) {
        C.rLim = gapLim;
        C.r0   = C.rLim - maxRows;
    }
}


// Build sorted list of boxes on this shank for gap checking:
// - Existing ROIs.
// - Ranges of excludes, assembled into boxes.
//
void ShankEditTab::buildTestBoxes( tImroROIs vT, int s )
{
    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        if( B.s == s )
            vT.push_back( B );
    }

    int r0 = -1, nr = 0;

    for( int ix = 0, nx = vX.size(); ix < nx; ++ix ) {

        const IMRO_Site &S = vX[ix];

        if( S.s < s )
            continue;
        else if( S.s > s )
            break;

        if( r0 == -1 ) {
            r0 = S.r;
            nr = 1;
        }
        else if( S.r < r0 + nr )
            ;
        else if( S.r == r0 + nr )
            ++nr;
        else {
            vT.push_back( IMRO_ROI( s, r0, r0 + nr, -1, -2 ) );
            r0 = S.r;
            nr = 1;
        }
    }

    if( r0 >= 0 )
        vT.push_back( IMRO_ROI( s, r0, r0 + nr, -1, -2 ) );

    std::sort( vT.begin(), vT.end() );
}


int ShankEditTab::shortfall( int s )
{
    if( G.nBase == 1 )
        return qMax( (getRqd( 0 ) - getSum( 0 )) / R->nCol_hwr(), 0 );
    else
        return qMax( (getRqd( s ) - getSum( s )) / R->nCol_hwr(), 0 );
}


void ShankEditTab::updateSums( int s )
{
    int nb  = vR.size(),
        sum = 0;

    for( int ib = 0; ib < nb; ++ib ) {
        const IMRO_ROI  &B = vR[ib];
        if( G.nBase == 1 || B.s == s )
            sum += B.rLim - B.r0;
    }

    getSumObj( s )->setText( QString("%1").arg( sum * R->nCol_hwr() ) );
}


int ShankEditTab::getSum( int s )
{
    return getSumObj( s )->text().toInt();
}


int ShankEditTab::getRqd( int s )
{
    return getRqdObj( s )->text().toInt();
}


QLabel *ShankEditTab::getSelObj( int s )
{
    if( G.nBase == 1 )
        return seTabUI->sel0;
    else {
        switch( s ) {
            case 0:  return seTabUI->sel0;
            case 1:  return seTabUI->sel1;
            case 2:  return seTabUI->sel2;
            default: return seTabUI->sel3;
        }
    }
}


QLabel *ShankEditTab::getSumObj( int s )
{
    if( G.nBase == 1 )
        return seTabUI->sum0;
    else {
        switch( s ) {
            case 0:  return seTabUI->sum0;
            case 1:  return seTabUI->sum1;
            case 2:  return seTabUI->sum2;
            default: return seTabUI->sum3;
        }
    }
}


QLabel *ShankEditTab::getRqdObj( int s )
{
    if( G.nBase == 1 )
        return seTabUI->rqd0;
    else {
        switch( s ) {
            case 0:  return seTabUI->rqd0;
            case 1:  return seTabUI->rqd1;
            case 2:  return seTabUI->rqd2;
            default: return seTabUI->rqd3;
        }
    }
}


void ShankEditTab::color()
{
    SC->drawMtx.lock();
        SC->view()->setExcludes( vX );
        SC->view()->setROI( vR );
        SC->update();
    SC->drawMtx.unlock();
}


// Called only from ctor.
//
void ShankEditTab::loadSettings()
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    lastDir = settings.value( "lastDlgDir", QString() ).toString();
}


void ShankEditTab::saveSettings() const
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    settings.setValue( "lastDlgDir", lastDir );
}



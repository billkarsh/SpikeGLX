
#include "ui_ShankEditTab.h"

#include "Util.h"
#include "ShankCtlBase.h"
#include "ShankView.h"
#include "Subset.h"
#include "SignalBlocker.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QSettings>
#include <QThread>


/* ---------------------------------------------------------------- */
/* Click ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankEditTab::Click::down( int s, int c, int r )
{
    this->s = s;
    rclick  = r;

    if( selected( c, r ) )
        se->setSel( s, sel_ib );
    else {
        where( c );

        if( code < 0 )
            se->beep( "Forbidden" );
        else if( !se->isFull( s ) )
            newBox( c );
    }
}


void ShankEditTab::Click::drag( int s, int r )
{
    if( sel_ib < 0 )
        return;

    IMRO_ROI    &B = se->vR[sel_ib];
    int         rView;

    if( s != this->s ) {
        if( code == 1 ) {
            if( sel_edge0 == B.r0 )
                return;
            B.r0 = sel_edge0;
        }
        else {
            if( sel_edge0 == B.rLim )
                return;
            B.rLim = sel_edge0;
        }
        rView = sel_edge0;
    }
    else {
        int del = r - rclick,
            newEdge;
        if( code == 1 ) {
            newEdge = qBound( gap0, sel_edge0 + del, gapLim );
            if( newEdge == B.r0 )
                return;
            B.r0 = newEdge;
        }
        else {
            newEdge = qBound( gap0, sel_edge0 + del, gapLim );
            if( newEdge == B.rLim )
                return;
            B.rLim = newEdge;
        }
        rView = newEdge;
    }

    se->SC->drawMtx.lock();
        se->SC->scroll()->scrollToRow( rView );
    se->SC->drawMtx.unlock();

    se->setSel( this->s, sel_ib );
    se->updateSums( this->s );
    se->color();
}


void ShankEditTab::Click::up()
{
    if( sel_ib >= 0 ) {
        se->updateAll( s, sel_ib );
        sel_ib = -1;    // cancel dragging
    }
}


// Return true if box selected; fill in:
// - sel_ib
// - sel_edge0
// - code {1,2}
// - gap0, gapLim
//
bool ShankEditTab::Click::selected( int c, int r )
{
    sel_ib = -1;

    for( int ib = 0, nb = se->vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = se->vR[ib];

        if( B.s == s && B.containsR( r ) && B.containsC( c ) ) {

            sel_ib = ib;

            int sel_FLR;    // {0,1,2} = {full,LHS,RHS}

            if( B.c0 >= 0 )
                sel_FLR = 2;
            else if( B.cLim >= 0 )
                sel_FLR = 1;
            else
                sel_FLR = 0;

            if( r < B.midPt() ) {
                sel_edge0   = B.r0;
                code        = 1;
                gapLim      = B.rLim - 1;
                sel_set_gap0( sel_FLR );
            }
            else {
                sel_edge0   = B.rLim;
                code        = 2;
                gap0        = B.r0 + 1;
                sel_set_gapLim( sel_FLR );
            }

            return true;
        }
    }

    return false;
}


// Set lower bound for extension of block lower edge.
//
void ShankEditTab::Click::sel_set_gap0( int sel_FLR )
{
    gap0 = 0;

// Scan boxes

    for( int ib = 0, nb = se->vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = se->vR[ib];

        if( ib == sel_ib )
            continue;
        if( B.s != s )
            continue;
        if( sel_FLR == 1 && B.c0 >= 0 )
            continue;
        if( sel_FLR == 2 && B.cLim >= 0 )
            continue;
        if( B.rLim > sel_edge0 )
            continue;
        if( B.rLim > gap0 )
            gap0 = B.rLim;
    }

// Scan vX (sorted)

    const IMRO_ROI  &B = se->vR[sel_ib];

    for( int ix = 0, nx = se->vX.size(); ix < nx; ++ix ) {

        const IMRO_Site &S = se->vX[ix];

        if( S.s < s )
            continue;
        else if( S.s > s )
            break;
        if( !B.containsC( S.c ) )
            continue;
        if( S.r >= sel_edge0 )
            break;
        if( S.r + 1 > gap0 )
            gap0 = S.r + 1;
    }

// Apply Rqd limit

    gap0 = sel_edge0 -
            qMin( se->rowsShortfall( se->vR[sel_ib] ),
            sel_edge0 - gap0 );
}


// Set upper bound for extension of block upper edge.
//
void ShankEditTab::Click::sel_set_gapLim( int sel_FLR )
{
    gapLim = se->R->nRow();

// Scan boxes

    for( int ib = 0, nb = se->vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = se->vR[ib];

        if( ib == sel_ib )
            continue;
        if( B.s != s )
            continue;
        if( sel_FLR == 1 && B.c0 >= 0 )
            continue;
        if( sel_FLR == 2 && B.cLim >= 0 )
            continue;
        if( B.r0 < sel_edge0 )
            continue;
        if( B.r0 < gapLim )
            gapLim = B.r0;
    }

// Scan vX (sorted)

    const IMRO_ROI  &B = se->vR[sel_ib];

    for( int ix = 0, nx = se->vX.size(); ix < nx; ++ix ) {

        const IMRO_Site &S = se->vX[ix];

        if( S.s < s )
            continue;
        else if( S.s > s )
            break;
        if( !B.containsC( S.c ) )
            continue;
        if( S.r < sel_edge0 )
            continue;
        if( S.r < gapLim )
            gapLim = S.r;
        else
            break;
    }

// Apply Rqd limit

    gapLim = sel_edge0 +
                qMin( se->rowsShortfall( se->vR[sel_ib] ),
                gapLim - sel_edge0 );
}


// Fill in:
// - code {-1,0}
// - gap0, gapLim if code 0
//
void ShankEditTab::Click::where( int c )
{
    int click_FLR   = 0,    // {0,1,2} = {full,LHS,RHS}
        h           = se->R->nCol_hwr() / 2;

    if( se->seTabUI->widthCB->currentIndex() )
        click_FLR = (c < h ? 1 : 2);

    std::vector<IMRO_ROI>   vT;
    buildTestBoxes( vT, click_FLR, h );

// Default
    code    = 0;
    gap0    = 0;
    gapLim  = se->R->nRow();

    for( int ib = 0, nb = vT.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vT[ib];

        if( B.s < s )
            continue;
        else if( B.s > s )
            break;

        if( B.rLim <= rclick ) {
            // box fully below click
            gap0 = qMax( gap0, B.rLim );
        }
        else {
            if( B.r0 > rclick ) {
                // box fully above
                gapLim = qMin( gapLim, B.r0 );
            }
            else {
                // in forbidden box
                code = -1;
            }

            return;
        }
    }
}


// Build sorted list of boxes on this shank for gap checking:
// - Existing ROIs.
// - Ranges of excludes, assembled into boxes.
//
// Box ALL/LHS/RHS according to click_FLR.
//
void ShankEditTab::Click::buildTestBoxes( tImroROIs vT, int click_FLR, int h )
{
    for( int ib = 0, nb = se->vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = se->vR[ib];

        if( B.s == s ) {
            // For LHS testing, ignore RHS boxes
            // For RHS testing, ignore LHS boxes
            if( click_FLR == 1 && B.c0 >= 0 )
                continue;
            if( click_FLR == 2 && B.cLim >= 0 )
                continue;
            vT.push_back( B );
        }
    }

// Assemble vX on pertinent colums, into boxes

    int r0 = -1, nr = 0;

    for( int ix = 0, nx = se->vX.size(); ix < nx; ++ix ) {

        const IMRO_Site &S = se->vX[ix];

        if( S.s < s )
            continue;
        else if( S.s > s )
            break;

        if( click_FLR == 1 && S.c >= h )
            continue;
        if( click_FLR == 2 && S.c < h )
            continue;

        if( r0 == -1 ) {
            // new box
            r0 = S.r;
            nr = 1;
        }
        else if( S.r < r0 + nr )
            ;
        else if( S.r == r0 + nr )
            ++nr;
        else {
            int c0 = -1,
                cL = -1;
            if( click_FLR == 1 )
                cL = h;
            else if( click_FLR  == 2 )
                c0 = h;
            vT.push_back( IMRO_ROI( s, r0, r0 + nr, c0, cL ) );
            r0 = S.r;
            nr = 1;
        }
    }

    if( r0 >= 0 ) {
        int c0 = -1,
            cL = -1;
        if( click_FLR == 1 )
            cL = h;
        else if( click_FLR  == 2 )
            c0 = h;
        vT.push_back( IMRO_ROI( s, r0, r0 + nr, c0, cL ) );
    }

    std::sort( vT.begin(), vT.end() );
}


void ShankEditTab::Click::newBox( int c )
{
    IMRO_ROI    B;  // default full shank

    B.s = s;

    if( se->seTabUI->widthCB->currentIndex() ) {
        int h = se->R->nCol_hwr() / 2;
        if( c < h )
            B.cLim = h;
        else
            B.c0   = h;
    }

    int r = qMin( se->seTabUI->rowsSB->value(), se->rowsShortfall( B ) );

    if( r >= gapLim - gap0 ) {
        // If box doesn't fit, then make same size as gap
        B.r0   = gap0;
        B.rLim = gapLim;
    }
    else {
        // Box fits, center at click
        B.r0   = rclick - r/2;
        B.rLim = B.r0 + r;

        // Slide to stay within gap
        if( B.r0 < gap0 ) {
            B.r0   = gap0;
            B.rLim = B.r0 + r;
        }
        else if( B.rLim > gapLim ) {
            B.rLim = gapLim;
            B.r0   = B.rLim - r;
        }
    }

    se->addBox( B );
}

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
        R(IMROTbl::alloc( inR->pn )), click(this)
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
    ConnectUI( seTabUI->saveChansBut, SIGNAL(clicked()), this, SLOT(saveChansBut()) );
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


void ShankEditTab::gridHover( int s, int r, bool quiet )
{
    click.drag( s, r );

    if( quiet )
        return;

    if( s < 0 ) {
        SC->setStatus( "" );
        return;
    }

    SC->setStatus( QString("Grid Row %1 %2")
                    .arg( r )
                    .arg( SC->getLbl( s, r ) ) );
}


void ShankEditTab::gridClicked( int s, int c, int r, bool shift )
{
// Actionable?

    if( !canEdit ) {
        beep( "Non-standard IMRO; can not be edited" );
        return;
    }

// Clear if shift

    if( shift ) {

        for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

            IMRO_ROI    &B = vR[ib];

            if( s == B.s && B.containsR( r ) && B.containsC( c ) ) {

                vR.erase( vR.begin() + ib );
                R->edit_exclude( vX, vR );
                setSel( s, -1 );
                updateSums( s );
                color();
                SC->setStatus( "" );
                return;
            }
        }
        return;
    }

// Handle click

    click.down( s, c, r );
}


void ShankEditTab::lbutReleased()
{
    click.up();
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


// Client expected behavior-
// 0 (modal): Handle like "Save chans" on IM Setup tab.
// 1 (FVW):   notify( "copied result to clipboard" ).
// 2 (SV):    Handle like SVGrafsM_Im.
//
void ShankEditTab::saveChansBut()
{
// ----------
// Any boxes?
// ----------

    int nC = getSum( 0 );
    if( G.nBase == 4 )
        nC += getSum( 1 ) + getSum( 2 ) + getSum( 3 );

    if( !nC ) {
        beep( "Select at least one channel to save" );
        return;
    }

// ----------
// Run dialog
// ----------

    QBitArray   b;
    R->edit_ROI2Bits( b, vR );
    emit SC->runSaveChansDlg( Subset::bits2RngStr( b ) );
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
        if( usageMode() == 0 ) {
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

// 0: Modal,    Config dialog
// 1: Offline,  FVW
// 2: Online,   SV
//
int ShankEditTab::usageMode()
{
    if( seTabUI->okBut->isVisible() )
        return (seTabUI->okBut->text() == "OK" ? 0 : 2);
    return 1;
}


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

    if( R->nCol_hwr() < 2 )
        seTabUI->widthCB->setEnabled( false );

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
    setSel( s, -1 );
    getSumObj( s )->setText( "0" );
    color();
    SC->setStatus( "" );
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


bool ShankEditTab::isDefault()
{
    bool    isdef;

    IMROTbl *Rdef = IMROTbl::alloc( R->pn );
        Rdef->fillDefault();
        isdef = (*R == *Rdef);
    delete Rdef;

    return isdef;
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


bool ShankEditTab::isFull( int s )
{
    if( G.nBase == 1 ) {
        if( getSum( 0 ) >= getRqd( 0 ) ) {
            beep( "All channels set" );
            return true;
        }
    }
    else {
        if( getSum( s ) >= getRqd( s ) ) {
            beep( "All channels on shank set" );
            return true;
        }
    }

    return false;
}


void ShankEditTab::addBox( const IMRO_ROI &B )
{
    vR.push_back( B );
    updateAll( B.s, vR.size() - 1 );
}


void ShankEditTab::updateAll( int s, int ib )
{
    setSel( s, ib );
    updateSums( s );

    if( isDone() )
        vX.clear();
    else
        R->edit_exclude( vX, vR );

    color();
    SC->setStatus( "" );
}


void ShankEditTab::updateSums( int s )
{
    int nb   = vR.size(),
        wmax = R->nCol_hwr(),
        sum  = 0;

    for( int ib = 0; ib < nb; ++ib ) {
        const IMRO_ROI  &B = vR[ib];
        if( G.nBase == 1 || B.s == s )
            sum += B.area( wmax );
    }

    getSumObj( s )->setText( QString("%1").arg( sum ) );
}


int ShankEditTab::rowsShortfall( const IMRO_ROI &B )
{
    int idx  = (G.nBase == 1 ? 0 : B.s),
        wmax = R->nCol_hwr();
    return qMax( (getRqd( idx ) - getSum( idx )) / B.width( wmax ), 0 );
}


int ShankEditTab::getSum( int s )
{
    return getSumObj( s )->text().toInt();
}


int ShankEditTab::getRqd( int s )
{
    return getRqdObj( s )->text().toInt();
}


void ShankEditTab::setSel( int s, int ib )
{
    QLabel  *obj;

    if( G.nBase == 1 )
        obj = seTabUI->sel0;
    else {
        switch( s ) {
            case 0:  obj = seTabUI->sel0; break;
            case 1:  obj = seTabUI->sel1; break;
            case 2:  obj = seTabUI->sel2; break;
            default: obj = seTabUI->sel3;
        }

        if( obj != seTabUI->sel0 )
            seTabUI->sel0->setText( "" );
        if( obj != seTabUI->sel1 )
            seTabUI->sel1->setText( "" );
        if( obj != seTabUI->sel2 )
            seTabUI->sel2->setText( "" );
        if( obj != seTabUI->sel3 )
            seTabUI->sel3->setText( "" );
    }

    int val = -1;

    if( ib >= 0 ) {
        const IMRO_ROI  &B = vR[ib];
        val = B.area( R->nCol_hwr() );
    }

    obj->setText( val >= 0 ? QString("%1").arg( val ) : "" );
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




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

void ShankEditTab::Click::down( int s, int c, int r, bool ctrl )
{
    this->s = s;
    rclick  = r;
    RAW     = 0;

    if( ctrl )
        RAW = 2;
    else if( se->seTabUI->methCB->currentIndex() )
        RAW = 1;

    if( selected( c, r, 1 ) )
        se->setSel( s, sel_ib, 1 );
    else if( selected( c, r, 2 ) )
        se->setSel( s, sel_ib, 2 );
    else if( se->canEdit && (RAW == 0 || (RAW == 1 && se->vA.size())) && selected( c, r, 0 ) )
        se->setSel( s, sel_ib, 0 );
    else if( !se->canEdit && !ctrl ) {
        se->beep( "Non-standard IMRO; can not be edited" );
        return;
    }
    else {
        where( c );

        if( RAW != 1 && code < 0 )
            se->beep( "Forbidden" );
        else if( RAW == 1 && se->vA.size() )
            se->beep( "One where-box at a time" );
        else if( ctrl || RAW == 1 || !se->isFull( s ) )
            newBox( c );
    }
}


void ShankEditTab::Click::drag( int s, int r )
{
    if( sel_ib < 0 )
        return;

    std::vector<IMRO_ROI>   *V;
    switch( RAW ) {
        case 1:  V = &se->vA; break;
        case 2:  V = &se->vW; break;
        default: V = &se->vR; break;
    }

    IMRO_ROI    &B = (*V)[sel_ib];
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

    se->setSel( this->s, sel_ib, RAW );

    switch( RAW ) {
        case 1:  se->SC->view()->setActive( se->vA ); break;
        case 2:  se->SC->view()->setWhere( se->vW, se->vI ); break;
        default: se->updateSums( this->s ); break;
    }

    se->color();
}


void ShankEditTab::Click::up()
{
    if( sel_ib >= 0 ) {
        se->updateRAW( s, sel_ib, RAW );
        sel_ib = -1;    // cancel dragging
    }
}


// Return true if box selected; fill in:
// - sel_ib
// - sel_edge0
// - code {1,2}
// - gap0, gapLim
//
bool ShankEditTab::Click::selected( int c, int r, int RAW )
{
    std::vector<IMRO_ROI>   *V;
    switch( RAW ) {
        case 1:  V = &se->vA; break;
        case 2:  V = &se->vW; break;
        default: V = &se->vR; break;
    }

    sel_ib = -1;

    int nb = (int)V->size();

    for( int ib = 0; ib < nb; ++ib ) {

        const IMRO_ROI  &B = (*V)[ib];

        if( B.s == s && B.containsR( r ) && B.containsC( c ) ) {

            sel_ib      = ib;
            this->RAW   = RAW;

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

    std::vector<IMRO_ROI>   *VB;
    std::vector<IMRO_Site>  *VX;
    switch( RAW ) {
        case 1:  return;
        case 2:  VB = &se->vW; VX = &se->vI; break;
        default: VB = &se->vR; VX = &se->vX; break;
    }

    int nb = (int)VB->size();

    for( int ib = 0; ib < nb; ++ib ) {

        const IMRO_ROI  &B = (*VB)[ib];

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

    const IMRO_ROI  &B = (*VB)[sel_ib];

    for( int ix = 0, nx = (int)VX->size(); ix < nx; ++ix ) {

        const IMRO_Site &S = (*VX)[ix];

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

    if( RAW == 2 ) {
        gap0 = sel_edge0 -
                qMin( se->rowsShortfallW( (*VB)[sel_ib] ),
                sel_edge0 - gap0 );
    }
    else if( RAW == 0 ) {
        gap0 = sel_edge0 -
                qMin( se->rowsShortfall( (*VB)[sel_ib] ),
                sel_edge0 - gap0 );
    }
}


// Set upper bound for extension of block upper edge.
//
void ShankEditTab::Click::sel_set_gapLim( int sel_FLR )
{
    gapLim = se->R->nRow();

// Scan boxes

    std::vector<IMRO_ROI>   *VB;
    std::vector<IMRO_Site>  *VX;
    switch( RAW ) {
        case 1:  return;
        case 2:  VB = &se->vW; VX = &se->vI; break;
        default: VB = &se->vR; VX = &se->vX; break;
    }

    int nb = (int)VB->size();

    for( int ib = 0; ib < nb; ++ib ) {

        const IMRO_ROI  &B = (*VB)[ib];

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

    const IMRO_ROI  &B = (*VB)[sel_ib];

    for( int ix = 0, nx = (int)VX->size(); ix < nx; ++ix ) {

        const IMRO_Site &S = (*VX)[ix];

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

    if( RAW == 2 ) {
        gapLim = sel_edge0 +
                    qMin( se->rowsShortfallW( (*VB)[sel_ib] ),
                    gapLim - sel_edge0 );
    }
    else if( RAW == 0 ) {
        gapLim = sel_edge0 +
                    qMin( se->rowsShortfall( (*VB)[sel_ib] ),
                    gapLim - sel_edge0 );
    }
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

    for( int ib = 0, nb = (int)vT.size(); ib < nb; ++ib ) {

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
    std::vector<IMRO_ROI>   *VB;
    std::vector<IMRO_Site>  *VX;
    switch( RAW ) {
        case 1:  return;
        case 2:  VB = &se->vW; VX = &se->vI; break;
        default: VB = &se->vR; VX = &se->vX; break;
    }

    int nb = (int)VB->size();

    for( int ib = 0; ib < nb; ++ib ) {

        const IMRO_ROI  &B = (*VB)[ib];

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

    int r0 = -1, nr = 0,
        nx = (int)VX->size();

    for( int ix = 0; ix < nx; ++ix ) {

        const IMRO_Site &S = (*VX)[ix];

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

    int r = se->seTabUI->rowsSB->value();

    switch( RAW ) {
        case 0: r = qMin( r, se->rowsShortfall( B ) );  break;
        case 2: r = qMin( r, se->rowsShortfallW( B ) ); break;
        default: break;
    }

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

    se->addRAW( B, RAW );
}

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ShankEditTab::ShankEditTab(
    ShankCtlBase    *SC,
    QWidget         *tab,
    const IMROTbl   *inR )
    :   QObject(0), SC(SC), seTabUI(0), mClear(0), mFill(0),
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

    cVis2Hwr.assign( R->nCol_vis(), 0 );
    for( int chwr = 0, nhwr = R->nCol_hwr(); chwr < nhwr; ++chwr ) {
        cVis2Hwr[R->col2vis_ev()[chwr]] = chwr;
        cVis2Hwr[R->col2vis_od()[chwr]] = chwr;
    }

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

    if( mFill ) {
        delete mFill;
        mFill = 0;
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


void ShankEditTab::gridClicked( int s, int cVis, int r, bool shift, bool ctrl )
{
    int c = cVis2Hwr[cVis];

// Clear if shift

    if( shift ) {

        for( int ib = 0, nb = (int)vA.size(); ib < nb; ++ib ) {

            const IMRO_ROI  &B = vA[ib];

            if( s == B.s && B.containsR( r ) && B.containsC( c ) ) {

                vA.clear();
                seTabUI->methCB->setEnabled( true );
                SC->view()->setActive( vA );
                setSel( s, -1, 0 );
                color();
                SC->setStatus( "" );
                return;
            }
        }

        for( int ib = 0, nb = (int)vW.size(); ib < nb; ++ib ) {

            const IMRO_ROI  &B = vW[ib];

            if( s == B.s && B.containsR( r ) && B.containsC( c ) ) {

                vW.erase( vW.begin() + ib );
                R->edit_exclude( vI, vW );
                SC->view()->setWhere( vW, vI );
                setSel( s, -1, 0 );
                color();
                SC->setStatus( "" );
                return;
            }
        }

        if( !canEdit ) {
            beep( "Non-standard IMRO; can not be edited" );
            return;
        }

        for( int ib = 0, nb = (int)vR.size(); ib < nb; ++ib ) {

            const IMRO_ROI  &B = vR[ib];

            if( s == B.s && B.containsR( r ) && B.containsC( c ) ) {

                vR.erase( vR.begin() + ib );
                R->edit_exclude( vX, vR );
                setSel( s, -1, 0 );
                updateSums( s );
                color();
                SC->setStatus( "" );
                return;
            }
        }
        return;
    }

// Handle click

    click.down( s, c, r, ctrl );
}


void ShankEditTab::lbutReleased()
{
    click.up();
}


bool ShankEditTab::rbutReleased( QPoint globalPos, int s, int cVis, int r )
{
    if( vA.size() &&
        vA[0].s == s &&
        vA[0].containsR( r ) &&
        vA[0].containsC( cVis2Hwr[cVis] ) ) {

        mFill->exec( globalPos );
        return true;
    }

    return false;
}


void ShankEditTab::beep( const QString &msg )
{
    SC->setStatus( msg );

    if( !msg.isEmpty() )
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

    beep( msg );
}


void ShankEditTab::defBut()
{
    vX.clear();
    vA.clear();
    SC->view()->setActive( vA );
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
    vA.clear();
    vR.clear();
    SC->view()->setActive( vA );
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


void ShankEditTab::fillFullLength()
{
    IMRO_ROI    &B = vA[0];

    B.r0    = 0;
    B.rLim  = R->nRow();

    SC->view()->setActive( vA );
    color();
    SC->setStatus( "" );
}


void ShankEditTab::fillAll()
{
    const IMRO_ROI  &B = vA[0];
    int             c0 = B.c_0(),
                    cL = B.c_lim( R->nCol_hwr() ),
                    mx = shortFall( B.s );

    if( !mx ) {
        beep( "Pattern already full" );
        return;
    }

    std::vector<IMRO_Site>  vXme;

    for( int r = B.r0; r < B.rLim; ++r ) {

        for( int c = c0; c < cL; ++c ) {

            if( excludedR( B.s, c, r ) )
                continue;
            if( excludedX( vX, B.s, c, r ) )
                continue;
            if( excludedX( vXme, B.s, c, r ) )
                continue;

            vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

            if( --mx <= 0 )
                goto done;

            R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
            std::sort( vXme.begin(), vXme.end() );
        }
    }

done:
    consolidate();
    finishFill( B.s );
}


void ShankEditTab::fillEven()
{
    const IMRO_ROI  &B = vA[0];
    int             c0 = B.c_0(),
                    cL = B.c_lim( R->nCol_hwr() ),
                    mx = shortFall( B.s );

    if( !mx ) {
        beep( "Pattern already full" );
        return;
    }

    std::vector<IMRO_Site>  vXme;

    for( int r = (B.r0 & 1 ? B.r0 + 1 : B.r0); r < B.rLim; r += 2 ) {

        for( int c = c0; c < cL; ++c ) {

            if( excludedR( B.s, c, r ) )
                continue;
            if( excludedX( vX, B.s, c, r ) )
                continue;
            if( excludedX( vXme, B.s, c, r ) )
                continue;

            vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

            if( --mx <= 0 )
                goto done;

            R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
            std::sort( vXme.begin(), vXme.end() );
        }
    }

done:
    consolidate();
    finishFill( B.s );
}


void ShankEditTab::fillOdd()
{
    const IMRO_ROI  &B = vA[0];
    int             c0 = B.c_0(),
                    cL = B.c_lim( R->nCol_hwr() ),
                    mx = shortFall( B.s );

    if( !mx ) {
        beep( "Pattern already full" );
        return;
    }

    std::vector<IMRO_Site>  vXme;

    for( int r = (B.r0 & 1 ? B.r0 : B.r0 + 1); r < B.rLim; r += 2 ) {

        for( int c = c0; c < cL; ++c ) {

            if( excludedR( B.s, c, r ) )
                continue;
            if( excludedX( vX, B.s, c, r ) )
                continue;
            if( excludedX( vXme, B.s, c, r ) )
                continue;

            vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

            if( --mx <= 0 )
                goto done;

            R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
            std::sort( vXme.begin(), vXme.end() );
        }
    }

done:
    consolidate();
    finishFill( B.s );
}


void ShankEditTab::fillLin1()
{
    const IMRO_ROI  &B = vA[0];
    int             c0 = B.c_0(),
                    h  = R->nCol_hwr(),
                    cL = B.c_lim( h ),
                    mx = shortFall( B.s );

    if( seTabUI->widthCB->currentIndex() ) {
        beep( "'Linear; Row Step 1' applies only to full shank width" );
        return;
    }

    if( !mx ) {
        beep( "Pattern already full" );
        return;
    }

    std::vector<IMRO_Site>  vXme;
    bool                    left = true;

    h /= 2;

    for( int r = B.r0; r < B.rLim; ++r ) {

        int tries = 0;

        if( left ) {
left:
            if( ++tries > 2 )
                continue;
            if( excludedR( B.s, c0, r ) )
                goto right;
            if( excludedX( vX, B.s, c0, r ) )
                goto right;
            if( excludedX( vXme, B.s, c0, r ) )
                goto right;

            left = true;

            for( int c = c0; c < h; ++c ) {

                vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

                if( --mx <= 0 )
                    goto done;

                R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
                std::sort( vXme.begin(), vXme.end() );
            }
        }
        else {
right:
            if( ++tries > 2 )
                continue;
            if( excludedR( B.s, h, r ) )
                goto left;
            if( excludedX( vX, B.s, h, r ) )
                goto left;
            if( excludedX( vXme, B.s, h, r ) )
                goto left;

            left = false;

            for( int c = h; c < cL; ++c ) {

                vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

                if( --mx <= 0 )
                    goto done;

                R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
                std::sort( vXme.begin(), vXme.end() );
            }
        }
    }

done:
    consolidate();
    finishFill( B.s );
}


void ShankEditTab::fillLin2()
{
    const IMRO_ROI  &B = vA[0];
    int             c0 = B.c_0(),
                    h  = R->nCol_hwr(),
                    cL = B.c_lim( h ),
                    mx = shortFall( B.s );

    if( seTabUI->widthCB->currentIndex() ) {
        beep( "'Linear; Row Step 2' applies only to full shank width" );
        return;
    }

    if( !mx ) {
        beep( "Pattern already full" );
        return;
    }

    std::vector<IMRO_Site>  vXme;
    int                     rstep = 1;
    bool                    left  = true;

    h /= 2;

    for( int r = B.r0; r < B.rLim; r += rstep ) {

        int tries = 0;

        if( left ) {
left:
            if( ++tries > 2 ) {
                rstep = 1;
                continue;
            }
            if( excludedR( B.s, c0, r ) )
                goto right;
            if( excludedX( vX, B.s, c0, r ) )
                goto right;
            if( excludedX( vXme, B.s, c0, r ) )
                goto right;

            left  = true;
            rstep = 2;

            for( int c = c0; c < h; ++c ) {

                vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

                if( --mx <= 0 )
                    goto done;

                R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
                std::sort( vXme.begin(), vXme.end() );
            }
        }
        else {
right:
            if( ++tries > 2 ) {
                rstep = 1;
                continue;
            }
            if( excludedR( B.s, h, r ) )
                goto left;
            if( excludedX( vX, B.s, h, r ) )
                goto left;
            if( excludedX( vXme, B.s, h, r ) )
                goto left;

            left  = false;
            rstep = 2;

            for( int c = h; c < cL; ++c ) {

                vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

                if( --mx <= 0 )
                    goto done;

                R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
                std::sort( vXme.begin(), vXme.end() );
            }
        }
    }

done:
    consolidate();
    finishFill( B.s );
}


void ShankEditTab::fillStag1()
{
    const IMRO_ROI  &B = vA[0];
    int             c0 = B.c_0(),
                    h  = R->nCol_hwr(),
                    cL = B.c_lim( h ),
                    mx = shortFall( B.s );

    if( seTabUI->widthCB->currentIndex() ) {
        beep( "'Staggered; Row Step 1' applies only to full shank width" );
        return;
    }

    if( !mx ) {
        beep( "Pattern already full" );
        return;
    }

    std::vector<IMRO_Site>  vXme;
    bool                    left = true;

    h /= 2;

    for( int r = B.r0; r < B.rLim; ++r ) {

        int tries = 0;

        if( left ) {
left:
            if( ++tries > 2 )
                continue;
            if( excludedR( B.s, c0, r ) )
                goto right;
            if( excludedX( vX, B.s, c0, r ) )
                goto right;
            if( excludedX( vXme, B.s, c0, r ) )
                goto right;

            left = false;

            for( int c = c0; c < h; ++c ) {

                vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

                if( --mx <= 0 )
                    goto done;

                R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
                std::sort( vXme.begin(), vXme.end() );
            }
        }
        else {
right:
            if( ++tries > 2 )
                continue;
            if( excludedR( B.s, h, r ) )
                goto left;
            if( excludedX( vX, B.s, h, r ) )
                goto left;
            if( excludedX( vXme, B.s, h, r ) )
                goto left;

            left = true;

            for( int c = h; c < cL; ++c ) {

                vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

                if( --mx <= 0 )
                    goto done;

                R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
                std::sort( vXme.begin(), vXme.end() );
            }
        }
    }

done:
    consolidate();
    finishFill( B.s );
}


void ShankEditTab::fillStag2()
{
    const IMRO_ROI  &B = vA[0];
    int             c0 = B.c_0(),
                    h  = R->nCol_hwr(),
                    cL = B.c_lim( h ),
                    mx = shortFall( B.s );

    if( seTabUI->widthCB->currentIndex() ) {
        beep( "'Staggered; Row Step 2' applies only to full shank width" );
        return;
    }

    if( !mx ) {
        beep( "Pattern already full" );
        return;
    }

    std::vector<IMRO_Site>  vXme;
    int                     rstep = 1;
    bool                    left  = true;

    h /= 2;

    for( int r = B.r0; r < B.rLim; r += rstep ) {

        int tries = 0;

        if( left ) {
left:
            if( ++tries > 2 ) {
                rstep = 1;
                continue;
            }
            if( excludedR( B.s, c0, r ) )
                goto right;
            if( excludedX( vX, B.s, c0, r ) )
                goto right;
            if( excludedX( vXme, B.s, c0, r ) )
                goto right;

            left  = false;
            rstep = 2;

            for( int c = c0; c < h; ++c ) {

                vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

                if( --mx <= 0 )
                    goto done;

                R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
                std::sort( vXme.begin(), vXme.end() );
            }
        }
        else {
right:
            if( ++tries > 2 ) {
                rstep = 1;
                continue;
            }
            if( excludedR( B.s, h, r ) )
                goto left;
            if( excludedX( vX, B.s, h, r ) )
                goto left;
            if( excludedX( vXme, B.s, h, r ) )
                goto left;

            left  = true;
            rstep = 2;

            for( int c = h; c < cL; ++c ) {

                vR.push_back( IMRO_ROI( B.s, r, r+1, c, c+1 ) );

                if( --mx <= 0 )
                    goto done;

                R->edit_exclude_1( vXme, IMRO_Site( B.s, c, r ) );
                std::sort( vXme.begin(), vXme.end() );
            }
        }
    }

done:
    consolidate();
    finishFill( B.s );
}


void ShankEditTab::fillKill()
{
    const IMRO_ROI  &B = vA[0];
    int             c0 = B.c_0(),
                    cL = B.c_lim( R->nCol_hwr() );

    ShankMap    M;
    vR2ShankMap( M );
    std::sort( M.e.begin(), M.e.end() );    // s->r->c

    for( int r = B.r0; r < B.rLim; ++r ) {

        for( int c = c0; c < cL; ++c ) {

            for( int i = 0, n = (int)M.e.size(); i < n; ++i ) {
                ShankMapDesc    &E = M.e[i];
                if( E.s == B.s && E.r == r && E.c == c )
                    M.e.erase( M.e.begin() + i );
            }
        }
    }

    R->edit_shankMap2ROI( vR, M, R->nCol_hwr() );
    finishFill( B.s );
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

        beep( msg );
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

    initMenus();
    initBoxes();

// Reference

    for( int i = 0, nr = (int)G.refs.size(); i < nr; ++i )
        seTabUI->rfCB->addItem( G.refs[i] );

// Gains, highpass

    for( int i = 0, ng = (int)G.gains.size(); i < ng; ++i ) {
        QString s = QString("%1").arg( G.gains[i] );
        seTabUI->apCB->addItem( s );
        seTabUI->lfCB->addItem( s );
    }

    seTabUI->apCB->setEnabled( G.apEnab );
    seTabUI->lfCB->setEnabled( G.lfEnab );
    seTabUI->hpCB->setEnabled( G.hpEnab );

    enableItems( true );
}


void ShankEditTab::initMenus()
{
    QAction *A;

    if( G.nBase == 4 ) {

        mClear = new QMenu();

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

    mFill = new QMenu();

    A = new QAction( "Max Box Length", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillFullLength()) );
    mFill->addAction( A );

    A = new QAction( this );
    A->setSeparator( true );
    mFill->addAction( A );

    A = new QAction( "Fill All Rows", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillAll()) );
    mFill->addAction( A );

    A = new QAction( "Fill Even Rows", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillEven()) );
    mFill->addAction( A );

    A = new QAction( "Fill Odd Rows", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillOdd()) );
    mFill->addAction( A );

    A = new QAction( this );
    A->setSeparator( true );
    mFill->addAction( A );

    A = new QAction( "Linear; Row Step 1", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillLin1()) );
    mFill->addAction( A );

    A = new QAction( "Linear; Row Step 2", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillLin2()) );
    mFill->addAction( A );

    A = new QAction( this );
    A->setSeparator( true );
    mFill->addAction( A );

    A = new QAction( "Staggered; Row Step 1", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillStag1()) );
    mFill->addAction( A );

    A = new QAction( "Staggered; Row Step 2", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillStag2()) );
    mFill->addAction( A );

    A = new QAction( this );
    A->setSeparator( true );
    mFill->addAction( A );

    A = new QAction( "Delete Green", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(fillKill()) );
    mFill->addAction( A );
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

    seTabUI->methCB->setEnabled( enabled );

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
    vA.clear();
    SC->view()->setActive( vA );

    for( int nb = (int)vR.size(), ib = nb - 1; ib >= 0; --ib ) {

        if( s == vR[ib].s )
            vR.erase( vR.begin() + ib );
    }

    R->edit_exclude( vX, vR );
    setSel( s, -1, 0 );
    getSumObj( s )->setText( "0" );
    color();
    SC->setStatus( "" );
}


void ShankEditTab::R2GUI()
{
    vX.clear();
    vA.clear();
    SC->view()->setActive( vA );
    R->edit_tbl2ROI( vR );
    enableItems( R->edit_isCanonical( vR ) );

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
    int rqd = getRqd();

    if( G.nBase == 1 )
        return getSum( 0 ) >= rqd;
    else {
        return getSum( 0 ) >= rqd &&
               getSum( 1 ) >= rqd &&
               getSum( 2 ) >= rqd &&
               getSum( 3 ) >= rqd;
    }
}


bool ShankEditTab::isFull( int s )
{
    if( G.nBase == 1 ) {
        if( getSum( 0 ) >= getRqd() ) {
            beep( "All channels set" );
            return true;
        }
    }
    else {
        if( getSum( s ) >= getRqd() ) {
            beep( "All channels on shank set" );
            return true;
        }
    }

    return false;
}


bool ShankEditTab::excludedR( int s, int c, int r )
{
    for( int ib = 0, nb = (int)vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        if( B.s == s && B.containsR( r ) && B.containsC( c ) )
            return true;
    }

    return false;
}


bool ShankEditTab::excludedX( tconstImroSites vX, int s, int c, int r )
{
    for( int ix = 0, nx = (int)vX.size(); ix < nx; ++ix ) {

        const IMRO_Site &S = vX[ix];

        if( S.s < s )
            continue;
        else if( S.s > s )
            break;
        if( S.r < r )
            continue;
        else if( S.r > r )
            break;
        if( S.c < c )
            continue;
        else if( S.c > c )
            break;

        return true;
    }

    return false;
}


void ShankEditTab::addRAW( const IMRO_ROI &B, int RAW )
{
    std::vector<IMRO_ROI>   *V;
    switch( RAW ) {
        case 1:  V = &vA; seTabUI->methCB->setEnabled( false ); break;
        case 2:  V = &vW; break;
        default: V = &vR; break;
    }

    V->push_back( B );
    updateRAW( B.s, (int)V->size() - 1, RAW );
}


void ShankEditTab::updateRAW( int s, int ib, int RAW )
{
    setSel( s, ib, RAW );

    switch( RAW ) {
        case 1:
            SC->view()->setActive( vA );
            break;
        case 2:
            R->edit_exclude( vI, vW );
            SC->view()->setWhere( vW, vI );
            break;
        default:
            updateSums( s );
            if( isDone() )
                vX.clear();
            else
                R->edit_exclude( vX, vR );
            break;
    }

    color();
    SC->setStatus( "" );
}


void ShankEditTab::updateSums( int s )
{
    int nb   = (int)vR.size(),
        wmax = R->nCol_hwr(),
        sum  = 0;

    for( int ib = 0; ib < nb; ++ib ) {
        const IMRO_ROI  &B = vR[ib];
        if( G.nBase == 1 || B.s == s )
            sum += B.area( wmax );
    }

    getSumObj( s )->setText( QString("%1").arg( sum ) );
}


void ShankEditTab::vR2ShankMap( ShankMap &M )
{
    for( int ib = 0, nb = (int)vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];
        int             c0 = B.c_0(),
                        cL = B.c_lim( R->nCol_hwr() );

        for( int r = B.r0; r < B.rLim; ++r ) {
            for( int c = c0; c < cL; ++c )
                M.e.push_back( ShankMapDesc( B.s, c, r, 1 ) );
        }
    }
}


void ShankEditTab::consolidate()
{
    ShankMap    M;
    vR2ShankMap( M );
    R->edit_shankMap2ROI( vR, M, R->nCol_hwr() );
}


void ShankEditTab::finishFill( int s )
{
    vA.clear();
    SC->view()->setActive( vA );
    seTabUI->methCB->setEnabled( true );

    updateSums( s );
    if( isDone() )
        vX.clear();
    else
        R->edit_exclude( vX, vR );

    color();
    SC->setStatus( "" );
}


int ShankEditTab::shortFall( int s )
{
    int idx = (G.nBase == 1 ? 0 : s);
    return qMax( getRqd() - getSum( idx ), 0 );
}


int ShankEditTab::rowsShortfall( const IMRO_ROI &B )
{
    return shortFall( B.s ) / B.width( R->nCol_hwr() );
}


int ShankEditTab::rowsShortfallW( const IMRO_ROI &B )
{
    int idx  = (G.nBase == 1 ? 0 : B.s),
        wmax = R->nCol_hwr();
    return qMax( (getRqd() - getSumW( idx )) / B.width( wmax ), 0 );
}


int ShankEditTab::getSum( int s )
{
    return getSumObj( s )->text().toInt();
}


int ShankEditTab::getSumW( int s )
{
    int nb   = (int)vW.size(),
        wmax = R->nCol_hwr(),
        sum  = 0;

    for( int ib = 0; ib < nb; ++ib ) {
        const IMRO_ROI  &B = vW[ib];
        if( G.nBase == 1 || B.s == s )
            sum += B.area( wmax );
    }

    return sum;
}


int ShankEditTab::getRqd()
{
    if( G.nBase == 1 )
        return R->nAP();
    else
        return R->nChanPerBank();
}


// RAW {0=R, 1=A, 2=W}
//
void ShankEditTab::setSel( int s, int ib, int RAW )
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
        const IMRO_ROI  *B;
        switch( RAW ) {
            case 1:  B = &vA[ib]; break;
            case 2:  B = &vW[ib]; break;
            default: B = &vR[ib]; break;
        }
        val = B->area( R->nCol_hwr() );
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


void ShankEditTab::color()
{
    QMutexLocker    ml( &SC->drawMtx );
    SC->view()->setROI( vR, vX );
    SC->update();
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



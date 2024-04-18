
#include "ui_ShankEditTab.h"

#include "Util.h"
#include "ShankCtlBase.h"
#include "ShankView.h"
#include "SignalBlocker.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMenu>
#include <QSettings>
#include <QThread>

#define NBOXES  (nBoxes[0] + nBoxes[1] + nBoxes[2] + nBoxes[3])


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
    ConnectUI( seTabUI->bx0CB, SIGNAL(currentIndexChanged(int)), this, SLOT(bx0CBChanged()) );
    ConnectUI( seTabUI->bx1CB, SIGNAL(currentIndexChanged(int)), this, SLOT(bx1CBChanged()) );
    ConnectUI( seTabUI->bx2CB, SIGNAL(currentIndexChanged(int)), this, SLOT(bx2CBChanged()) );
    ConnectUI( seTabUI->bx3CB, SIGNAL(currentIndexChanged(int)), this, SLOT(bx3CBChanged()) );
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


void ShankEditTab::gridClicked( int s, int r )
{
    if( !canEdit ) {
        beep( "Non-standard IMRO; can not be edited" );
        return;
    }

    int nb = vR.size();

// Toggle off

    for( int ib = 0; ib < nb; ++ib ) {

        IMRO_ROI    &B = vR[ib];

        if( s == B.s && r >= B.r0 && r < B.rLim ) {

            vR.erase( vR.begin() + ib );
            R->edit_exclude( vX, vR );
            color();
            return;
        }
    }

// Full?

    if( nBase == 1 ) {
        if( nb >= nBoxes[0] ) {
            beep( "Already enough boxes" );
            return;
        }
    }
    else {
        if( boxesOnShank( s ) >= nBoxes[s] ) {
            beep( "Already enough boxes on shank" );
            return;
        }
    }

// Only one possible?

    if( R->nRow() == grid ) {
        vX.clear();
        R->edit_defaultROI( nBoxes, vR );
        color();
        return;
    }

// Forbidden?

    if( forbidden( s, r ) ) {
        beep( "Forbidden" );
        return;
    }

// Place

    IMRO_ROI    C;
    clickHere( C, s, r );

// Shift?

    if( nBase == 1 ) {
        if( nBoxes[0] > 1 && !fitIntoGap( C ) )
            return;
    }
    else {
        if( nBoxes[s] > 1 && !fitIntoGap( C ) )
            return;
    }

// Add

    vR.push_back( C );

    if( vR.size() >= NBOXES )
        vX.clear();
    else
        R->edit_exclude( vX, vR );

    color();
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
    R->edit_defaultROI( nBoxes, vR );

    enableItems( true, true );

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
    enableItems( true );
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


void ShankEditTab::bx0CBChanged()
{
    bxCBChanged( 0 );
}


void ShankEditTab::bx1CBChanged()
{
    bxCBChanged( 1 );
}


void ShankEditTab::bx2CBChanged()
{
    bxCBChanged( 2 );
}


void ShankEditTab::bx3CBChanged()
{
    bxCBChanged( 3 );
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
    IMRO_GUI    G = R->edit_GUI();
    nBase   = G.nBase;
    grid    = G.grid;

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

    enableItems( true, true );
}


void ShankEditTab::initClearMenu()
{
    if( nBase == 4 ) {

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
    if( nBase == 1 ) {
        nBoxes[0] = 1;
        nBoxes[1] = 0;
        nBoxes[2] = 0;
        nBoxes[3] = 0;
        seTabUI->bxGB->setTitle( "Boxes (whole probe)" );
        seTabUI->bx1CB->hide();
        seTabUI->bx2CB->hide();
        seTabUI->bx3CB->hide();
    }
    else {
        for( int is = 0; is < 4; ++is )
            nBoxes[is] = 1;
    }

    for( int nb = 2, bxrows = boxRows( 0 ); nb <= IMRO_ROI_MAX; nb *= 2 ) {

        if( (bxrows / nb) % grid == 0 ) {

            QString s = QString("%1").arg( nb );

            seTabUI->bx0CB->addItem( s );
            if( nBase == 4 ) {
                seTabUI->bx1CB->addItem( s );
                seTabUI->bx2CB->addItem( s );
                seTabUI->bx3CB->addItem( s );
            }
        }
        else
            break;
    }
}


void ShankEditTab::enableItems( bool enabled, bool onebox )
{
    SignalBlocker   b0(seTabUI->bx0CB),
                    b1(seTabUI->bx1CB),
                    b2(seTabUI->bx2CB),
                    b3(seTabUI->bx3CB);

    canEdit = enabled;

    if( enabled ) {

        if( nBase == 4 ) {
            seTabUI->clearBut->setText( "Clear Boxes" );
            seTabUI->clearBut->setMenu( mClear );
        }

        if( onebox )
            nBoxes[0] = 1;

        seTabUI->bx0CB->setEditable( false );
        seTabUI->bx0CB->setEnabled( true );
        seTabUI->bx0CB->setCurrentIndex(
            seTabUI->bx0CB->findText( QString("%1").arg( nBoxes[0] ) ) );

        if( nBase == 4 ) {

            if( onebox ) {
                nBoxes[1] = 1;
                nBoxes[2] = 1;
                nBoxes[3] = 1;
            }

            seTabUI->bx1CB->setEditable( false );
            seTabUI->bx1CB->setEnabled( true );
            seTabUI->bx1CB->setCurrentIndex(
                seTabUI->bx1CB->findText( QString("%1").arg( nBoxes[1] ) ) );

            seTabUI->bx2CB->setEditable( false );
            seTabUI->bx2CB->setEnabled( true );
            seTabUI->bx2CB->setCurrentIndex(
                seTabUI->bx2CB->findText( QString("%1").arg( nBoxes[2] ) ) );

            seTabUI->bx3CB->setEditable( false );
            seTabUI->bx3CB->setEnabled( true );
            seTabUI->bx3CB->setCurrentIndex(
                seTabUI->bx3CB->findText( QString("%1").arg( nBoxes[3] ) ) );
        }

        IMRO_GUI    G = R->edit_GUI();
        seTabUI->rfCB->setEnabled( true );
        seTabUI->apCB->setEnabled( G.apEnab );
        seTabUI->lfCB->setEnabled( G.lfEnab );
        seTabUI->hpCB->setEnabled( G.hpEnab );
    }
    else {
        if( nBase == 4 ) {
            seTabUI->clearBut->setText( "Clear All Boxes" );
            seTabUI->clearBut->setMenu( 0 );
        }

        seTabUI->bx0CB->setEditable( true );
        seTabUI->bx0CB->setCurrentText( "NA" );
        seTabUI->bx0CB->setEnabled( false );

        if( nBase == 4 ) {
            seTabUI->bx1CB->setEditable( true );
            seTabUI->bx1CB->setCurrentText( "NA" );
            seTabUI->bx1CB->setEnabled( false );

            seTabUI->bx2CB->setEditable( true );
            seTabUI->bx2CB->setCurrentText( "NA" );
            seTabUI->bx2CB->setEnabled( false );

            seTabUI->bx3CB->setEditable( true );
            seTabUI->bx3CB->setCurrentText( "NA" );
            seTabUI->bx3CB->setEnabled( false );
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
    color();
}


int ShankEditTab::boxRows( int s )
{
    int bxrows = R->nChanPerBank() / R->nCol_hwr();

    switch( nBoxes[nBase == 4 ? s : 0] ) {
        case 64: bxrows /= 2;
        case 32: bxrows /= 2;
        case 16: bxrows /= 2;
        case 8:  bxrows /= 2;
        case 4:  bxrows /= 2;
        case 2:  bxrows /= 2;
        default: break;
    }

    return bxrows;
}


int ShankEditTab::boxesOnShank( int s )
{
    int N = 0;

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {
        if( vR[ib].s == s )
            ++N;
    }

    return N;
}


void ShankEditTab::bxCBChanged( int s )
{
    int ui;

    switch( s ) {
        case 0: ui = seTabUI->bx0CB->currentText().toInt(); break;
        case 1: ui = seTabUI->bx1CB->currentText().toInt(); break;
        case 2: ui = seTabUI->bx2CB->currentText().toInt(); break;
        case 3: ui = seTabUI->bx3CB->currentText().toInt(); break;
    }

    if( ui < nBoxes[s] && vR.size() ) {

        int yesNo = QMessageBox::warning(
            SC,
            "Smaller Box Count",
            "Reducing the box count will clear current boxes...\n\n"
            "Reduce count anyway?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No );

        if( yesNo != QMessageBox::Yes ) {
            QString str = QString("%1").arg( nBoxes[s] );
            switch( s ) {
                case 0:
                {
                    SignalBlocker   b0(seTabUI->bx0CB);
                    seTabUI->bx0CB->setCurrentIndex( seTabUI->bx0CB->findText( str ) );
                    break;
                }
                case 1:
                {
                    SignalBlocker   b1(seTabUI->bx1CB);
                    seTabUI->bx1CB->setCurrentIndex( seTabUI->bx1CB->findText( str ) );
                    break;
                }
                case 2:
                {
                    SignalBlocker   b2(seTabUI->bx2CB);
                    seTabUI->bx2CB->setCurrentIndex( seTabUI->bx2CB->findText( str ) );
                    break;
                }
                case 3:
                {
                    SignalBlocker   b3(seTabUI->bx3CB);
                    seTabUI->bx3CB->setCurrentIndex( seTabUI->bx3CB->findText( str ) );
                    break;
                }
            }
            return;
        }

        if( nBase == 1 )
            clearAll();
        else
            clearShank( s );

        return;
    }

    do {

        for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

            const IMRO_ROI  &B  = vR[ib];

            if( nBase == 4 && B.s != s )
                continue;

            int mid = (B.r0 + B.rLim) / 2;

            vR.push_back( IMRO_ROI( B.s, mid, B.rLim ) );
            vR[ib].rLim = mid;
        }

    } while( (nBoxes[s] *= 2) < ui );

    nBoxes[s] = ui;
    color();
}


void ShankEditTab::R2GUI()
{
    R->edit_tbl2ROI( nBoxes, vR );
    enableItems( R->edit_isCanonical( nBoxes, vR ) );

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
    int rem = NBOXES - vR.size();

    if( rem > 0 ) {
        beep( QString("Click %1 more box%2")
                .arg( rem ).arg( rem > 1 ? "es" : "" ) );
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


bool ShankEditTab::forbidden( int s, int r )
{
    for( int ix = 0, nx = vX.size(); ix < nx; ++ix ) {

        const IMRO_Site &S = vX[ix];

        if( S.s < s )
            continue;
        else if( S.s > s )
            return false;

        if( S.r < r )
            continue;
        else if( S.r > r )
            return false;

        return true;    // same (shank,row)
    }

    return false;
}


void ShankEditTab::clickHere( IMRO_ROI &C, int s, int r )
{
    int rem, bxrows = boxRows( s );

    C.s     = s;
    C.r0    = r - bxrows/2;

    if( C.r0 <= 0 )
        C.r0 = 0;
    else if( (rem = C.r0 % grid) )
        C.r0 -= rem;

    C.rLim = C.r0 + bxrows;

    if( C.rLim >= R->nRow() ) {
        C.rLim = R->nRow();
        C.r0 = C.rLim - bxrows;
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
            vT.push_back( IMRO_ROI( s, r0, r0 + nr ) );
            r0 = S.r;
            nr = 1;
        }
    }

    if( r0 >= 0 )
        vT.push_back( IMRO_ROI( s, r0, r0 + nr ) );

    qSort( vT.begin(), vT.end() );
}


// Return true if room.
//
bool ShankEditTab::fitIntoGap( IMRO_ROI &C )
{
    std::vector<IMRO_ROI>   vT;
    int                     bxrows = boxRows( C.s );
    buildTestBoxes( vT, C.s );

    for( int ib = 0, nb = vT.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vT[ib];

        if( boxOverlapsMe( C, B ) ) {

            if( boxIsBelowMe( C, B ) ) {

                C.r0    = B.rLim;
                C.rLim  = C.r0 + bxrows;
                ++ib;

                if( C.rLim > R->nRow() ||
                    (ib < nb && boxOverlapsMe( C, vT[ib] )) ) {

                    beep( "No room" );
                    return false;
                }
            }
            else {
                C.rLim  = B.r0;
                C.r0    = C.rLim - bxrows;
                --ib;

                if( C.r0 < 0 ||
                    (ib >= 0 && boxOverlapsMe( C, vT[ib] )) ) {

                    beep( "No room" );
                    return false;
                }
            }

            break;
        }
        else if( C.rLim < B.r0 )
            break;
    }

    return true;
}


bool ShankEditTab::boxOverlapsMe( const IMRO_ROI &Me, const IMRO_ROI &B )
{
    return Me.r0 < B.rLim && Me.rLim > B.r0;
}


bool ShankEditTab::boxIsBelowMe( const IMRO_ROI &Me, const IMRO_ROI &B )
{
    return B.r0 <= Me.r0;
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



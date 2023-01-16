
#include "ui_ShankEditTab.h"

#include "Util.h"
#include "ShankCtlBase.h"
#include "ShankView.h"
#include "SignalBlocker.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ShankEditTab::ShankEditTab(
    ShankCtlBase    *SC,
    QWidget         *tab,
    const IMROTbl   *inR )
    :   QObject(0), SC(SC), seTabUI(0),
        R0(IMROTbl::alloc( inR->type )),
        Rfile(IMROTbl::alloc( inR->type )),
        R(IMROTbl::alloc( inR->type ))
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
    ConnectUI( seTabUI->clearBut, SIGNAL(clicked()), this, SLOT(clearBut()) );
    ConnectUI( seTabUI->bxCB, SIGNAL(currentIndexChanged(int)), this, SLOT(bxCBChanged()) );
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


void ShankEditTab::gridHover( int s, int c, int r )
{
    if( s < 0 ) {
        SC->setStatus( "" );
        return;
    }

    SC->setStatus( QString("Grid Row %1").arg( r ) );
}


void ShankEditTab::gridClicked( int s, int c, int r )
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

// Only one possible?

    if( R->nRow() == grid ) {
        vX.clear();
        nBoxes = R->edit_defaultROI( vR );
        color();
        return;
    }

// Full?

    if( nb >= nBoxes ) {
        beep( "Already enough boxes" );
        return;
    }

// Forbidden?

    if( forbidden( s, c, r ) ) {
        beep( "Forbidden" );
        return;
    }

// Place

    IMRO_ROI    C;
    clickHere( C, s, r );

// Shift?

    if( nBoxes > 1 && !fitIntoGap( C ) )
        return;

// Add

    vR.push_back( C );

    if( vR.size() >= nBoxes )
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
        SignalBlocker   b0(seTabUI->bxCB);
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
    SignalBlocker   b0(seTabUI->bxCB);

    vX.clear();

    enableItems( true );
    seTabUI->bxCB->setCurrentIndex( 0 );
    nBoxes = R->edit_defaultROI( vR );
    setBoxRows();

    IMRO_Attr   A = R->edit_Attr_def();
    seTabUI->rfCB->setCurrentIndex( A.refIdx );
    seTabUI->apCB->setCurrentIndex( A.apgIdx );
    seTabUI->lfCB->setCurrentIndex( A.lfgIdx );
    seTabUI->hpCB->setCurrentIndex( A.hpfIdx );
    color();
    canEdit = true;
}


void ShankEditTab::clearBut()
{
    vX.clear();
    vR.clear();
    enableItems( true );
    nBoxes = seTabUI->bxCB->currentText().toInt();
    setBoxRows();
    color();
    canEdit = true;
}


void ShankEditTab::bxCBChanged()
{
    int ui = seTabUI->bxCB->currentText().toInt();

    if( ui < nBoxes && vR.size() ) {

        int yesNo = QMessageBox::warning(
            SC,
            "Smaller Box Count",
            "Reducing the box count will clear current boxes...\n\n"
            "Reduce count anyway?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No );

        if( yesNo != QMessageBox::Yes ) {
            SignalBlocker   b0(seTabUI->bxCB);
            seTabUI->bxCB->setCurrentIndex(
                seTabUI->bxCB->findText( QString("%1").arg( nBoxes ) ) );
            return;
        }

        clearBut();
        return;
    }

    do {

        for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

            const IMRO_ROI  &B  = vR[ib];
            int             mid = (B.r0 + B.rLim) / 2;

            vR.push_back( IMRO_ROI( B.s, mid, B.rLim ) );
            vR[ib].rLim = mid;
        }

    } while( (nBoxes *= 2) < ui );

    nBoxes = ui;
    setBoxRows();
    color();
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
            emit SC->modal_done( SC, fn, false );
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

    seTabUI->prbLbl->setText( QString("%1").arg( R->type ) );

// Boxes

    grid    = G.grid;
    nBoxes  = 1;
    setBoxRows();

    int bxrows = boxRows / 2;

    for( int nb = 2; nb <= 8; nb *= 2, bxrows /= 2 ) {

        if( bxrows % grid == 0 )
            seTabUI->bxCB->addItem( QString("%1").arg( nb ) );
        else
            break;
    }

    seTabUI->bxCB->setCurrentIndex( 0 );

// Reference

    int ntip = R->nShank();

    if( ntip == 1 )
        seTabUI->rfCB->addItem( "Tip" );
    else {
        for( int i = 0; i < ntip; ++i )
            seTabUI->rfCB->addItem( QString("Tip %1").arg( i ) );
        seTabUI->rfCB->addItem( "Join Tips" );
    }

// Gains, highpass

    for( int i = 0, ng = G.gains.size(); i < ng; ++i ) {
        QString s = QString("%1").arg( G.gains[i] );
        seTabUI->apCB->addItem( s );
        seTabUI->lfCB->addItem( s );
    }

    seTabUI->apCB->setEnabled( G.apEnab );
    seTabUI->lfCB->setEnabled( G.lfEnab );
    seTabUI->hpCB->setEnabled( G.hpEnab );

    canEdit = true;
}


void ShankEditTab::enableItems( bool enabled )
{
    if( enabled ) {
        IMRO_GUI    G = R->edit_GUI();
        seTabUI->bxCB->setEditable( false );
        seTabUI->bxCB->setEnabled( true );
        seTabUI->rfCB->setEnabled( true );
        seTabUI->apCB->setEnabled( G.apEnab );
        seTabUI->lfCB->setEnabled( G.lfEnab );
        seTabUI->hpCB->setEnabled( G.hpEnab );
    }
    else {
        seTabUI->bxCB->setEditable( true );
        seTabUI->bxCB->setCurrentText( "NA" );
        seTabUI->bxCB->setEnabled( false );
        seTabUI->rfCB->setEnabled( false );
        seTabUI->apCB->setEnabled( false );
        seTabUI->lfCB->setEnabled( false );
        seTabUI->hpCB->setEnabled( false );
    }
}


void ShankEditTab::setBoxRows()
{
    boxRows = R->nAP() / R->nCol_smap();

    switch( nBoxes ) {
        case 8: boxRows /= 2;
        case 4: boxRows /= 2;
        case 2: boxRows /= 2;
        default: break;
    }
}


void ShankEditTab::R2GUI()
{
    int sel;

    nBoxes  = R->edit_tbl2ROI( vR );
    canEdit =
        R->edit_isCanonical( vR ) &&
        (sel = seTabUI->bxCB->findText( QString("%1").arg( nBoxes ) )) >= 0;

    vX.clear();

    if( canEdit ) {
        SignalBlocker   b0(seTabUI->bxCB);
        enableItems( true );
        seTabUI->bxCB->setCurrentIndex( sel );
        setBoxRows();
    }
    else {
        enableItems( false );
        beep( "Non-standard IMRO; can not be edited" );
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
    int nb  = vR.size(),
        rem = nBoxes - nb;

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
    IMROTbl *Rdef = IMROTbl::alloc( R->type );
        Rdef->fillDefault();
        bool    isdef = (*R == *Rdef);
    delete Rdef;

    return isdef;
}


bool ShankEditTab::forbidden( int s, int c, int r )
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

        if( S.c < c )
            continue;

        return S.c == c;
    }

    return false;
}


void ShankEditTab::clickHere( IMRO_ROI &C, int s, int r )
{
    int rem;

    C.s     = s;
    C.r0    = r - boxRows/2;

    if( C.r0 <= 0 )
        C.r0 = 0;
    else if( (rem = C.r0 % grid) )
        C.r0 -= rem;

    C.rLim = C.r0 + boxRows;

    if( C.rLim >= R->nRow() ) {
        C.rLim = R->nRow();
        C.r0 = C.rLim - boxRows;
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

        if( S.c != 0 )
            continue;

        if( r0 == -1 ) {
            r0 = S.r;
            nr = 1;
        }
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
    buildTestBoxes( vT, C.s );

    for( int ib = 0, nb = vT.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vT[ib];

        if( hitBoxBelow( C, B ) ) {

            C.r0    = B.rLim;
            C.rLim  = C.r0 + boxRows;
            ++ib;

            if( C.rLim > R->nRow() ||
                (ib < nb && hitBoxAbove( C, vT[ib] )) ) {

                beep( "No room" );
                return false;
            }
        }
        else if( hitBoxAbove( C, B ) ) {

            C.rLim  = B.r0;
            C.r0    = C.rLim - boxRows;
            --ib;

            if( C.r0 < 0 ||
                (ib >= 0 && hitBoxBelow( C, vT[ib] )) ) {

                beep( "No room" );
                return false;
            }

            break;
        }
        else if( C.rLim < B.r0 )
            break;
    }

    return true;
}


bool ShankEditTab::hitBoxAbove( const IMRO_ROI &C, const IMRO_ROI &A )
{
    return C.rLim > A.r0 && C.rLim <= A.rLim;
}


bool ShankEditTab::hitBoxBelow( const IMRO_ROI &C, const IMRO_ROI &B )
{
    return C.r0 >= B.r0 && C.r0 < B.rLim;
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



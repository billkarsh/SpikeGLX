
#include "ui_FVW_ShankWindow.h"

#include "Util.h"
#include "MainApp.h"
#include "FVW_ShankCtl.h"
#include "FileViewerWindow.h"
#include "DataFile.h"
#include "Subset.h"
#include "Biquad.h"
#include "SignalBlocker.h"

#include <QAction>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* class Tally ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVW_ShankCtl::Tally::init( int maxInt, int nNu )
{
    this->maxInt    = maxInt;
    this->nNu       = nNu;
}


void FVW_ShankCtl::Tally::zeroData()
{
    vmin.assign( nNu,  99000 );
    vmax.assign( nNu, -99000 );
    sums.assign( nNu,  0 );
    sumSamps = 0;
}


void FVW_ShankCtl::Tally::accumSpikes(
    const short *data,
    int         ntpts,
    int         nchans,
    int         c0,
    int         cLim,
    int         thresh,
    int         inarow )
{
    if( !ntpts )
        return;

    double  v = maxInt * thresh * 1e-6 / VMAX;

    sumSamps += ntpts;

    for( int c = c0; c < cLim; ++c ) {

        const short *d      = &data[c],
                    *dlim   = &data[c + ntpts*nchans];

        int i       = c - c0,
            spikes  = 0,
            T,
            hiCnt;

        T = v * df->ig2Gain( i );

        hiCnt = (*d <= T ? inarow : 0);

        while( (d += nchans) < dlim ) {

            if( *d <= T ) {

                if( ++hiCnt == inarow )
                    ++spikes;
            }
            else
                hiCnt = 0;
        }

        sums[i] += spikes;
    }
}


void FVW_ShankCtl::Tally::normSpikes()
{
    double  count2Rate = df->samplingRateHz() / sumSamps;

    for( int i = 0; i < nNu; ++i )
        sums[i] *= count2Rate;
}


void FVW_ShankCtl::Tally::accumPkPk(
    const short *data,
    int         ntpts,
    int         nchans,
    int         c0,
    int         cLim )
{
    if( !ntpts )
        return;

    for( int it = 0; it < ntpts; ++it, data += nchans ) {

        for( int c = c0; c < cLim; ++c ) {

            int v = data[c],
                i = c - c0;

            if( v < vmin[i] )
                vmin[i] = v;

            if( v > vmax[i] )
                vmax[i] = v;
        }
    }
}


void FVW_ShankCtl::Tally::normPkPk()
{
    double  ysc = 1e6 * VMAX / maxInt;

    for( int i = 0; i < nNu; ++i )
        sums[i] = (vmax[i] - vmin[i]) * ysc / df->ig2Gain( i );
}

/* ---------------------------------------------------------------- */
/* FVW_ShankCtl --------------------------------------------------- */
/* ---------------------------------------------------------------- */

FVW_ShankCtl::FVW_ShankCtl( const DataFile *df, QWidget *parent )
    :   QDialog(parent), VMAX(df->vRange().rmax), df(df), scUI(0),
        chanMap(df->chanMap()), tly(df, VMAX), hipass(0), lopass(0),
        maxInt(0), nC(df->numChans()), lfp(df->subtypeFromObj()=="imec.lf")
{
    setWindowFlag( Qt::Tool, true );
}


FVW_ShankCtl::~FVW_ShankCtl()
{
    drawMtx.lock();
        if( hipass ) {
            delete hipass;
            hipass = 0;
        }
        if( lopass ) {
            delete lopass;
            lopass = 0;
        }
    drawMtx.unlock();

    if( chanMap ) {
        delete chanMap;
        chanMap = 0;
    }

    if( scUI ) {
        delete scUI;
        scUI = 0;
    }

    emit closed( this );
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVW_ShankCtl::mapChanged( const ShankMap *map )
{
    nNu = map->e.size();
    scUI->scroll->theV->setShankMap( map );
    tly.init( maxInt, nNu );
}


void FVW_ShankCtl::showDialog()
{
    showNormal();
    scUI->scroll->scrollToSelected();
    mainApp()->modelessOpened( this );
}


void FVW_ShankCtl::selChan( int ig )
{
    const ShankMap  *M = scUI->scroll->theV->getSmap();

    if( M && ig < int(M->e.size()) ) {

        int ic = df->channelIDs()[ig];

        scUI->scroll->theV->setSel( ig );
        scUI->chanBut->setText( chanMap->name( ig, df->trig_isChan( ic ) ) );
    }
}


void FVW_ShankCtl::putInit()
{
    tly.zeroData();

// FVW allows random access to file so we
// invalidate filter history on each draw.

    nzero = BIQUAD_TRANS_WIDE;
}


void FVW_ShankCtl::putScans( const vec_i16 &_data )
{
    int ntpts = int(_data.size()) / nC;

// -----------------------------
// Make local copy we can filter
// -----------------------------

    vec_i16 data;
    Subset::subsetBlock( data, *(vec_i16*)&_data, 0, nNu, nC );

    hipass->applyBlockwiseMem( &data[0], maxInt, ntpts, nNu, 0, nNu );

    if( lopass )
        lopass->applyBlockwiseMem( &data[0], maxInt, ntpts, nNu, 0, nNu );

    zeroFilterTransient( &data[0], ntpts, nNu );

// --------------------------
// Process current data chunk
// --------------------------

    if( set.what == 0 )
        tly.accumSpikes( &data[0], ntpts, nNu, 0, nNu, set.thresh, set.inarow );
    else
        tly.accumPkPk( &data[0], ntpts, nNu, 0, nNu );
}


void FVW_ShankCtl::putDone()
{
    if( !set.what )
        tly.normSpikes();
    else
        tly.normPkPk();

    color();
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVW_ShankCtl::cursorOver( int ig )
{
    if( ig < 0 ) {
        scUI->statusLbl->setText( QString() );
        return;
    }

    int r  = scUI->scroll->theV->getSmap()->e[ig].r,
        ic = df->channelIDs()[ig];

    scUI->statusLbl->setText(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( chanMap->name( ig, df->trig_isChan( ic ) ) ) );
}


void FVW_ShankCtl::lbutClicked( int ig )
{
    cursorOver( ig );
    emit selChanged( ig );
}


void FVW_ShankCtl::ypixChanged( int y )
{
    set.yPix = y;
    scUI->scroll->setRowPix( y );
    saveSettings();
}


void FVW_ShankCtl::whatChanged( int i )
{
    drawMtx.lock();
        set.what = i;
        SignalBlocker   b0(scUI->rngSB);
        scUI->TSB->setEnabled( !i );
        scUI->inarowSB->setEnabled( !i );
        scUI->rngSB->setValue( set.rng[i] );
        updateFilter( false );
    drawMtx.unlock();
    saveSettings();
    emit feedMe();
}


void FVW_ShankCtl::threshChanged( int t )
{
    drawMtx.lock();
        set.thresh = -t;
    drawMtx.unlock();
    saveSettings();
}


void FVW_ShankCtl::inarowChanged( int s )
{
    drawMtx.lock();
        set.inarow = s;
    drawMtx.unlock();
    saveSettings();
}


void FVW_ShankCtl::rangeChanged( int r )
{
    drawMtx.lock();
        set.rng[set.what] = r;
        color();
    drawMtx.unlock();
    saveSettings();
}


void FVW_ShankCtl::chanBut()
{
    scUI->scroll->scrollToSelected();
}


void FVW_ShankCtl::helpBut()
{
    showHelp( "ShankView_Help" );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void FVW_ShankCtl::baseInit( const ShankMap *map )
{
    loadSettings();

    scUI = new Ui::FVW_ShankWindow;
    scUI->setupUi( this );

    scUI->scroll->theV->setRowPix( set.yPix );

    scUI->ypixSB->installEventFilter( this );
    scUI->ypixSB->setValue( set.yPix );

    scUI->whatCB->setCurrentIndex( set.what );
    scUI->whatCB->setEnabled( !lfp );

    scUI->TSB->installEventFilter( this );
    scUI->TSB->setValue( -set.thresh );
    scUI->TSB->setEnabled( set.what == 0 );

    scUI->inarowSB->installEventFilter( this );
    scUI->inarowSB->setValue( set.inarow );
    scUI->inarowSB->setEnabled( set.what == 0 );

    scUI->rngSB->installEventFilter( this );
    scUI->rngSB->setValue( set.rng[set.what] );

    ConnectUI( scUI->scroll->theV, SIGNAL(cursorOver(int,bool)), this, SLOT(cursorOver(int)) );
    ConnectUI( scUI->scroll->theV, SIGNAL(lbutClicked(int,bool)), this, SLOT(lbutClicked(int)) );
    ConnectUI( scUI->ypixSB, SIGNAL(valueChanged(int)), this, SLOT(ypixChanged(int)) );
    ConnectUI( scUI->whatCB, SIGNAL(currentIndexChanged(int)), this, SLOT(whatChanged(int)) );
    ConnectUI( scUI->TSB, SIGNAL(valueChanged(int)), this, SLOT(threshChanged(int)) );
    ConnectUI( scUI->inarowSB, SIGNAL(valueChanged(int)), this, SLOT(inarowChanged(int)) );
    ConnectUI( scUI->rngSB, SIGNAL(valueChanged(int)), this, SLOT(rangeChanged(int)) );
    ConnectUI( scUI->chanBut, SIGNAL(clicked()), this, SLOT(chanBut()) );
    ConnectUI( scUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );

    updateFilter( true );

    setAttribute( Qt::WA_DeleteOnClose, false );
    restoreScreenState();

    QString type = df->fileLblFromObj();
    type.front() = type.front().toUpper();
    setWindowTitle( QString("%1 Shank Activity (Offline)").arg( type ) );

    mapChanged( map );
    selChan( 0 );
}


// Force Ctrl+A events to be treated as 'show audio dialog',
// instead of 'text-field select-all'.
//
bool FVW_ShankCtl::eventFilter( QObject *watched, QEvent *event )
{
    if( event->type() == QEvent::KeyPress ) {

        QKeyEvent   *e = static_cast<QKeyEvent*>(event);

        if( e->modifiers() == Qt::ControlModifier ) {

            if( e->key() == Qt::Key_A ) {
                mainApp()->act.aoDlgAct->trigger();
                e->ignore();
                return true;
            }
        }
    }

    return QDialog::eventFilter( watched, event );
}


void FVW_ShankCtl::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
    }
    else
        QDialog::keyPressEvent( e );
}


void FVW_ShankCtl::closeEvent( QCloseEvent *e )
{
    QDialog::closeEvent( e );

    if( e->isAccepted() ) {

        saveScreenState();
        emit closed( this );
    }
}


// Note:
// restoreScreenState() must be called after initializing
// a window's controls with setupUI().
//
void FVW_ShankCtl::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( screenStateName() ).toByteArray() ) ) {

        // Get size from form, or do nothing.
    }
}


void FVW_ShankCtl::saveScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( screenStateName(), saveGeometry() );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVW_ShankCtl::updateFilter( bool lock )
{
    if( lock )
        drawMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( lopass ) {
        delete lopass;
        lopass = 0;
    }

    double  srate = df->samplingRateHz();

    if( set.what < 2 )
        hipass = new Biquad( bq_type_highpass, 300/srate );
    else {
        // LFP
        hipass = new Biquad( bq_type_highpass, 0.2/srate );
        lopass = new Biquad( bq_type_lowpass,  300/srate );
    }

    if( lock )
        drawMtx.unlock();
}


void FVW_ShankCtl::zeroFilterTransient( short *data, int ntpts, int nchans )
{
    if( nzero > 0 ) {

        // overwrite with zeros

        if( ntpts > nzero )
            ntpts = nzero;

        memset( data, 0, ntpts*nchans*sizeof(qint16) );
        nzero -= ntpts;
    }
}


void FVW_ShankCtl::color()
{
    scUI->scroll->theV->colorPads( tly.sums, set.rng[set.what] );
    update();
}


void FVW_ShankCtl::update()
{
    scUI->scroll->theV->updateNow();

//    QMetaObject::invokeMethod(
//        scUI->scroll->theV,
//        "updateNow",
//        Qt::QueuedConnection );
}


void FVW_ShankCtl::dcAve(
    std::vector<int>    &ave,
    short               *data,
    int                 maxInt,
    int                 ntpts,
    int                 nchans,
    int                 c0,
    int                 cLim )

{
    int                 nC = cLim - c0;
    std::vector<float>  sum( nC, 0.0F );

    for( int it = 0; it < ntpts; ++it, data += nchans ) {

        for( int c = c0; c < cLim; ++c )
            sum[c-c0] += data[c];
    }

    ave.resize( nC );

    for( int c = 0; c < nC; ++c )
        ave[c] = qBound( -maxInt, int(sum[c]/ntpts), maxInt - 1 );
}



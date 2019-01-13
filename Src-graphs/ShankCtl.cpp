
#include "ui_ShankWindow.h"

#include "Util.h"
#include "MainApp.h"
#include "ShankCtl.h"
#include "DAQ.h"
#include "ShankMap.h"
#include "Biquad.h"
#include "SignalBlocker.h"

#include <QAction>
#include <QCloseEvent>
#include <QSettings>

/* ---------------------------------------------------------------- */
/* class Tally ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtl::Tally::init( double sUpdt, int ip )
{
    this->ip = ip;

    if( ip >= 0 )
        nPads = p.im.each[ip].imCumTypCnt[CimCfg::imSumAP];
    else
        nPads = p.ni.niCumTypCnt[CniCfg::niSumNeural];

    updtChanged( sUpdt );
}


#define SEC_PER_FETCH   0.1

void ShankCtl::Tally::updtChanged( double s )
{
    chunksReqd = qMax( 1, int(s / SEC_PER_FETCH) );
    zeroData();
}


void ShankCtl::Tally::zeroData()
{
    vmin.fill(  99000, nPads );
    vmax.fill( -99000, nPads );
    sums.fill( 0, nPads );
    sumSamps    = 0;
    chunksDone  = 0;
}


bool ShankCtl::Tally::countSpikes(
    const short *data,
    int         ntpts,
    int         nchans,
    int         c0,
    int         cLim,
    int         thresh,
    int         inarow )
{
    if( !ntpts )
        return false;

    sumSamps += ntpts;

    for( int c = c0; c < cLim; ++c ) {

        const short *d      = &data[c],
                    *dlim   = &data[c + ntpts*nchans];
        int i       = c - c0,
            T       = (ip >= 0 ?
                        p.im.vToInt10( thresh*1e-6, ip, i ) :
                        p.ni.vToInt16( thresh*1e-6, i ));
        int hiCnt   = (*d <= T ? inarow : 0),
            spikes  = 0;

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

    bool    done = ++chunksDone >= chunksReqd;

    if( done ) {

        double  count2Rate =
                (ip >= 0 ? p.im.each[ip].srate : p.ni.srate) / sumSamps;

        for( int i = 0; i < nPads; ++i )
            sums[i] *= count2Rate;
    }

    return done;
}


bool ShankCtl::Tally::accumPkPk(
    const short *data,
    int         ntpts,
    int         nchans,
    int         c0,
    int         cLim )
{
    if( !ntpts )
        return false;

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

    bool    done = ++chunksDone >= chunksReqd;

    if( done ) {

        for( int i = 0; i < nPads; ++i )
            sums[i] = vmax[i] - vmin[i];
    }

    return done;
}

/* ---------------------------------------------------------------- */
/* ShankCtl ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankCtl::ShankCtl( const DAQ::Params &p, int jpanel, QWidget *parent )
    :   QWidget(parent), p(p), scUI(0), tly(p),
        hipass(0), lopass(0), jpanel(jpanel)
{
}


ShankCtl::~ShankCtl()
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

    if( scUI ) {
        delete scUI;
        scUI = 0;
    }

    emit closed( this );
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtl::geomLoad()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value(
            QString("WinLayout_ShankView_Panel%1/geometry").arg( jpanel )
            ).toByteArray() ) ) {

        // Get size from form, or do nothing.
    }
}


void ShankCtl::geomSave()
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue(
        QString("WinLayout_ShankView_Panel%1/geometry").arg( jpanel ),
        saveGeometry() );
}


void ShankCtl::showDialog()
{
    showNormal();
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

void ShankCtl::ypixChanged( int y )
{
    set.yPix = y;
    scUI->scroll->setRowPix( y );
    saveSettings();
}


void ShankCtl::whatChanged( int i )
{
    drawMtx.lock();
        set.what = i;
        SignalBlocker   b0(scUI->rngSB);
        scUI->TSB->setEnabled( !i );
        scUI->inarowSB->setEnabled( !i );
        scUI->rngSB->setValue( set.rng[i] );
        updateFilter( false );
        tly.zeroData();
        scUI->scroll->theV->colorPads( tly.sums, 1e99 );
        QMetaObject::invokeMethod(
            scUI->scroll->theV,
            "updateNow",
            Qt::QueuedConnection );
    drawMtx.unlock();
    saveSettings();
}


void ShankCtl::threshChanged( int t )
{
    drawMtx.lock();
        set.thresh = -t;
        tly.zeroData();
    drawMtx.unlock();
    saveSettings();
}


void ShankCtl::inarowChanged( int s )
{
    drawMtx.lock();
        set.inarow = s;
        tly.zeroData();
    drawMtx.unlock();
    saveSettings();
}


void ShankCtl::updtChanged( double s )
{
    drawMtx.lock();
        set.updtSecs = s;
        tly.updtChanged( s );
    drawMtx.unlock();
    saveSettings();
}


void ShankCtl::rangeChanged( int r )
{
    drawMtx.lock();
        set.rng[set.what] = r;
        tly.zeroData();
    drawMtx.unlock();
    saveSettings();
}


void ShankCtl::chanButClicked()
{
    scUI->scroll->scrollToSelected();
}


void ShankCtl::helpButClicked()
{
    showHelp( "ShankView_Help" );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void ShankCtl::baseInit( int ip )
{
    loadSettings();

    scUI = new Ui::ShankWindow;
    scUI->setupUi( this );

    scUI->scroll->theV->setRowPix( set.yPix );

    scUI->ypixSB->installEventFilter( this );
    scUI->ypixSB->setValue( set.yPix );

    scUI->whatCB->setCurrentIndex( set.what );

    scUI->TSB->installEventFilter( this );
    scUI->TSB->setValue( -set.thresh );
    scUI->TSB->setEnabled( set.what == 0 );

    scUI->inarowSB->installEventFilter( this );
    scUI->inarowSB->setValue( set.inarow );
    scUI->inarowSB->setEnabled( set.what == 0 );

    scUI->updtSB->installEventFilter( this );
    scUI->updtSB->setValue( set.updtSecs );

    scUI->rngSB->installEventFilter( this );
    scUI->rngSB->setValue( set.rng[set.what] );

    ConnectUI( scUI->scroll->theV, SIGNAL(cursorOver(int,bool)), this, SLOT(cursorOver(int,bool)) );
    ConnectUI( scUI->scroll->theV, SIGNAL(lbutClicked(int,bool)), this, SLOT(lbutClicked(int,bool)) );
    ConnectUI( scUI->ypixSB, SIGNAL(valueChanged(int)), this, SLOT(ypixChanged(int)) );
    ConnectUI( scUI->whatCB, SIGNAL(currentIndexChanged(int)), this, SLOT(whatChanged(int)) );
    ConnectUI( scUI->TSB, SIGNAL(valueChanged(int)), this, SLOT(threshChanged(int)) );
    ConnectUI( scUI->inarowSB, SIGNAL(valueChanged(int)), this, SLOT(inarowChanged(int)) );
    ConnectUI( scUI->updtSB, SIGNAL(valueChanged(double)), this, SLOT(updtChanged(double)) );
    ConnectUI( scUI->rngSB, SIGNAL(valueChanged(int)), this, SLOT(rangeChanged(int)) );
    ConnectUI( scUI->chanBut, SIGNAL(clicked()), this, SLOT(chanButClicked()) );
    ConnectUI( scUI->helpBut, SIGNAL(clicked()), this, SLOT(helpButClicked()) );

    updateFilter( true );

    setAttribute( Qt::WA_DeleteOnClose, false );

    tly.init( set.updtSecs, ip );
}


void ShankCtl::zeroFilterTransient( short *data, int ntpts, int nchans )
{
    if( nzero > 0 ) {

        // overwrite with zeros

        if( ntpts > nzero )
            ntpts = nzero;

        memset( data, 0, ntpts*nchans*sizeof(qint16) );
        nzero -= ntpts;
    }
}


void ShankCtl::dcAve(
    QVector<int>    &ave,
    short           *data,
    int             maxInt,
    int             ntpts,
    int             nchans,
    int             c0,
    int             cLim )

{
    int             nC = cLim - c0;
    QVector<float>  sum( nC, 0.0F );

    for( int it = 0; it < ntpts; ++it, data += nchans ) {

        for( int c = c0; c < cLim; ++c )
            sum[c-c0] += data[c];
    }

    ave.resize( nC );

    for( int c = 0; c < nC; ++c )
        ave[c] = qBound( -maxInt, int(sum[c]/ntpts), maxInt - 1 );
}


// Force Ctrl+A events to be treated as 'show audio dialog',
// instead of 'text-field select-all'.
//
bool ShankCtl::eventFilter( QObject *watched, QEvent *event )
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

    return QWidget::eventFilter( watched, event );
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

    if( e->isAccepted() ) {

        // reset for next showing of window
        nzero = BIQUAD_TRANS_WIDE;

        emit closed( this );
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */



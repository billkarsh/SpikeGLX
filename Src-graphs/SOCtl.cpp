
#include "ui_SpikesWindow.h"

#include "SOCtl.h"
#include "Util.h"
#include "MainApp.h"
#include "AIQ.h"
#include "DAQ.h"
#include "Run.h"
#include "SignalBlocker.h"

#include <QAction>
#include <QKeyEvent>
#include <QSettings>
#include <QThread>

#define PERIOD_SECS 0.1
#define WAVEMAX     96

/* ---------------------------------------------------------------- */
/* SOWorker ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SOWorker::setGroups( const SOGroup *grpbase )
{
    QMutexLocker    ml( &grpMtx );

    memcpy( grp, grpbase, 4 * sizeof(SOGroup) );

    Run         *run = mainApp()->getRun();
    QSet<int>   oldprb, newprb;

    for( int i = 0; i < 4; ++i ) {
        oldprb.insert( str[i].ip );
        newprb.insert( grp[i].ip );
    }

    for( int i = 0; i < 4; ++i ) {

        SOStream    &S = str[i];
        SOGroup     &G = grp[i];

        if( S.aiQ && !newprb.contains( S.ip ) )
            S.aiQ->qf_spikeClient( false );

        S.stream = QString("imec%1").arg( G.ip );
        S.aiQ    = run->getQ( -jsIM, G.ip );
        if( S.ip != G.ip || S.ch != G.ch || S.T != G.T )
            S.fromCt = 0;
        S.ip = G.ip;
        S.ch = G.ch;
        S.T  = G.T;

        if( S.aiQ && !oldprb.contains( S.ip ) )
            S.aiQ->qf_spikeClient( true );
    }
}


void SOWorker::run()
{
    const int   loopPeriod_us = 1e6 * PERIOD_SECS;

    while( !isStopped() ) {

        double  loopT = getTime();

        if( !isPaused() ) {

            QMutexLocker    ml( &grpMtx );

            for( int i = 0; i < 4; ++i )
                scan( grp[i], str[i] );
        }

        // Fetch no more often than every loopPeriod_us

        loopT = 1e6*(getTime() - loopT);    // microsec

        if( loopT < loopPeriod_us )
            QThread::usleep( loopPeriod_us - loopT );
        else
            QThread::usleep( 1000 * 10 );
    }

    for( int i = 0; i < 4; ++i ) {
        if( str[i].aiQ )
            str[i].aiQ->qf_spikeClient( false );
    }

    emit finished();
}


void SOWorker::scan( SOGroup &G, SOStream &S )
{
#define LHS     20
#define INAROW  3

    float   wave[NPNT];
    quint64 outCt,
            endCt   = S.aiQ->endCount() - NPNT;
    qint16  *src;
    int     T       = G.T / G.i2uV,
            nadd    = 0,
            vmin,
            imin;

    while( S.fromCt < endCt ) {

        if( S.aiQ->findFallingEdge( outCt, S.fromCt, G.ch, T, INAROW ) ) {

            S.fromCt = outCt;

            vec_i16 data;
            data.reserve( WAVEMAX * G.nC );

            if( 1 != S.aiQ->getNSampsFromCt( data, outCt - 32, WAVEMAX ) )
                break;

            if( data.size() < WAVEMAX * G.nC )
                break;

            src  = &data[G.ch];
            vmin = src[32*G.nC];
            imin = 32;

            for( int it = 33; it < WAVEMAX; ++it ) {
                if( src[it*G.nC] < vmin ) {
                    vmin = src[it*G.nC];
                    imin = it;
                }
            }

            if( imin - LHS + NPNT <= WAVEMAX ) {

                for( int i = 0, it = imin - LHS; i < NPNT; ++i, ++it )
                    wave[i] = src[it*G.nC] * G.i2uV;

                G.grf->addWave( wave, S.fromCt );
                ++nadd;
            }

            S.fromCt += imin - 32;
        }
        else {
            S.fromCt = outCt;
            break;
        }
    }

    if( !nadd )
        G.grf->addWave( wave, -qint64(endCt + NPNT) );
}

/* ---------------------------------------------------------------- */
/* SOFetcher ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

SOFetcher::SOFetcher()
{
    thread  = new QThread;
    worker  = new SOWorker;

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


SOFetcher::~SOFetcher()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() ) {

        worker->stop();
        thread->wait();
    }

    delete thread;
}


void SOFetcher::setGroups( SOGroup *grpbase, const DAQ::Params &p )
{
    for( int i = 0; i < 4; ++i ) {
        SOGroup                 &G = grpbase[i];
        const CimCfg::PrbEach   &E = p.im.prbj[G.ip];
        G.i2uV  = 1E6 * E.intToV( 1, G.ch );
        G.nC    = E.imCumTypCnt[CimCfg::imSumAll];
    }

    worker->setGroups( grpbase );
}

/* ---------------------------------------------------------------- */
/* SOCtl ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

SOCtl::SOCtl( const DAQ::Params &p, QWidget *parent )
    :   QDialog(parent), p(p), soUI(0), fetch(0)
{
}


void SOCtl::predelete()
{
    saveScreenState();
    emit closed( this );

    if( soUI ) {
        delete soUI;
        soUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SOCtl::init()
{
    soUI = new Ui::SpikesWindow;
    soUI->setupUi( this );

    grp[0].grf = soUI->graph0;
    grp[1].grf = soUI->graph1;
    grp[2].grf = soUI->graph2;
    grp[3].grf = soUI->graph3;

    loadSettings();

    soUI->pSB0->installEventFilter( this );
    soUI->pSB1->installEventFilter( this );
    soUI->pSB2->installEventFilter( this );
    soUI->pSB3->installEventFilter( this );

    soUI->cSB0->installEventFilter( this );
    soUI->cSB1->installEventFilter( this );
    soUI->cSB2->installEventFilter( this );
    soUI->cSB3->installEventFilter( this );

    soUI->tSB0->installEventFilter( this );
    soUI->tSB1->installEventFilter( this );
    soUI->tSB2->installEventFilter( this );
    soUI->tSB3->installEventFilter( this );

    soUI->ySB0->installEventFilter( this );
    soUI->ySB1->installEventFilter( this );
    soUI->ySB2->installEventFilter( this );
    soUI->ySB3->installEventFilter( this );

    int np = p.stream_nIM() - 1;
    soUI->pSB0->setMaximum( np );
    soUI->pSB1->setMaximum( np );
    soUI->pSB2->setMaximum( np );
    soUI->pSB3->setMaximum( np );

    SOGroup *G;
    int     maxch;

    G = &grp[0];
    if( G->ip > np )
        G->ip = 0;
    G->grf->sRate = p.im.prbj[G->ip].srate;
    soUI->pSB0->setValue( G->ip );
    maxch = p.im.prbj[G->ip].roTbl->nAP() - 1;
    G->ch = (G->ch <= maxch ? G->ch : 0);
    soUI->cSB0->setMaximum( maxch );
    soUI->cSB0->setValue( G->ch );
    soUI->tSB0->setValue( -G->T );
    soUI->ySB0->setValue( G->grf->yscl );

    G = &grp[1];
    if( G->ip > np )
        G->ip = 0;
    G->grf->sRate = p.im.prbj[G->ip].srate;
    soUI->pSB1->setValue( G->ip );
    maxch = p.im.prbj[G->ip].roTbl->nAP() - 1;
    G->ch = (G->ch <= maxch ? G->ch : 0);
    soUI->cSB1->setMaximum( maxch );
    soUI->cSB1->setValue( G->ch );
    soUI->tSB1->setValue( -G->T );
    soUI->ySB1->setValue( G->grf->yscl );

    G = &grp[2];
    if( G->ip > np )
        G->ip = 0;
    G->grf->sRate = p.im.prbj[G->ip].srate;
    soUI->pSB2->setValue( G->ip );
    maxch = p.im.prbj[G->ip].roTbl->nAP() - 1;
    G->ch = (G->ch <= maxch ? G->ch : 0);
    soUI->cSB2->setMaximum( maxch );
    soUI->cSB2->setValue( G->ch );
    soUI->tSB2->setValue( -G->T );
    soUI->ySB2->setValue( G->grf->yscl );

    G = &grp[3];
    if( G->ip > np )
        G->ip = 0;
    G->grf->sRate = p.im.prbj[G->ip].srate;
    soUI->pSB3->setValue( G->ip );
    maxch = p.im.prbj[G->ip].roTbl->nAP() - 1;
    G->ch = (G->ch <= maxch ? G->ch : 0);
    soUI->cSB3->setMaximum( maxch );
    soUI->cSB3->setValue( G->ch );
    soUI->tSB3->setValue( -G->T );
    soUI->ySB3->setValue( G->grf->yscl );

    saveSettings();

    ConnectUI( soUI->pSB0, SIGNAL(valueChanged(int)), this, SLOT(pSB0Changed(int)) );
    ConnectUI( soUI->pSB1, SIGNAL(valueChanged(int)), this, SLOT(pSB1Changed(int)) );
    ConnectUI( soUI->pSB2, SIGNAL(valueChanged(int)), this, SLOT(pSB2Changed(int)) );
    ConnectUI( soUI->pSB3, SIGNAL(valueChanged(int)), this, SLOT(pSB3Changed(int)) );

    ConnectUI( soUI->cSB0, SIGNAL(valueChanged(int)), this, SLOT(cSB0Changed(int)) );
    ConnectUI( soUI->cSB1, SIGNAL(valueChanged(int)), this, SLOT(cSB1Changed(int)) );
    ConnectUI( soUI->cSB2, SIGNAL(valueChanged(int)), this, SLOT(cSB2Changed(int)) );
    ConnectUI( soUI->cSB3, SIGNAL(valueChanged(int)), this, SLOT(cSB3Changed(int)) );

    ConnectUI( soUI->tSB0, SIGNAL(valueChanged(int)), this, SLOT(tSB0Changed(int)) );
    ConnectUI( soUI->tSB1, SIGNAL(valueChanged(int)), this, SLOT(tSB1Changed(int)) );
    ConnectUI( soUI->tSB2, SIGNAL(valueChanged(int)), this, SLOT(tSB2Changed(int)) );
    ConnectUI( soUI->tSB3, SIGNAL(valueChanged(int)), this, SLOT(tSB3Changed(int)) );

    ConnectUI( soUI->ySB0, SIGNAL(valueChanged(int)), this, SLOT(ySB0Changed(int)) );
    ConnectUI( soUI->ySB1, SIGNAL(valueChanged(int)), this, SLOT(ySB1Changed(int)) );
    ConnectUI( soUI->ySB2, SIGNAL(valueChanged(int)), this, SLOT(ySB2Changed(int)) );
    ConnectUI( soUI->ySB3, SIGNAL(valueChanged(int)), this, SLOT(ySB3Changed(int)) );

    ConnectUI( soUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );

// Window

#ifdef Q_OS_WIN
    setWindowFlags( Qt::Tool );
#endif
    setAttribute( Qt::WA_DeleteOnClose, false );

    restoreScreenState();
}


void SOCtl::setChan( int gp, int ip, int ch )
{
    if( !fetch )
        fetch = new SOFetcher();

    if( gp-- > 0 )
        pSBChanged( gp, ip, ch );
    else {
        fetch->pause( true );
        fetch->waitPaused();
        fetch->setGroups( grp, p );
    }

    fetch->pause( false );
    showDialog();
}


void SOCtl::stopFetching()
{
    if( fetch ) {
        delete fetch;
        fetch = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SOCtl::pSB0Changed( int v )
{
    pSBChanged( 0, v, -1 );
}


void SOCtl::pSB1Changed( int v )
{
    pSBChanged( 1, v, -1 );
}


void SOCtl::pSB2Changed( int v )
{
    pSBChanged( 2, v, -1 );
}


void SOCtl::pSB3Changed( int v )
{
    pSBChanged( 3, v, -1 );
}


void SOCtl::cSB0Changed( int v )
{
    cSBChanged( 0, v );
}


void SOCtl::cSB1Changed( int v )
{
    cSBChanged( 1, v );
}


void SOCtl::cSB2Changed( int v )
{
    cSBChanged( 2, v );
}


void SOCtl::cSB3Changed( int v )
{
    cSBChanged( 3, v );
}


void SOCtl::tSB0Changed( int v )
{
    tSBChanged( 0, v );
}


void SOCtl::tSB1Changed( int v )
{
    tSBChanged( 1, v );
}


void SOCtl::tSB2Changed( int v )
{
    tSBChanged( 2, v );
}


void SOCtl::tSB3Changed( int v )
{
    tSBChanged( 3, v );
}


void SOCtl::ySB0Changed( int v )
{
    ySBChanged( 0, v );
}


void SOCtl::ySB1Changed( int v )
{
    ySBChanged( 1, v );
}


void SOCtl::ySB2Changed( int v )
{
    ySBChanged( 2, v );
}


void SOCtl::ySB3Changed( int v )
{
    ySBChanged( 3, v );
}


void SOCtl::helpBut()
{
    showHelp( "SpikeView_Help" );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Force Ctrl+A events to be treated as 'show audio dialog',
// instead of 'text-field select-all'.
//
bool SOCtl::eventFilter( QObject *watched, QEvent *event )
{
    if( event->type() == QEvent::KeyPress ) {

        QKeyEvent   *e = static_cast<QKeyEvent*>(event);

        if( e->modifiers() == Qt::ControlModifier ) {

            if( e->key() == Qt::Key_Z ) {
                mainApp()->act.raiseConsAct->trigger();
                e->ignore();
                return true;
            }
            else if( e->key() == Qt::Key_A ) {
                mainApp()->act.aoDlgAct->trigger();
                e->ignore();
                return true;
            }
        }
    }

    return QDialog::eventFilter( watched, event );
}


void SOCtl::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
    }
    else
        QDialog::keyPressEvent( e );
}


void SOCtl::closeEvent( QCloseEvent *e )
{
    QDialog::closeEvent( e );

    if( e->isAccepted() ) {

        stopFetching();
        emit closed( this );
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// If ch < 0 this is called by p-spinner, else by right-click menu.
//
// Changing stream always entails setting new max channel, and we
// will force current channel to zero if exceeds max.
//
void SOCtl::pSBChanged( int i, int v, int ch )
{
    QSpinBox    *pSB, *cSB;

    if( i == 0 ) {
        pSB = soUI->pSB0;
        cSB = soUI->cSB0;
    }
    else if( i == 1 ) {
        pSB = soUI->pSB1;
        cSB = soUI->cSB1;
    }
    else if( i == 2 ) {
        pSB = soUI->pSB2;
        cSB = soUI->cSB2;
    }
    else {
        pSB = soUI->pSB3;
        cSB = soUI->cSB3;
    }

    SignalBlocker   b0(pSB), b1(cSB);

    SOGroup &G      = grp[i];
    int     maxch   = p.im.prbj[v].roTbl->nAP() - 1;

    G.ip = v;
    if( ch >= 0 )
        pSB->setValue( v );
    else
        ch = cSB->value();
    G.ch = ch = (ch <= maxch ? ch : 0);
    cSB->setMaximum( maxch );
    cSB->setValue( ch );

    bool wasPaused = fetch->pause( true );
    fetch->waitPaused();

        G.grf->dataMtx.lock();
            G.grf->sRate = p.im.prbj[v].srate;
        G.grf->dataMtx.unlock();
        G.grf->clear();
        fetch->setGroups( grp, p );

    if( !wasPaused ) fetch->pause( false );

    saveSettings();
}


void SOCtl::cSBChanged( int i, int v )
{
    SOGroup &G = grp[i];
    G.ch = v;

    bool wasPaused = fetch->pause( true );
    fetch->waitPaused();

        G.grf->clear();
        fetch->setGroups( grp, p );

    if( !wasPaused ) fetch->pause( false );
    saveSettings();
}


void SOCtl::tSBChanged( int i, int v )
{
    SOGroup &G = grp[i];
    G.T = -v;

    bool wasPaused = fetch->pause( true );
    fetch->waitPaused();

        G.grf->clear();
        fetch->setGroups( grp, p );

    if( !wasPaused ) fetch->pause( false );
    saveSettings();
}


void SOCtl::ySBChanged( int i, int v )
{
    SOGroup &G = grp[i];
    G.grf->dataMtx.lock();
        G.grf->yscl = v;
    G.grf->dataMtx.unlock();
    G.grf->update();
    saveSettings();
}


void SOCtl::showDialog()
{
    showNormal();
    mainApp()->modelessOpened( this );
}


void SOCtl::loadSettings()
{
    STDSETTINGS( settings, "soctl" );

    for( int i = 0; i < 4; ++i ) {
        SOGroup &G = grp[i];
        G.grf->label = i + 1;
        settings.beginGroup( QString("Group%1").arg( i ) );
            G.grf->yscl = settings.value( "yscl", 500 ).toInt();
            G.ip        = settings.value( "probe", 0 ).toInt();
            G.ch        = settings.value( "chan", 0 ).toInt();
            G.T         = settings.value( "thresh", -75 ).toInt();
        settings.endGroup();
    }
}


void SOCtl::saveSettings() const
{
    STDSETTINGS( settings, "soctl" );

    for( int i = 0; i < 4; ++i ) {
        const SOGroup   &G = grp[i];
        settings.beginGroup( QString("Group%1").arg( i ) );
            settings.setValue( "yscl", G.grf->yscl );
            settings.setValue( "probe", G.ip );
            settings.setValue( "chan", G.ch );
            settings.setValue( "thresh", G.T );
        settings.endGroup();
    }
}


// Note:
// restoreScreenState() must be called after initializing
// a window's controls with setupUI().
//
void SOCtl::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( "WinLayout_Spikes/geometry" ).toByteArray() ) ) {

        // Get size from form, or do nothing.
    }
}


void SOCtl::saveScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( "WinLayout_Spikes/geometry", geomSave( this, 26, 5 ) );
}



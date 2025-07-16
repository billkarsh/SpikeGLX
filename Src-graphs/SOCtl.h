#ifndef SOCTL_H
#define SOCTL_H

#include "HelpButDialog.h"

#include <QMutex>

namespace Ui {
class SpikesWindow;
}

namespace DAQ {
struct Params;
}

class SOGraph;
class AIQ;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// ---------------------
// Spike Overlay feature
// ---------------------

struct SOGroup {
    SOGraph *grf;
    float   i2uV;
    int     nC,
            ip,
            ch,
            T;
};

struct SOStream {
    QString     stream;
    const AIQ   *aiQ;
    quint64     fromCt;
    int         ip, ch, T;
    SOStream() : aiQ(0), ip(-1) {}
};

class SOWorker : public QObject
{
    Q_OBJECT

private:
    SOGroup         grp[4];
    SOStream        str[4];
    mutable QMutex  grpMtx,
                    runMtx;
    volatile bool   paused,
                    pleaseStop;

public:
    // Initially paused to wait for setGroups() call
    SOWorker() : QObject(0), paused(true), pleaseStop(false)    {}

    void setGroups( const SOGroup *grpbase );

    bool pause( bool pause )
        {
            QMutexLocker ml( &runMtx );
            bool was = paused;
            paused = pause;
            return was;
        }
    bool isPaused() const
        {QMutexLocker ml( &runMtx ); return paused;}
    void waitPaused()
        {
            QMutexLocker ml( &runMtx );
            if( paused )    // wait for access to grp
                QMutexLocker ml2( &grpMtx );
        }

    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void finished();

public slots:
    void run();

private:
    void scan( SOGroup &G, SOStream &S );
};

class SOFetcher
{
private:
    QThread     *thread;
    SOWorker    *worker;

public:
    SOFetcher();
    virtual ~SOFetcher();

    void setGroups( SOGroup *grpbase, const DAQ::Params &p );

    bool pause( bool pause )    {return worker->pause( pause );}
    void waitPaused()           {worker->waitPaused();}
};

class SOCtl : public HelpButDialog
{
    Q_OBJECT

private:
    const DAQ::Params   &p;
    Ui::SpikesWindow    *soUI;
    SOFetcher           *fetch;
    SOGroup             grp[4];

public:
    SOCtl( const DAQ::Params &p, QWidget *parent );
    virtual ~SOCtl();

    void init();
    void setChan( int gp, int ip, int ch );
    void stopFetching();

signals:
    void closed( QWidget *w );

private slots:
    void pSB0Changed( int v );
    void pSB1Changed( int v );
    void pSB2Changed( int v );
    void pSB3Changed( int v );
    void cSB0Changed( int v );
    void cSB1Changed( int v );
    void cSB2Changed( int v );
    void cSB3Changed( int v );
    void tSB0Changed( int v );
    void tSB1Changed( int v );
    void tSB2Changed( int v );
    void tSB3Changed( int v );
    void ySB0Changed( int v );
    void ySB1Changed( int v );
    void ySB2Changed( int v );
    void ySB3Changed( int v );

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void pSBChanged( int i, int v, int ch );
    void cSBChanged( int i, int v );
    void tSBChanged( int i, int v );
    void ySBChanged( int i, int v );

    void showDialog();

    void loadSettings();
    void saveSettings() const;

    void restoreScreenState();
    void saveScreenState() const;
};

#endif  // SOCTL_H



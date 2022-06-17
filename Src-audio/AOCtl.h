#ifndef AOCTL_H
#define AOCTL_H

#include "AODevBase.h"
#include "Biquad.h"

#include <QWidget>
#include <QMutex>

namespace Ui {
class AOWindow;
}

namespace DAQ {
struct Params;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class AOCtl : public QWidget
{
    Q_OBJECT

    friend class AODevRtAudio;

private:
    struct EachStream {
        QString     loCutStr,
                    hiCutStr;
        double      volume;
        int         left,
                    right;
    };

    struct User {
        const DAQ::Params       &p;
        std::vector<EachStream> each;
        QString                 stream;
        bool                    autoStart;

        User( const DAQ::Params &p ) : p(p) {loadSettings( 0 );}
        void loadSettings( bool remote = false );
        void saveSettings( bool remote = false ) const;
    };

    struct Derived {
        Biquad  hipass,
                lopass;
        double  srate,
                loCut,
                hiCut,
                lVol,
                rVol;
        int     streamjs,   // {0,1,2}
                streamip,   // if {obx,imec}
                lChan,
                rChan,
                nNeural,
                maxInt,
                maxLatency;

        void usr2drv( AOCtl *aoC );

        qint16 vol( qint16 val, double vol ) const;
    };

private:
    const DAQ::Params   &p;
    Ui::AOWindow        *aoUI;
    QString             ctorErr;
    User                usr;
    Derived             drv;
    AODevBase           *aoDev;
    mutable QMutex      aoMtx;      // guards {usr, aoDev}
    int                 nDevChans;
    bool                dlgShown;

public:
    AOCtl( const DAQ::Params &p, QWidget *parent = 0 );
    virtual ~AOCtl();

    bool uniqueAIs( std::vector<int> &vAI, const QString &stream ) const;

    bool showDialog( QWidget *parent = 0 );

    void graphSetsChannel( int chan, bool isLeft, const QString &stream );

    // Development tests
    void test1()                {aoDev->test1();}
    void test2()                {aoDev->test2();}
    void test3()                {aoDev->test3();}

    // Device api
    bool doAutoStart()          {return aoDev->doAutoStart();}
    bool readyForScans() const  {return aoDev->readyForScans();}
    bool devStart(
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ )
                                {return aoDev->devStart( imQ, obQ, niQ );}
    void devStop()              {aoDev->devStop();}
    void restart();

signals:
    void closed( QWidget *w );

public slots:
    void reset( bool remote = false );
    QString cmdSrvSetsAOParamStr(
        const QString   &groupStr,
        const QString   &paramStr );

private slots:
    void streamCBChanged( bool live = true );
    void leftSBChanged( int val );
    void rightSBChanged( int val );
    void loCBChanged( const QString &str );
    void hiCBChanged( const QString &str );
    void volSBChanged( double val );
    void help();
    void stop();
    void apply();

protected:
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void ctorCheckAudioSupport();
    void str2RemoteIni( const QString &groupStr, const QString prmStr );
    void liveChange();
    bool valid( QString &err, bool remote = false );
    void restoreScreenState();
    void saveScreenState() const;
};

#endif  // AOCTL_H



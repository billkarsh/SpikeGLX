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

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class AOCtl : public QWidget
{
    Q_OBJECT

    friend class AODevRtAudio;

private:
    struct User {
        QString     stream,
                    loCutStr,
                    hiCutStr;
        double      volume;
        int         imLeft,
                    imRight,
                    niLeft,
                    niRight;
        bool        autoStart;

        User() {loadSettings( false );}
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
        int     lChan,
                rChan,
                nNeural,
                maxBits,
                maxLatency;
        bool    isImec;

        void usr2drv( AOCtl *aoC );

        qint16 vol( qint16 val, double vol ) const;
    };

private:
    const DAQ::Params   &p;
    Ui::AOWindow        *aoUI;
    QDialog             *helpDlg;
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

    bool uniqueAIs( QVector<int> &vAI ) const;

    bool showDialog( QWidget *parent = 0 );

    void graphSetsChannel( int chan, bool isLeft, bool isImec );

    // Development tests
    void test1()                {aoDev->test1();}
    void test2()                {aoDev->test2();}
    void test3()                {aoDev->test3();}

    // Device api
    bool doAutoStart()          {return aoDev->doAutoStart();}
    bool readyForScans() const  {return aoDev->readyForScans();}
    bool devStart( const AIQ *imQ, const AIQ *niQ )
                                {return aoDev->devStart( imQ, niQ );}
    void devStop()              {aoDev->devStop();}
    void restart();

signals:
    void closed( QWidget *w );

public slots:
    QString cmdSrvSetsAOParamStr( const QString &paramString );

private slots:
    void streamCBChanged( bool live = true );
    void loCBChanged( const QString &str );
    void liveChange();
    void showHelp();
    void reset( bool remote = false );
    void stop();
    void apply();

protected:
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void ctorCheckAudioSupport();
    void str2RemoteIni( const QString str );
    bool valid( QString &err );
    void saveScreenState();
    void restoreScreenState();
};

#endif  // AOCTL_H



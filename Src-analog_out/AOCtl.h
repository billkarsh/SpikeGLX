#ifndef AOCTL_H
#define AOCTL_H

#include "DAQ.h"
#include "CniAO.h"

#include <QWidget>
#include <QString>
#include <QVector>
#include <QMutex>

namespace Ui {
class AOWindow;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class AOCtl : public QWidget
{
    Q_OBJECT

    friend class CniAODmx;

private:
    struct User {
        VRange      range;
        QString     devStr,
                    aoClockStr,
                    uiChanStr,
                    loCutStr,
                    hiCutStr;
        double      volume;
        bool        autoStart;

        User() {loadSettings( false );}
        void prettyChanStr();
        void loadSettings( bool remote = false );
        void saveSettings( bool remote = false ) const;
        bool isClockInternal() const    {return aoClockStr == "Internal";}
    };

private:
    DAQ::Params         &p;
    QVector<QString>    devNames;
    Ui::AOWindow        *aoUI;
    QDialog             *noteDlg;
    User                usr;
    QMap<uint,uint>     o2iMap;
    CniAO               *niAO;
    mutable QMutex      aoMtx;  // guards {usr, taskHandle}

public:
    AOCtl( DAQ::Params &p, QWidget *parent = 0 );
    virtual ~AOCtl();

    double bufSecs();

    bool showDialog( QWidget *parent = 0 );

    bool doAutoStart()                  {return niAO->doAutoStart();}
    bool readyForScans()                {return niAO->readyForScans();}
    bool devStart()                     {return niAO->devStart();}
    void devStop()                      {niAO->devStop();}
    void putScans( vec_i16 &aiData )    {niAO->putScans( aiData );}
    void restart();

signals:
    void closed();

public slots:
    QString cmdSrvSetsAOParamStr( const QString &paramString );

private slots:
    void deviceCBChanged();
    void loCBChanged( const QString &str );
    void liveChange();
    void showNote();
    void reset( bool remote = false );
    void stop();
    void apply();

protected:
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void str2RemoteIni( const QString str );
    bool str2Map( const QString &s );
    bool valid( QString &err );
};

#endif  // AOCTL_H



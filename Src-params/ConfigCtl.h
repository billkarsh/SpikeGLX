#ifndef CONFIGCTL_H
#define CONFIGCTL_H

#include "Config_devtab.h"
#include "Config_imtab.h"
#include "Config_obxtab.h"
#include "Config_nitab.h"
#include "Config_synctab.h"
#include "Config_gatetab.h"
#include "Config_trigtab.h"
#include "Config_snstab.h"
#include "DAQ.h"

#include <QDialog>

namespace Ui {
class ConfigureDialog;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ConfigCtl : public QObject
{
    Q_OBJECT

private:
    QDialog                 *cfgDlg;
    Ui::ConfigureDialog     *cfgUI;
    Config_devtab           *devTab;
    Config_imtab            *imTab;
    Config_obxtab           *obxTab;
    Config_nitab            *niTab;
    Config_synctab          *syncTab;
    Config_gatetab          *gateTab;
    Config_trigtab          *trigTab;
    Config_snstab           *snsTab;

public:
    CimCfg::ImProbeTable    prbTab; // THE TABLE, filled in by detect()
    DAQ::Params             acceptedParams;
    bool                    usingIM,    // selected and detected
                            usingOB,    // selected and detected
                            usingNI,    // selected and detected
                            validated;

public:
    ConfigCtl( QObject *parent = 0 );
    virtual ~ConfigCtl();

    bool showDialog();
    bool isConfigDlg( QObject *parent );
    QDialog *dialog()   {return (QDialog*)cfgDlg;}

    void setParams( const DAQ::Params &p, bool write );

    bool externSetsRunName(
        QString         &err,
        const QString   &name,
        QWidget         *parent );

    bool externSetsNotes( QString &err, const QString &s );

    void graphSetsImroFile( const QString &file, int ip );
    void graphSetsStdbyStr( const QString &sdtbyStr, int ip );
    void graphSetsImChanMap( const QString &cmFile, int ip );
    void graphSetsObChanMap( const QString &cmFile, int ip );
    void graphSetsNiChanMap( const QString &cmFile );
    void graphSetsImSaveStr( const QString &saveStr, int ip, bool lfPairChk );
    void graphSetsObSaveStr( const QString &saveStr, int ip );
    void graphSetsNiSaveStr( const QString &saveStr );
    void graphSetsImSaveBit( int chan, bool setOn, int ip );
    void graphSetsObSaveBit( int chan, bool setOn, int ip );
    void graphSetsNiSaveBit( int chan, bool setOn );

    bool chanMapGetsShankOrder(
        QString     &s,
        QString     type,
        int         ip,
        bool        rev,
        QWidget     *parent ) const;

    void setSelectiveAccess( bool availIM, bool availNI );

    void streamCB_fillConfig( QComboBox *CB ) const;
    bool niSingletonReserve()
        {return niTab->singletonReserve();}
    void niSingletonRelease()
        {niTab->singletonRelease();}
    const QString &niCurDevName() const
        {return niTab->curDevName();}
    void syncNiDevChanged()
        {syncTab->syncSourceCBChanged();}

    bool validIMROTbl( QString &err, CimCfg::PrbEach &E, int ip, bool srCheck ) const;
    bool validImMaps( QString &err, CimCfg::PrbEach &E, int ip ) const;
    bool validDataDir( QString &err ) const;
    bool diskParamsToQ( QString &err, DAQ::Params &q ) const;

public slots:
    QString cmdSrvGetsSaveChansIm( int ip ) const;
    QString cmdSrvGetsSaveChansOb( int ip ) const;
    QString cmdSrvGetsSaveChansNi() const;
    QString cmdSrvGetsParamStr( int type, int ip ) const;
    QString cmdSrvSetsParamStr( const QString &paramString, int type, int ip );

    void initUsing_im_ob();
    void initUsing_ni();
    void initUsing_all();

    void updateCalWarning();

private slots:
    void tabChanged( int tab );
    void helpBut();
    void reset();
    void verify();
    void okBut();

private:
    void setNoDialogAccess( bool clearNi = true );
    void paramsFromDialog(
        DAQ::Params     &q,
        QVector<uint>   &vcMN1,
        QVector<uint>   &vcMA1,
        QVector<uint>   &vcXA1,
        QVector<uint>   &vcXD1,
        QVector<uint>   &vcMN2,
        QVector<uint>   &vcMA2,
        QVector<uint>   &vcXA2,
        QVector<uint>   &vcXD2,
        QString         &uiStr1Err,
        QString         &uiStr2Err ) const;
    bool validDevTab( QString &err, DAQ::Params &q ) const;
    bool validImLEDs( QString &err, DAQ::Params &q ) const;
    bool validImStdbyBits( QString &err, CimCfg::PrbEach &E, int ip ) const;
    bool validObChannels( QString &err, DAQ::Params &q, int istr ) const;
    bool validNiDevices( QString &err, DAQ::Params &q ) const;
    bool validNiClock( QString &err, DAQ::Params &q ) const;
    bool validNiChannels(
        QString         &err,
        DAQ::Params     &q,
        QVector<uint>   &vcMN1,
        QVector<uint>   &vcMA1,
        QVector<uint>   &vcXA1,
        QVector<uint>   &vcXD1,
        QVector<uint>   &vcMN2,
        QVector<uint>   &vcMA2,
        QVector<uint>   &vcXA2,
        QVector<uint>   &vcXD2,
        QString         &uiStr1Err,
        QString         &uiStr2Err ) const;
    void validImShankMap( CimCfg::PrbEach &E ) const;
    bool validNiShankMap( QString &err, DAQ::Params &q ) const;
    bool validImChanMap( QString &err, CimCfg::PrbEach &E, int ip ) const;
    bool validObChanMap( QString &err, DAQ::Params &q, int istr ) const;
    bool validNiChanMap( QString &err, DAQ::Params &q ) const;
    bool validImSaveBits( QString &err, DAQ::Params &q, int ip ) const;
    bool validObSaveBits( QString &err, DAQ::Params &q, int istr ) const;
    bool validNiSaveBits( QString &err, DAQ::Params &q ) const;
    bool validSyncTab( QString &err, DAQ::Params &q ) const;
    bool validTriggerStream( QString &err, DAQ::Params &q ) const;
    bool validImTriggering( QString &err, DAQ::Params &q ) const;
    bool validObTriggering( QString &err, DAQ::Params &q ) const;
    bool validNiTriggering( QString &err, DAQ::Params &q ) const;
    bool validTrgPeriEvent( QString &err, DAQ::Params &q ) const;
    bool validTrgLowTime( QString &err, DAQ::Params &q ) const;
    bool validDiskAvail( QString &err, DAQ::Params &q ) const;

    bool validRunName(
        QString         &err,
        DAQ::Params     &q,
        QString         runName,
        QWidget         *parent );

    bool shankParamsToQ( QString &err, DAQ::Params &q, int ip ) const;
    bool valid( QString &err, QWidget *parent = 0, int iprb = -1 );
    void running_setProbe( DAQ::Params &q, int ip );
};

#endif  // CONFIGCTL_H



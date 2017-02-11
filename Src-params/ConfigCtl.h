#ifndef CONFIGCTL_H
#define CONFIGCTL_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class ConfigureDialog;
class DevicesTab;
class IMCfgTab;
class NICfgTab;
class GateTab;
class GateImmedPanel;
class GateTCPPanel;
class TrigTab;
class TrigImmedPanel;
class TrigTimedPanel;
class TrigTTLPanel;
class TrigSpikePanel;
class TrigTCPPanel;
class MapTab;
class SeeNSaveTab;
}

class HelpButDialog;

class QSharedMemory;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ConfigCtl : public QObject
{
    Q_OBJECT

private:
    Ui::ConfigureDialog *cfgUI;
    Ui::DevicesTab      *devTabUI;
    Ui::IMCfgTab        *imTabUI;
    Ui::NICfgTab        *niTabUI;
    Ui::GateTab         *gateTabUI;
    Ui::GateImmedPanel  *gateImmPanelUI;
    Ui::GateTCPPanel    *gateTCPPanelUI;
    Ui::TrigTab         *trigTabUI;
    Ui::TrigImmedPanel  *trigImmPanelUI;
    Ui::TrigTimedPanel  *trigTimPanelUI;
    Ui::TrigTTLPanel    *trigTTLPanelUI;
    Ui::TrigSpikePanel  *trigSpkPanelUI;
    Ui::TrigTCPPanel    *trigTCPPanelUI;
    Ui::MapTab          *mapTabUI;
    Ui::SeeNSaveTab     *snsTabUI;
    HelpButDialog       *cfgDlg;
    QVector<QString>    devNames;
    QSharedMemory       *singleton;
    bool                imecOK,
                        nidqOK;

public:
    CimCfg::IMVers  imVers; // filled in by detect();
    DAQ::Params     acceptedParams;
    bool            validated;

public:
    ConfigCtl( QObject *parent = 0 );
    virtual ~ConfigCtl();

    bool showDialog();

    void setRunName( const QString &name );
    void graphSetsImroFile( const QString &file );
    void graphSetsStdbyStr( const QString &sdtbyStr );
    void graphSetsImSaveStr( const QString &saveStr );
    void graphSetsNiSaveStr( const QString &saveStr );
    void graphSetsImSaveBit( int chan, bool setOn );
    void graphSetsNiSaveBit( int chan, bool setOn );

    bool validRunName(
        QString         &err,
        const QString   &runName,
        QWidget         *parent,
        bool            isGUI ) const;

    bool chanMapGetsShankOrder(
        QString         &s,
        const QString   type,
        QWidget         *parent ) const;

public slots:
    QString cmdSrvGetsSaveChansIm() const;
    QString cmdSrvGetsSaveChansNi() const;
    QString cmdSrvGetsParamStr() const;
    QString cmdSrvSetsParamStr( const QString &paramString );

private slots:
    void skipDetect();
    void detect();
    void forceButClicked();
    void stripButClicked();
    void imroButClicked();
    void device1CBChanged();
    void device2CBChanged();
    void muxingChanged();
    void clk1CBChanged();
    void freqButClicked();
    void syncEnableClicked( bool checked );
    void gateModeChanged();
    void manOvShowButClicked( bool checked );
    void trigModeChanged();
    void imShkMapButClicked();
    void niShkMapButClicked();
    void imChnMapButClicked();
    void niChnMapButClicked();
    void runDirButClicked();
    void diskButClicked();
    void trigTimHInfClicked();
    void trigTimNInfClicked( bool checked );
    void trigTTLModeChanged( int _mode );
    void trigTTLNInfClicked( bool checked );
    void trigSpkNInfClicked( bool checked );
    void reset( DAQ::Params *pRemote = 0 );
    void verify();
    void okBut();

private:
    bool singletonReserve();
    void singletonRelease();
    void setNoDialogAccess();
    void setSelectiveAccess();
    bool somethingChecked();
    void imWrite( const QString &s );
    void imWriteCurrent();
    void imDetect();
    void niWrite( const QString &s );
    QColor niSetColor( const QColor &c );
    void niDetect();
    bool doingImec() const;
    bool doingNidq() const;
    void diskWrite( const QString &s );
    void setupDevTab( DAQ::Params &p );
    void setupImTab( DAQ::Params &p );
    void setupNiTab( DAQ::Params &p );
    void setupGateTab( DAQ::Params &p );
    void setupTrigTab( DAQ::Params &p );
    void setupMapTab( DAQ::Params &p );
    void setupSnsTab( DAQ::Params &p );
    void setupNiVRangeCB();
    QString uiMNStr2FromDlg() const;
    QString uiMAStr2FromDlg() const;
    QString uiXAStr2FromDlg() const;
    QString uiXDStr2FromDlg() const;
    bool isMuxingFromDlg() const;
    bool niChannelsFromDialog( CniCfg &ni ) const;
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
    bool validImROTbl( QString &err, DAQ::Params &q ) const;
    bool validImStdbyBits( QString &err, DAQ::Params &q ) const;
    bool validNiDevices( QString &err, DAQ::Params &q ) const;
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
    bool validImTriggering( QString &err, DAQ::Params &q ) const;
    bool validNiTriggering( QString &err, DAQ::Params &q ) const;
    bool validImShankMap( QString &err, DAQ::Params &q ) const;
    bool validNiShankMap( QString &err, DAQ::Params &q ) const;
    bool validImChanMap( QString &err, DAQ::Params &q ) const;
    bool validNiChanMap( QString &err, DAQ::Params &q ) const;
    bool validImSaveBits( QString &err, DAQ::Params &q ) const;
    bool validNiSaveBits( QString &err, DAQ::Params &q ) const;
    bool validDiskAvail( QString &err, DAQ::Params &q ) const;
    bool shankParamsToQ( QString &err, DAQ::Params &q ) const;
    bool diskParamsToQ( QString &err, DAQ::Params &q ) const;
    bool valid( QString &err, bool isGUI = false );
};

#endif  // CONFIGCTL_H



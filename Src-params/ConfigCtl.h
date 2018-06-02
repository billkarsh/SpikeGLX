#ifndef CONFIGCTL_H
#define CONFIGCTL_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class ConfigureDialog;
class DevicesTab;
class IMCfgTab;
class NICfgTab;
class SyncTab;
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
    Ui::ConfigureDialog                 *cfgUI;
    Ui::DevicesTab                      *devTabUI;
    Ui::IMCfgTab                        *imTabUI;
    Ui::NICfgTab                        *niTabUI;
    Ui::SyncTab                         *syncTabUI;
    Ui::GateTab                         *gateTabUI;
    Ui::GateImmedPanel                  *gateImmPanelUI;
    Ui::GateTCPPanel                    *gateTCPPanelUI;
    Ui::TrigTab                         *trigTabUI;
    Ui::TrigImmedPanel                  *trigImmPanelUI;
    Ui::TrigTimedPanel                  *trigTimPanelUI;
    Ui::TrigTTLPanel                    *trigTTLPanelUI;
    Ui::TrigSpikePanel                  *trigSpkPanelUI;
    Ui::TrigTCPPanel                    *trigTCPPanelUI;
    Ui::MapTab                          *mapTabUI;
    Ui::SeeNSaveTab                     *snsTabUI;
    HelpButDialog                       *cfgDlg;
    QVector<QString>                    devNames;
    QSharedMemory                       *singleton;
    mutable QVector<CimCfg::AttrEach>   imGUI;
    int                                 imGUILast;
    bool                                imecOK,
                                        nidqOK;

public:
    CimCfg::ImProbeTable    prbTab; // filled in by detect();
    DAQ::Params             acceptedParams;
    bool                    validated;

public:
    ConfigCtl( QObject *parent = 0 );
    virtual ~ConfigCtl();

    bool showDialog();
    bool isConfigDlg( QObject *parent );

    void setParams( const DAQ::Params &p, bool write );

    void externSetsRunName( const QString &name );
    void graphSetsImroFile( const QString &file, int ip );
    void graphSetsStdbyStr( const QString &sdtbyStr, int ip );
    void graphSetsImChanMap( const QString &cmFile, int ip );
    void graphSetsNiChanMap( const QString &cmFile );
    void graphSetsImSaveStr( const QString &saveStr, int ip );
    void graphSetsNiSaveStr( const QString &saveStr );
    void graphSetsImSaveBit( int chan, bool setOn, int ip );
    void graphSetsNiSaveBit( int chan, bool setOn );

    bool validRunName(
        QString         &err,
        const QString   &runName,
        QWidget         *parent,
        bool            isGUI ) const;

    bool chanMapGetsShankOrder(
        QString         &s,
        const QString   type,
        bool            rev,
        QWidget         *parent ) const;

public slots:
    QString cmdSrvGetsSaveChansIm( uint ip ) const;
    QString cmdSrvGetsSaveChansNi() const;
    QString cmdSrvGetsParamStr() const;
    QString cmdSrvSetsParamStr( const QString &paramString );

private slots:
    void comCBChanged();
    void moreButClicked();
    void lessButClicked();
    void imPrbTabChanged();
    void nidqEnabClicked();
    void detectButClicked();
    void forceButClicked();
    void exploreButClicked();
    void stripButClicked();
    void otherProbeCBChanged();
    void copyButClicked();
    void imroButClicked();
    void updateCalWarning();
    void device1CBChanged();
    void device2CBChanged();
    void muxingChanged();
    void clk1CBChanged();
    void startEnableClicked( bool checked );
    void syncSourceCBChanged();
    void syncImChanTypeCBChanged();
    void syncNiChanTypeCBChanged();
    void syncCalChkClicked();
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
    void trigTTLAnalogChanged();
    void trigTTLModeChanged( int _mode );
    void trigTTLNInfClicked( bool checked );
    void trigSpkNInfClicked( bool checked );
    void probeCBChanged();
    void reset();
    void verify();
    void okButClicked();

private:
    bool singletonReserve();
    void singletonRelease();
    void setNoDialogAccess( bool clearNi = true );
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
    void initImProbeMap();
    void updtImProbeMap();
    void imGUI_Init( const DAQ::Params &p );
    void imGUI_ToDlg();
    void imGUI_FromDlg( int idst ) const;
    void imGUI_Copy( int idst, int isrc );
    void setupDevTab( const DAQ::Params &p );
    void setupImTab( const DAQ::Params &p );
    void setupNiTab( const DAQ::Params &p );
    void setupSyncTab( const DAQ::Params &p );
    void setupGateTab( const DAQ::Params &p );
    void setupTrigTab( const DAQ::Params &p );
    void setupMapTab( const DAQ::Params &p );
    void setupSnsTab( const DAQ::Params &p );
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
    bool validImROTbl( QString &err, CimCfg::AttrEach &E, int ip ) const;
    bool validImStdbyBits( QString &err, CimCfg::AttrEach &E ) const;
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
    bool validImSaveBits( QString &err, DAQ::Params &q, int ip ) const;
    bool validNiSaveBits( QString &err, DAQ::Params &q ) const;
    bool validSyncTab( QString &err, DAQ::Params &q ) const;
    bool validImTriggering( QString &err, DAQ::Params &q ) const;
    bool validNiTriggering( QString &err, DAQ::Params &q ) const;
    bool validImShankMap( QString &err, DAQ::Params &q, int ip ) const;
    bool validNiShankMap( QString &err, DAQ::Params &q ) const;
    bool validImChanMap( QString &err, DAQ::Params &q, int ip ) const;
    bool validNiChanMap( QString &err, DAQ::Params &q ) const;
    bool validDiskAvail( QString &err, DAQ::Params &q ) const;
    bool shankParamsToQ( QString &err, DAQ::Params &q ) const;
    bool diskParamsToQ( QString &err, DAQ::Params &q ) const;
    bool valid( QString &err, bool isGUI = false );
};

#endif  // CONFIGCTL_H



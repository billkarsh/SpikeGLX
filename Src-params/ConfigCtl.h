#ifndef CONFIGCTL_H
#define CONFIGCTL_H

#include "DAQ.h"

#include <QObject>
#include <QDialog>

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
class SeeNSaveTab;
}

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
    Ui::SeeNSaveTab     *snsTabUI;
    QDialog             *cfgDlg;
    QVector<QString>    devNames;
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
    void graphSetsImSaveBit( int chan, bool setOn );
    void graphSetsNiSaveBit( int chan, bool setOn );

    bool validRunName(
        QString         &err,
        const QString   &runName,
        QWidget         *parent,
        bool            isGUI );

public slots:
    QString cmdSrvGetsSaveChansIm() const;
    QString cmdSrvGetsSaveChansNi() const;
    QString cmdSrvGetsParamStr() const;
    QString cmdSrvSetsParamStr( const QString &paramString );

private slots:
    void skipDetect();
    void detect();
    void device1CBChanged();
    void device2CBChanged();
    void muxingChanged();
    void aiRangeChanged();
    void clk1CBChanged();
    void freqButClicked();
    void syncEnableClicked( bool checked );
    void gateModeChanged();
    void manOvShowButClicked( bool checked );
    void trigModeChanged();
    void imChnMapButClicked();
    void niChnMapButClicked();
    void runDirButClicked();
    void trigTimHInfClicked();
    void trigTimNInfClicked( bool checked );
    void trigTTLModeChanged( int _mode );
    void trigTTLNInfClicked( bool checked );
    void trigSpkNInfClicked( bool checked );
    void reset( DAQ::Params *pRemote = 0 );
    void verify();
    void okBut();

private:
    void setNoDialogAccess();
    void setSelectiveAccess();
    void imWrite( const QString &s );
    void imDetect();
    void niWrite( const QString &s );
    void niDetect();
    bool doingImec() const;
    bool doingNidq() const;
    void setupDevTab( DAQ::Params &p );
    void setupImTab( DAQ::Params &p );
    void setupNiTab( DAQ::Params &p );
    void setupGateTab( DAQ::Params &p );
    void setupTrigTab( DAQ::Params &p );
    void setupSnsTab( DAQ::Params &p );
    QString uiMNStr2FromDlg();
    QString uiMAStr2FromDlg();
    QString uiXAStr2FromDlg();
    QString uiXDStr2FromDlg();
    bool isMuxingFromDlg();
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
        QString         &uiStr2Err );
    bool validDevTab( QString &err, DAQ::Params &q );
    bool validNiDevices( QString &err, DAQ::Params &q );
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
        QString         &uiStr2Err );
    bool validTriggering( QString &err, DAQ::Params &q );
    bool validImChanMap( QString &err, DAQ::Params &q );
    bool validNiChanMap( QString &err, DAQ::Params &q );
    bool validImSaveBits( QString &err, DAQ::Params &q );
    bool validNiSaveBits( QString &err, DAQ::Params &q );
    bool valid( QString &err, bool isGUI = false );
};

#endif  // CONFIGCTL_H



#ifndef CONFIGCTL_H
#define CONFIGCTL_H

#include "DAQ.h"

#include <QObject>
#include <QDialog>

namespace Ui {
class ConfigureDialog;
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
    Ui::NICfgTab        *niCfgTabUI;
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
    QString             startupErr;

public:
    DAQ::Params     acceptedParams;

public:
    ConfigCtl( QObject *parent = 0 );
    virtual ~ConfigCtl();

    void showStartupMessage();
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
    QString cmdSrvGetsSaveChansIm();
    QString cmdSrvGetsSaveChansNi();
    QString cmdSrvGetsParamStr();
    QString cmdSrvSetsParamStr( const QString &paramString );

private slots:
    void device1CBChanged();
    void device2CBChanged();
    void muxingChanged();
    void aiRangeChanged();
    void clk1CBChanged();
    void freqButClicked();
    void syncEnableClicked( bool checked );
    void manOvShowButClicked( bool checked );
    void gateModeChanged();
    void trigModeChanged();
    void chnMapButClicked();
    void runDirButClicked();
    void reset( DAQ::Params *pRemote = 0 );
    void verify();
    void okBut();
    void trigTimHInfClicked();
    void trigTimNInfClicked( bool checked );
    void trigTTLModeChanged( int _mode );
    void trigTTLNInfClicked( bool checked );
    void trigSpkNInfClicked( bool checked );

private:
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
// BK: Need imec complements
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
    bool validNidqChanMap( QString &err, DAQ::Params &q );
    bool validNidqSaveBits( QString &err, DAQ::Params &q );
    bool valid( QString &err, bool isGUI = false );
};

#endif  // CONFIGCTL_H



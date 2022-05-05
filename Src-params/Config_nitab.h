#ifndef CONFIG_NITAB_H
#define CONFIG_NITAB_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class NITab;
class NISourceDlg;
}

class ConfigCtl;

class QSharedMemory;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Config_nitab : public QObject
{
    Q_OBJECT

private:
    struct NISrc {
        double  base,
                R0,
                maxrate,
                settle,
                saferate;
        QString dev;
        int     nAI;
        bool    simsam,
                exttrig;
    };

    NISrc               nisrc;

private:
    Ui::NITab           *niTabUI;
    Ui::NISourceDlg     *sourceUI;
    ConfigCtl           *cfg;
    QVector<QString>    devNames;
    QSharedMemory       *singleton;

public:
    Config_nitab( ConfigCtl *cfg, QWidget *tab );
    virtual ~Config_nitab();

    void toGUI( const DAQ::Params &p );
    void fromGUI(
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

    bool singletonReserve();
    void singletonRelease();

    const QString &curDevName() const;
    bool isMuxingFromDlg() const;
    int nSources() const;

    bool remoteValidate( QString &err, const DAQ::Params &p );

private slots:
    void device1CBChanged();
    void device2CBChanged();
    void muxingChanged();
    void clkSourceCBChanged();
    void startEnableClicked( bool checked );
    void niShkMapBut();
    void niChnMapBut();
    void newSourceBut();
    void sourceSettleChanged();
    void sourceMaxChecked();
    void sourceSafeChecked();
    void sourceWhisperChecked();
    void sourceEnabItems();
    void sourceDivChanged( int i );
    void sourceSetDiv( int i );
    void sourceMakeName();

private:
    void setupNiVRangeCB();
    QString uiMNStr2FromDlg() const;
    QString uiMAStr2FromDlg() const;
    QString uiXAStr2FromDlg() const;
    QString uiXDStr2FromDlg() const;
    bool niChannelsFromDialog( CniCfg &ni ) const;
};

#endif  // CONFIG_NITAB_H



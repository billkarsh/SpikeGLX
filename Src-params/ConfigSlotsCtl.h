#ifndef CONFIGSLOTSCTL_H
#define CONFIGSLOTSCTL_H

#include "CimCfg.h"

#include <QObject>

namespace Ui {
class ConfigSlotsDialog;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ConfigSlotsCtl : public QObject
{
    Q_OBJECT

private:
    QDialog                     *csDlg;
    Ui::ConfigSlotsDialog       *csUI;
    CimCfg::ImProbeTable        &prbTab;
    QVector<CimCfg::CfgSlot>    vCS;

public:
    ConfigSlotsCtl( QObject *parent, CimCfg::ImProbeTable &prbTab );

    bool run();

private slots:
    void selectionChanged();
    void slotCBChanged( int sel );
    void showCBChanged( int sel );
    void removeBut();
    void helpBut();
    void detectBut();
    void okBut();
    void cancelBut();

private:
    void toGUI();
    bool fromGUI();
};

#endif  // CONFIGSLOTSCTL_H



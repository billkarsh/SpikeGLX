#ifndef CONFIGSLOTSCTL_H
#define CONFIGSLOTSCTL_H

#include "CimCfg.h"

#include <QDialog>

namespace Ui {
class ConfigSlotsDialog;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ConfigSlotsCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::ConfigSlotsDialog       *csUI;
    CimCfg::ImProbeTable        &prbTab;
    QVector<CimCfg::CfgSlot>    vCS;

public:
    ConfigSlotsCtl( QWidget *parent, CimCfg::ImProbeTable &prbTab );
    virtual ~ConfigSlotsCtl();

    bool run();

private slots:
    void addBut();
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



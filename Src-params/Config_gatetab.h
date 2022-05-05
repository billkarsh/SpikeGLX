#ifndef CONFIG_GATETAB_H
#define CONFIG_GATETAB_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class GateTab;
class GateImmedPanel;
class GateTCPPanel;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Config_gatetab : public QObject
{
    Q_OBJECT

private:
    Ui::GateTab         *gateTabUI;
    Ui::GateImmedPanel  *gateImmPanelUI;
    Ui::GateTCPPanel    *gateTCPPanelUI;

public:
    Config_gatetab( QWidget *tab );
    virtual ~Config_gatetab();

    void toGUI( const DAQ::Params &p );
    void fromGUI( DAQ::Params &q );

private slots:
    void gateModeChanged();
    void manOvShowBut( bool checked );
};

#endif  // CONFIG_GATETAB_H



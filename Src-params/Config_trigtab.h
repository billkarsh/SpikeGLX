#ifndef CONFIG_TRIGTAB_H
#define CONFIG_TRIGTAB_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class TrigTab;
class TrigImmedPanel;
class TrigTimedPanel;
class TrigTTLPanel;
class TrigSpikePanel;
class TrigTCPPanel;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Config_trigtab : public QObject
{
    Q_OBJECT

private:
    Ui::TrigTab         *trigTabUI;
    Ui::TrigImmedPanel  *trigImmPanelUI;
    Ui::TrigTimedPanel  *trigTimPanelUI;
    Ui::TrigTTLPanel    *trigTTLPanelUI;
    Ui::TrigSpikePanel  *trigSpkPanelUI;
    Ui::TrigTCPPanel    *trigTCPPanelUI;

public:
    Config_trigtab( QWidget *tab );
    virtual ~Config_trigtab();

    void toGUI( const DAQ::Params &p );
    void fromGUI( DAQ::Params &q );

private slots:
    void trigModeChanged();
    void trigTimHInfClicked();
    void trigTimNInfClicked( bool checked );
    void trigTTLAnalogChanged();
    void trigTTLModeChanged( int _mode );
    void trigTTLNInfClicked( bool checked );
    void trigSpkNInfClicked( bool checked );
};

#endif  // CONFIG_TRIGTAB_H



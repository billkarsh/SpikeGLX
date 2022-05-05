#ifndef CONFIG_SYNCTAB_H
#define CONFIG_SYNCTAB_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class SyncTab;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Config_synctab : public QObject
{
    Q_OBJECT

private:
    Ui::SyncTab *syncTabUI;

public:
    Config_synctab( QWidget *tab );
    virtual ~Config_synctab();

    void toGUI( const DAQ::Params &p );
    void fromGUI( DAQ::Params &q );

    void resetCalRunMode();

    DAQ::SyncSource curSource() const;

public slots:
    void syncSourceCBChanged();

private slots:
    void syncNiChanTypeCBChanged();
    void syncCalChkClicked();
};

#endif  // CONFIG_SYNCTAB_H



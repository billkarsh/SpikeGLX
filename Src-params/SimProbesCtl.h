#ifndef SIMPROBESCTL_H
#define SIMPROBESCTL_H

#include "SimProbes.h"

#include <QObject>

namespace Ui {
class SimProbesDialog;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SimProbesCtl : public QObject
{
    Q_OBJECT

private:
    QDialog                 *spDlg;
    Ui::SimProbesDialog     *spUI;
    SimProbes               &SP;
    QMap<SPAddr,QString>    maddr;
    int                     clkRow;

public:
    SimProbesCtl( QObject *parent, SimProbes &SP );
    virtual ~SimProbesCtl();

    void run();

private slots:
    void sortBut();
    void addBut();
    void rmvBut();
    void helpBut();
    void okBut();
    void cancelBut();
    void cellDoubleClicked( int row, int col );
    void editPath();

private:
    SPAddr lowestAvailAddr();
    SPAddr selectedAddr();
    int addr2row( SPAddr &A );
    void selectAddr( SPAddr &A );
    bool selectFile( QString &file );
    void toTable();
    bool fromTable();
};

#endif  // SIMPROBESCTL_H



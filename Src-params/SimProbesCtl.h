#ifndef SIMPROBESCTL_H
#define SIMPROBESCTL_H

#include "SimProbes.h"

#include <QDialog>

namespace Ui {
class SimProbesDialog;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SimProbesCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::SimProbesDialog     *spUI;
    SimProbes               &SP;
    QMap<SPAddr,QString>    maddr;
    int                     clkRow;

public:
    SimProbesCtl( QWidget *parent, SimProbes &SP );
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



#ifndef SAVECHANSCTL_H
#define SAVECHANSCTL_H

#include "CimCfg.h"

#include <QWidget>

namespace Ui {
class IMSaveChansDlg;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SaveChansCtl : public QObject
{
    Q_OBJECT

private:
    QWidget                 *parent;
    QDialog                 *svDlg;
    Ui::IMSaveChansDlg      *svUI;
    const CimCfg::PrbEach   &E;
    int                     ip,
                            nAP,
                            nLF,
                            nSY;

public:
    SaveChansCtl( QWidget *parent, const CimCfg::PrbEach &E, int ip );
    virtual ~SaveChansCtl();

    bool edit( QString &out_uistr, bool &lfPairChk );

private slots:
    void setBut();
};

#endif  // SAVECHANSCTL_H



#ifndef SAVECHANSCTL_H
#define SAVECHANSCTL_H

#include "CimCfg.h"

#include <QDialog>

namespace Ui {
class IMSaveChansDlg;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SaveChansCtl : public QDialog
{
    Q_OBJECT

private:
    QWidget                 *parent;
    Ui::IMSaveChansDlg      *svUI;
    const CimCfg::PrbEach   &E;
    int                     ip,
                            nAP,
                            nLF,
                            nSY;

public:
    SaveChansCtl( QWidget *parent, const CimCfg::PrbEach &E, int ip );
    virtual ~SaveChansCtl();

    bool edit( QString &uistr, bool &lfPairChk );

private slots:
    void applyBut();

private:
    bool getCurBits( QBitArray &b );
};

#endif  // SAVECHANSCTL_H



#ifndef IMHSTCTL_H
#define IMHSTCTL_H

#ifdef HAVE_IMEC

#include "IMEC/NeuropixAPI.h"

#include <QDialog>

namespace Ui {
class IMBISTDlg;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMHSTCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::IMBISTDlg       *hstUI;
    std::vector<int>    openSlots;
    bool                isHelloText;

public:
    IMHSTCtl();
    virtual ~IMHSTCtl();

private slots:
    void go();
    void helpBut();
    void clear();
    void save();

private:
    QString getErrorStr();
    bool okVersions();
    void write( const QString &s );
    bool _openSlot();
    void _closeSlots();
    void _closeHST();
    bool stdStart( int itest, int secs = 0 );
    bool stdTest( const QString &fun, Neuropixels::NP_ErrorCode err );

    void test_runAll();
    bool test_communication();
    bool test_supplyVoltages();
    bool test_controlSignals();
    bool test_masterClock();
    bool test_PSBDataBus();
    bool test_signalGenerator();
};

#endif  // HAVE_IMEC

#endif  // IMHSTCTL_H



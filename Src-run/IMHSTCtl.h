#ifndef IMHSTCTL_H
#define IMHSTCTL_H

#ifdef HAVE_IMEC

#include "IMEC/NeuropixAPI.h"

#include <QObject>

namespace Ui {
class IMBISTDlg;
}

class HelpButDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMHSTCtl : public QObject
{
    Q_OBJECT

private:
    HelpButDialog       *dlg;
    Ui::IMBISTDlg       *hstUI;
    std::vector<int>    openSlots;
    bool                isHelloText;

public:
    IMHSTCtl( QObject *parent = 0 );
    virtual ~IMHSTCtl();

private slots:
    void go();
    void clear();
    void save();

private:
    QString getErrorStr();
    void write( const QString &s );
    bool _openSlot();
    void _closeSlots();
    void _closeHST();
    bool stdStart( int itest, int secs = 0 );
    bool stdTest( const QString &fun, Neuropixels::NP_ErrorCode err );

    void test_runAll();
    void test_communication();
    void test_supplyVoltages();
    void test_controlSignals();
    void test_masterClock();
    void test_PSBDataBus();
    void test_signalGenerator();
};

#endif  // HAVE_IMEC

#endif  // IMHSTCTL_H



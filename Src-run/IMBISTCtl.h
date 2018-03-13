#ifndef IMBISTCTL_H
#define IMBISTCTL_H

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

class IMBISTCtl : public QObject
{
    Q_OBJECT

private:
    NeuropixAPI     IM;
    HelpButDialog   *dlg;
    Ui::IMBISTDlg   *bistUI;
    bool            isClosed;

public:
    IMBISTCtl( QObject *parent = 0 );
    virtual ~IMBISTCtl();

private slots:
    void comCBChanged( int icom );
    void go();
    void clear();
    void save();

private:
    void write( const QString &s );
    bool open();
    bool openProbe();
    void close();
    bool stdStart();
    void stdFinish( int err );
    void test_bistBS();
    void test_bistHB();
    void test_bistPRBS();
    void test_bistI2CMM();
    void test_bistEEPROM();
    void test_bistSR();
    void test_bistPSB();
    void test_bistSignal();
    void test_bistNoise();
    void test_HSTestVDDDA1V2();
    void test_HSTestVDDDD1V2();
    void test_HSTestVDDDA1V8();
    void test_HSTestVDDDD1V8();
    void test_HSTestOscillator();
    void test_HSTestMCLK();
    void test_HSTestPCLK();
    void test_HSTestPSB();
    void test_HSTestI2C();
    void test_HSTestNRST();
    void test_HSTestREC_NRESET();
};

#endif  // HAVE_IMEC

#endif  // IMBISTCTL_H



#ifndef IMBISTCTL_H
#define IMBISTCTL_H

#ifdef HAVE_IMEC

#include "IMEC/NeuropixAPI.h"

#include <QDialog>

namespace Ui {
class IMBISTDlg;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMBISTCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::IMBISTDlg       *bistUI;
    std::vector<int>    openSlots4;
    QString             pn;
    int                 type;
    bool                testEEPROM;

public:
    IMBISTCtl();
    virtual ~IMBISTCtl();

private slots:
    void go();
    void helpBut();
    void clear();
    void save();

private:
    void write( const QString &s );
    void writeMapMsg( int slot );
    bool _openSlot();
    void _closeSlots();
    bool okVersions();
    bool probeType();
    bool EEPROMCheck();
    bool openProbe();
    void closeProbe();
    void stdStart( int itest, int secs = 0 );
    void stdFinish4( Neuropixels::NP_ErrorCode err );
    void stdFinish();
    bool test_bistBS();
    void test_bistHB();
    void test_bistPRBS();
    void test_bistI2CMM();
    void test_bistEEPROM();
    void test_bistSR();
    void test_bistPSB();
    void test_bistSignal();
    void test_bistNoise();
};

#endif  // HAVE_IMEC

#endif  // IMBISTCTL_H



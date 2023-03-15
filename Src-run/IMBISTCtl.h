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
    HelpButDialog       *dlg;
    Ui::IMBISTDlg       *bistUI;
    std::vector<int>    openSlots;
    QString             pn;
    int                 type;
    bool                testEEPROM;

public:
    IMBISTCtl( QObject *parent = 0 );
    virtual ~IMBISTCtl();

private slots:
    void go();
    void clear();
    void save();

private:
    void write( const QString &s );
    void writeMapMsg( int slot );
    bool _openSlot();
    void _closeSlots();
    bool _openProbe();
    void _closeProbe();
    bool probeType();
    bool EEPROMCheck();
    bool stdStart( int itest, int secs = 0 );
    void stdFinish( Neuropixels::NP_ErrorCode err );
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



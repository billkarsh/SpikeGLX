#ifndef IMBISTCTL_H
#define IMBISTCTL_H

#ifdef HAVE_IMEC
#if 0

#include "IMEC/Neuropix_basestation_api.h"

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
    Neuropix_basestation_api    IM;
    HelpButDialog               *dlg;
    Ui::IMBISTDlg               *bistUI;
    bool                        isClosed;

public:
    IMBISTCtl( QObject *parent = 0 );
    virtual ~IMBISTCtl();

private slots:
    void go();
    void clear();
    void save();

private:
    void write( const QString &s );
    bool open();
    void close();
    void test4();
    void test5();
    void test6();
    void test7();
    void test8();
    void test9();
};

#endif  // under development
#endif  // HAVE_IMEC

#endif  // IMBISTCTL_H



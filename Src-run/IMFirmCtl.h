#ifndef IMFIRMCTL_H
#define IMFIRMCTL_H

#ifdef HAVE_IMEC

#include <QDialog>

namespace Ui {
class IMFirmDlg;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMFirmCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::IMFirmDlg   *firmUI;
    int             tech,
                    jobBits,    // {0=none,1=BS,2=BSC}
                    bsBytes,
                    bscBytes,
                    barOffset;

public:
    IMFirmCtl( QWidget *parent = 0 );
    virtual ~IMFirmCtl();

private slots:
    void slotChanged();
    void update();
    void helpBut();

private:
    void beep( const QString &msg );
    void write( const QString &s );
    void verInit( const QString &s );
    bool verGet( int slot );
    void verEval();
    bool paths( QString &bs, QString &bsc );
    QString verToBuild( const QString &vers );
    static int callback( size_t bytes );
};

#endif  // HAVE_IMEC

#endif  // IMFIRMCTL_H



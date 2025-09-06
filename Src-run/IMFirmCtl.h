#ifndef IMFIRMCTL_H
#define IMFIRMCTL_H

#ifdef HAVE_IMEC

#include <QObject>

namespace Ui {
class IMFirmDlg;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMFirmCtl : public QObject
{
    Q_OBJECT

private:
    QDialog         *dlg;
    Ui::IMFirmDlg   *firmUI;
    int             tech,
                    jobBits,    // {0=none,1=BS,2=BSC}
                    bsBytes,
                    bscBytes,
                    barOffset;

public:
    IMFirmCtl( QObject *parent = 0 );
    virtual ~IMFirmCtl();

private slots:
    void slotChanged();
    void update();
    void helpBut();

private:
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



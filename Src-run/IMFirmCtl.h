#ifndef IMFIRMCTL_H
#define IMFIRMCTL_H

#ifdef HAVE_IMEC

#include <QObject>

namespace Ui {
class IMFirmDlg;
}

class HelpButDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMFirmCtl : public QObject
{
    Q_OBJECT

private:
    HelpButDialog   *dlg;
    Ui::IMFirmDlg   *firmUI;

public:
    IMFirmCtl( QObject *parent = 0 );
    virtual ~IMFirmCtl();

private slots:
    void detect();
    void bsBrowse();
    void bscBrowse();
    void update();

private:
    static int callback( size_t bytes );
};

#endif  // HAVE_IMEC

#endif  // IMFIRMCTL_H



#ifndef WAVEPLANCTL_H
#define WAVEPLANCTL_H

#include <QObject>

namespace Ui {
class WavePlanDialog;
}

class HelpButDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class WavePlanCtl : public QObject
{
    Q_OBJECT

private:
    HelpButDialog       *wvDlg;
    Ui::WavePlanDialog  *wvUI;
    QString             type;
    int                 binsamp;

public:
    WavePlanCtl( QObject *parent = 0 );
    virtual ~WavePlanCtl();

private slots:
    void devChanged();
    void devVChanged();
    void newBut();
    void loadBut();
    bool checkBut();
    void saveBut();
    void closeBut();

private:
    void info( const QString &msg );
    void beep( const QString &msg );
};

#endif  // WAVEPLANCTL_H



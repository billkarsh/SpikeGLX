#ifndef WAVEPLANCTL_H
#define WAVEPLANCTL_H

#include <QObject>

namespace Ui {
class WavePlanDialog;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class WavePlanCtl : public QObject
{
    Q_OBJECT

private:
    QDialog             *wvDlg;
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
    void helpBut();
    bool checkBut();
    void saveBut();
    void closeBut();

private:
    QString makeWaves();
    void info( const QString &msg );
    void beep( const QString &msg );
};

#endif  // WAVEPLANCTL_H



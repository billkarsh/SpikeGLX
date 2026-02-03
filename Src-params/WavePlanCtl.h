#ifndef WAVEPLANCTL_H
#define WAVEPLANCTL_H

#include <QDialog>

namespace Ui {
class WavePlanDialog;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class WavePlanCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::WavePlanDialog  *wvUI;
    QString             type;
    int                 binsamp;

public:
    WavePlanCtl();
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



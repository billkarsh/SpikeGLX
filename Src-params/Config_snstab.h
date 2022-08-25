#ifndef CONFIG_SNSTAB_H
#define CONFIG_SNSTAB_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class SNSTab;
}

class MainApp;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Config_snstab : public QObject
{
    Q_OBJECT

private:
    Ui::SNSTab  *snsTabUI;
    MainApp     *app;

public:
    Config_snstab( QWidget *tab );
    virtual ~Config_snstab();

    void toGUI( const DAQ::Params &p );
    void fromGUI( DAQ::Params &q );

    void setDialogNoAccess();
    void setNotes( const QString &s );
    void setRunName( const QString &name );

private slots:
    void dataDirBut();
    void diskBut();

private:
    void setDataDirLbl() const;
    void diskWrite( const QString &s );
};

#endif  // CONFIG_SNSTAB_H



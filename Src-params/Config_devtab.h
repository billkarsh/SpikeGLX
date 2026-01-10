#ifndef CONFIG_DEVTAB_H
#define CONFIG_DEVTAB_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class DevTab;
}

class ConfigCtl;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Config_devtab : public QObject
{
    Q_OBJECT

private:
    Ui::DevTab  *devTabUI;
    ConfigCtl   *cfg;

public:
    Config_devtab( ConfigCtl *cfg, QWidget *tab );
    virtual ~Config_devtab();

    void toGUI( const DAQ::Params &p, bool loadProbes );
    void fromGUI( DAQ::Params &q );

    void setNoDialogAccess( bool clearNi );

    void showCalWarning( bool show );
    void updateIMParams();
    void updateObxIDs( DAQ::Params &q );
    void remoteDetect( ERRLVL &R );

private slots:
    void cfgSlotsBut();
    void simPrbBut();
    void imPrbTabCellChng( int row, int col );
    void psvSaveSettings(
        const QString   &key,
        const QString   &pnval,
        const QString   &snval );
    void hssnSaveSettings( const QString &key, const QString &val );
    void detectBut();

private:
    void detect( ERRLVL &R );
    void prbTabToGUI();
    void imWrite( const QString &s );
    void imWriteCurrent();
    bool imDetect( ERRLVL &R );
    void psvPNRemote( const QVector<int> &vP );
    void psvPNDialog( const QVector<int> &vP );
    void HSSNDialog( const QVector<int> &vP );
    void niWrite( const QString &s );
    QColor niSetColor( const QColor &c );
    bool niDetect( ERRLVL &R );
};

#endif  // CONFIG_DEVTAB_H



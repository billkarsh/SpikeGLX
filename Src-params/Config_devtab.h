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

private slots:
    void cfgSlotsBut();
    void simPrbBut();
    void imPrbTabCellChng( int row, int col );
    void hssnSaveSettings( const QString &key, const QString &val );
    void detectBut();

private:
    void prbTabToGUI();
    void imWrite( const QString &s );
    void imWriteCurrent();
    bool imDetect();
    void HSSNDialog( QVector<int> &vP );
    void niWrite( const QString &s );
    QColor niSetColor( const QColor &c );
    bool niDetect();
};

#endif  // CONFIG_DEVTAB_H


#ifndef CONFIG_OBXTAB_H
#define CONFIG_OBXTAB_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class ObxTab;
}

class ConfigCtl;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Config_obxtab : public QObject
{
    Q_OBJECT

private:
    Ui::ObxTab                  *obxTabUI;
    ConfigCtl                   *cfg;
    QMap<int,CimCfg::ObxEach>   sn2set;
    QVector<CimCfg::ObxEach>    each;

public:
    Config_obxtab( ConfigCtl *cfg, QWidget *tab );
    virtual ~Config_obxtab();

    void toGUI();
    void fromGUI( DAQ::Params &q );

    void updateSaveChans( CimCfg::ObxEach &E, int isel );
    void updateObx( const CimCfg::ObxEach &E, int isel );
    void saveSettings();

    QString remoteSetObxEach( const QString &s, int istr );

private slots:
    void selectionChanged();
    void cellDoubleClicked( int ip, int col );
    void editChan();
    void cellChanged( int ip, int col );
    void defBut();
    void copyAllBut();
    void copyToBut();

private:
    void loadSettings();
    void onDetect();
    void toTbl();
    void fromTbl();
    void toTbl( int ip );
    void fromTbl( int ip );
    void copy( int idst, int isrc );
    int curObx();
};

#endif  // CONFIG_OBXTAB_H



#ifndef CONFIG_IMTAB_H
#define CONFIG_IMTAB_H

#include "DAQ.h"

#include <QObject>

namespace Ui {
class IMTab;
}

class ConfigCtl;
class ShankCtlBase;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Config_imtab : public QObject
{
    Q_OBJECT

private:
    Ui::IMTab                       *imTabUI;
    ConfigCtl                       *cfg;
    QMap<quint64,CimCfg::PrbEach>   sn2set;
    QVector<CimCfg::PrbEach>        each;
    int                             imro_ip;
    bool                            pairChk;

public:
    Config_imtab( ConfigCtl *cfg, QWidget *tab );
    virtual ~Config_imtab();

    void toGUI( const DAQ::Params &p );
    void fromGUI( DAQ::Params &q );

    void reset( const DAQ::Params &p );
    bool calPolicyIsNever() const;

    void regularizeSaveChans( CimCfg::PrbEach &E, int nC, int ip );
    void updateProbe( const CimCfg::PrbEach &E, int ip );
    void saveSettings();

    QString remoteVfyPrbAll( const CimCfg &next, const CimCfg &prev );
    QString remoteSetPrbEach( const QString &s, int ip );

public slots:
    void imro_done( ShankCtlBase *editor, QString fn, bool ok );

private slots:
    void svyChkClicked( bool scroll = true );
    void selectionChanged();
    void cellDoubleClicked( int ip, int col );
    void editIMRO();
    void editShank();
    void editChan();
    void editSave();
    void forceBut();
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
    int curProbe();
};

#endif  // CONFIG_IMTAB_H



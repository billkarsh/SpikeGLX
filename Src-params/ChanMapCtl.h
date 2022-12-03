#ifndef CHANMAPCTL_H
#define CHANMAPCTL_H

#include "ChanMap.h"

#include <QObject>

namespace Ui {
class ChanMappingDlg;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ChanMapCtl : public QObject
{
    Q_OBJECT

private:
    QDialog             *mapDlg;
    Ui::ChanMappingDlg  *mapUI;
    const ChanMap       &D;
    ChanMap             *Mref,
                        *Mcur;
    QString             iniFile,
                        refFile,
                        lastDir;
    int                 ip;

public:
    ChanMapCtl( QObject *parent, const ChanMap &defMap );
    virtual ~ChanMapCtl();

    QString edit( const QString &file, int ip );

private slots:
    void applyAutoBut( int idx = -1 );
    void applyListBut();
    void loadBut();
    void saveBut();
    void okBut();
    void cancelBut();

private:
    void defaultOrder();
    void createMcur();
    void copyMcur2ref();
    void loadSettings();
    void saveSettings() const;
    void emptyTable();
    void Mcur2table();
    bool table2Mcur();
    void loadFile( const QString &file );
    void theseChansToTop( const QString &s );
};

#endif  // CHANMAPCTL_H



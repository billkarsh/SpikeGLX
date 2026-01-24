#ifndef CHANMAPCTL_H
#define CHANMAPCTL_H

#include "ChanMap.h"

#include <QDialog>

namespace Ui {
class ChanMappingDlg;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ChanMapCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::ChanMappingDlg  *mapUI;
    const ChanMap       &D;
    ChanMap             *Mref,
                        *Mcur;
    QString             iniFile,
                        refFile,
                        lastDir;
    int                 ip;

public:
    ChanMapCtl( QWidget *parent, const ChanMap &defMap );
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



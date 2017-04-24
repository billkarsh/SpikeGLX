#ifndef CHANMAPCTL_H
#define CHANMAPCTL_H

#include "ChanMap.h"

#include <QObject>

namespace Ui {
class ChanMapping;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ChanMapCtl : public QObject
{
    Q_OBJECT

private:
    QDialog         *mapDlg;
    Ui::ChanMapping *mapUI;
    const ChanMap   &D;
    ChanMap         *M0,
                    *M;
    QString         inFile,
                    M0File,
                    lastDir;

public:
    ChanMapCtl( QObject *parent, const ChanMap &defMap );
    virtual ~ChanMapCtl();

    QString Edit( const QString &file );

private slots:
    void applyAutoBut();
    void applyListBut();
    void loadBut();
    void saveBut();
    void okBut();
    void cancelBut();

private:
    void defaultOrder();
    void createM();
    void copyM2M0();
    void loadSettings();
    void saveSettings() const;
    void emptyTable();
    void M2Table();
    bool table2M();
    void loadFile( const QString &file );
    void theseChansToTop( const QString &s );
};

#endif  // CHANMAPCTL_H



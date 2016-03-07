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
    ChanMap         &D;
    ChanMap         *M0,
                    *M;
    QString         inFile,
                    M0File,
                    lastDir;

public:
    ChanMapCtl( QObject *parent, ChanMap &D );
    virtual ~ChanMapCtl();

    QString Edit( const QString &file );

private slots:
    void defaultBut();
    void loadBut();
    void saveBut();
    void okBut();
    void cancelBut();

private:
    void createM();
    void copyM2M0();
    void loadSettings();
    void saveSettings() const;
    void emptyTable();
    void M2Table();
    bool Table2M();
    void loadFile( const QString &file );
};

#endif  // CHANMAPCTL_H



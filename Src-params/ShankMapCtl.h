#ifndef SHANKMAPCTL_H
#define SHANKMAPCTL_H

#include "ShankMap.h"

#include <QObject>

namespace Ui {
class ShankMapping;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankMapCtl : public QObject
{
    Q_OBJECT

private:
    QDialog             *mapDlg;
    Ui::ShankMapping    *mapUI;
    const IMROTbl       &imro;
    const ShankMap      &D;
    ShankMap            *M0,
                        *M;
    QString             type,
                        inFile,
                        M0File,
                        lastDir;

public:
    ShankMapCtl(
        QObject         *parent,
        const IMROTbl   &imro,
        const ShankMap  &defMap,
        const QString   &type );
    virtual ~ShankMapCtl();

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

#endif  // SHANKMAPCTL_H



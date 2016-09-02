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
    ShankMap            *M0,
                        *M;
    QString             type,
                        inFile,
                        M0File,
                        lastDir;
    const int           nChan;
    int                 NS, NC, NR;

public:
    ShankMapCtl(
        QObject         *parent,
        const IMROTbl   &imro,
        const QString   &type,
        const int       nChan );
    virtual ~ShankMapCtl();

    QString Edit( const QString &file );

private slots:
    void hdrChanged();
    void defaultBut();
    void loadBut();
    void saveBut();
    void okBut();
    void cancelBut();

private:
    void emptyM();
    void copyM2M0();
    void loadSettings();
    void saveSettings() const;
    void emptyTable();
    void M2Table();
    bool Table2M();
    void loadFile( const QString &file );
};

#endif  // SHANKMAPCTL_H



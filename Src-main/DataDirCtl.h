#ifndef DATADIRCTL_H
#define DATADIRCTL_H

#include <QObject>
#include <QStringList>

namespace Ui {
class DataDirDialog;
}

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DataDirCtl : public QObject
{
    Q_OBJECT

private:
    QDialog             *ddDlg;
    Ui::DataDirDialog   *ddUI;
    QStringList         sl;
    bool                isMD;

public:
    DataDirCtl( QObject *parent, const QStringList &sl, bool isMD );
    virtual ~DataDirCtl();

    void run();

private slots:
    void mainBut();
    void addBut();
    void rmvBut();
    void editBut();
    void okBut();
    void cancelBut();

private:
    bool selectDir( QString &dir_inout );
    void resizeSL( int n );
    void updateMain();
    void updateTable();
    void toDialog();
    bool fromDialog();
};

#endif  // DATADIRCTL_H



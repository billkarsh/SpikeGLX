#ifndef DATADIRCTL_H
#define DATADIRCTL_H

#include <QDialog>
#include <QStringList>

namespace Ui {
class DataDirDialog;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DataDirCtl : public QDialog
{
    Q_OBJECT

private:
    Ui::DataDirDialog   *ddUI;
    QStringList         sl;
    bool                isMD;

public:
    DataDirCtl( const QStringList &sl, bool isMD );
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



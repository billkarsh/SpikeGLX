#ifndef IMROEDITOR_T21_H
#define IMROEDITOR_T21_H

#include <QObject>

namespace Ui {
class IMROEditor_T21;
}

struct IMROTbl_T21;

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMROEditor_T21 : public QObject
{
    Q_OBJECT

private:
    QDialog             *edDlg;
    Ui::IMROEditor_T21  *edUI;
    IMROTbl_T21         *R0,
                        *R;
    QString             inFile,
                        R0File,
                        lastDir;
    quint32             type;
    bool                running;

public:
    IMROEditor_T21( QObject *parent );
    virtual ~IMROEditor_T21();

    bool Edit( QString &outFile, const QString &file, int selectRow );

private slots:
    void defaultBut();
    void loadBut();
    void okBut();
    void cancelBut();

private:
    void createR();
    void copyR2R0();
    void loadSettings();
    void saveSettings() const;
    void loadFile( const QString &file );
};

#endif  // IMROEDITOR_T21_H



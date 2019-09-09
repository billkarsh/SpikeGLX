#ifndef IMROEDITOR_T24_H
#define IMROEDITOR_T24_H

#include <QObject>

namespace Ui {
class IMROEditor_T24;
}

struct IMROTbl_T24;

class QDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMROEditor_T24 : public QObject
{
    Q_OBJECT

private:
    QDialog             *edDlg;
    Ui::IMROEditor_T24  *edUI;
    IMROTbl_T24         *R0,
                        *R;
    QString             inFile,
                        R0File,
                        lastDir;
    quint32             type;
    bool                running;

public:
    IMROEditor_T24( QObject *parent );
    virtual ~IMROEditor_T24();

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

#endif  // IMROEDITOR_T24_H



#ifndef EXPORTCTL_H
#define EXPORTCTL_H

#include <QObject>
#include <QBitArray>
#include <QString>

namespace Ui {
class ExportDialog;
}

class DataFile;
class FileViewerWindow;

class QDialog;
class QFileInfo;
class QWidget;
class QProgressDialog;
class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ExportCtl: public QObject
{
        Q_OBJECT

private:
    struct ExportParams
    {
        enum Radio {
            // format
            bin     = 0,
            csv     = 1,
            // grf or scn
            all     = 0,
            sel     = 1,
            custom  = 2
        };

        // INPUT parameters (not modified by the dialog)
        QBitArray   inGrfVisBits;
        qint64      inScnsMax,
                    inScnSelFrom,
                    inScnSelTo;
        int         inNG;

        // IN/OUT parameters (modified by the dialog)
        QString     filename;   // < required input
        QBitArray   grfBits;    // < input prepopulates grfCustomLE
        qint64      scnFrom,    // < input determines scnR value
                    scnTo;      // < input determines scnR value
        Radio       fmtR,       // < from settings
                    grfR,       // < from settings
                    scnR;       // < from caller inputs

        ExportParams();
        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;
    };

private:
    QDialog             *dlg;
    Ui::ExportDialog    *expUI;
    ExportParams        E;
    const DataFile      *df;    // one stream at a time
    FileViewerWindow    *fvw;   // to get gains

public:
    ExportCtl( QWidget *parent = 0 );
    virtual ~ExportCtl();

    void loadSettings( QSettings &S )       {E.loadSettings( S );}
    void saveSettings( QSettings &S ) const {E.saveSettings( S );}

    // Call these setters in order-
    // 1) initDataFile
    // 2) initGrfRange
    // 3) initScnRange
    // 4) showExportDlg

    void initDataFile( const DataFile *df );
    void initGrfRange( const QBitArray &visBits, int curSel );
    void initTimeRange( qint64 selFrom, qint64 selTo );

    // Note that fvw is not const because doExport calls
    // dataFile.readFile which does a seek on the file,
    // hence, alters the file state. Otherwise, exporting
    // <IS> effectively const.

    bool showExportDlg( FileViewerWindow *fvw );

private slots:
    void browseButClicked();
    void formatChanged();
    void graphsChanged();
    void scansChanged();
    void okBut();

private:
    QString sglFilename( const QFileInfo &fi );
    void dialogFromParams();
    int customLE2Bits( QBitArray &bits, bool warn );
    void estimateFileSize();
    bool validateSettings();
    void doExport();
    bool exportAsBinary(
        QProgressDialog &progress,
        qint64          nscans,
        qint64          step );
    bool exportAsText(
        QProgressDialog &progress,
        qint64          nscans,
        qint64          step );
};

#endif  // EXPORTCTL_H



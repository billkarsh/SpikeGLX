#ifndef HELPBUTDIALOG_H
#define HELPBUTDIALOG_H

#include <QDialog>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class HelpButDialog : public QDialog
{
    Q_OBJECT

private:
    QDialog         *helpDlg;
    const QString   title,
                    filename;

public:
    HelpButDialog(
        const QString   &title,
        const QString   &filename,
        QWidget         *parent = 0 )
        :   QDialog(parent),
            helpDlg(0), title(title), filename(filename)    {}
    virtual ~HelpButDialog();

protected:
    virtual bool event( QEvent *e );
    virtual void closeEvent( QCloseEvent* );

private:
    void killHelp();
};

#endif  // HELPBUTDIALOG_H



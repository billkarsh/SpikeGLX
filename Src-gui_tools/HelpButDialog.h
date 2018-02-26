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
    const QString   filename;

public:
    HelpButDialog( const QString &filename, QWidget *parent = 0 )
        :   QDialog(parent), filename(filename) {}

protected:
    virtual bool event( QEvent *e );
};

#endif  // HELPBUTDIALOG_H



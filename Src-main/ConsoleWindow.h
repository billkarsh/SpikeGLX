#ifndef CONSOLEWINDOW_H
#define CONSOLEWINDOW_H

#include <QMainWindow>
#include <QColor>

class QTextEdit;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Main GUI.
// Main menu bar.
// Text area for Log, Error, Warning, Debug messages
// ...and user annotations.
//
class ConsoleWindow : public QMainWindow
{
    Q_OBJECT

private:
    QColor  defTextColor;
    int     nLines,
            maxLines;

public:
    ConsoleWindow( QWidget *parent = 0, Qt::WindowFlags flags = 0 );
    virtual ~ConsoleWindow();

    void setReadOnly( bool readOnly );

public slots:
    void logAppendText( const QString &txt, const QColor &clr );
    void saveToFile();

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void closeEvent( QCloseEvent *e );

private:
    QTextEdit* textEdit() const;
    void saveScreenState();
    void restoreScreenState();
};

#endif  // CONSOLEWINDOW_H



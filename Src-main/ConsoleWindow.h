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
    ConsoleWindow();
    void predelete();

    void setReadOnly( bool readOnly );

public slots:
    void logAppendText( const QString &txt, const QColor &clr );
    void saveToFile();

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void closeEvent( QCloseEvent *e );

private:
    QTextEdit* textEdit() const;
    void restoreScreenState();
    void saveScreenState() const;
};

#endif  // CONSOLEWINDOW_H



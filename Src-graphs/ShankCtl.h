#ifndef SHANKCTL_H
#define SHANKCTL_H

#include <QWidget>

namespace Ui {
class ShankWindow;
}

namespace DAQ {
struct Params;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankCtl : public QWidget
{
    Q_OBJECT

protected:
    struct UsrSettings {
        int yPix;
    };

protected:
    DAQ::Params         &p;
    Ui::ShankWindow     *scUI;
    UsrSettings         set;

public:
    ShankCtl( DAQ::Params &p, QWidget *parent = 0 );
    virtual ~ShankCtl();

    virtual void init() = 0;

    void showDialog();
    void update();
    void selChan( int ic, const QString &name );
    void layoutChanged();

signals:
    void selChanged( int ic, bool shift );
    void closed( QWidget *w );

public slots:

private slots:
    void ypixChanged( int y );
    void chanButClicked();

protected:
    void baseInit();

    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

    virtual void loadSettings() = 0;
    virtual void saveSettings() const = 0;

private:
};

#endif  // SHANKCTL_H



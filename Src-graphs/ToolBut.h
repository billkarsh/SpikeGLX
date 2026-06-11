#ifndef TOOLBUT_H
#define TOOLBUT_H

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QStyleOptionToolButton>
#include <QToolButton>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class LVEditor : public QWidget
{
    Q_OBJECT
protected:
   QString  val;
public:
    LVEditor( const QString &help, const QString &val, QWidget *parent );
    virtual ~LVEditor() {}
    virtual void setValue( const QString &val ) = 0;
signals:
    void finished( QString val );
};


class LVE_chk : public LVEditor {
    Q_OBJECT
public:
    QCheckBox   *m_checkBox;
public:
    LVE_chk(
        const QString   &name,
        const QString   &help,
        const QString   &val,
        QWidget         *parent );
    virtual void setValue( const QString &val )
    {this->val = val; m_checkBox->setChecked( val == "1" );}
private:
    void onFinished() {
        val = (m_checkBox->isChecked() ? "1" : "0");
        emit finished( val );
    }
};


class LVE_int : public LVEditor {
    Q_OBJECT
public:
    QSpinBox    *m_spinBox;
public:
    LVE_int( const QString &help, const QString &val, QWidget *parent );
    virtual void setValue( const QString &val )
    {this->val = val; m_spinBox->setValue( val.toInt() );}
private:
    void onFinished() {
        val = QString("%1").arg( m_spinBox->value() );
        emit finished( val );
    }
};


class LVE_dbl : public LVEditor {
    Q_OBJECT
public:
    QDoubleSpinBox  *m_spinBox;
public:
    LVE_dbl( const QString &help, const QString &val, QWidget *parent );
    virtual void setValue( const QString &val )
    {this->val = val; m_spinBox->setValue( val.toDouble() );}
private:
    void onFinished() {
        val = QString("%1").arg( m_spinBox->value() );
        emit finished( val );
    }
};


class LVE_cb : public LVEditor {
    Q_OBJECT
public:
    QComboBox  *m_comboBox;
public:
    LVE_cb( const QString &help, QWidget *parent );
    virtual void setValue( const QString &val )
    {this->val = val; m_comboBox->setCurrentIndex( m_comboBox->findText( val ) );}
private:
    void onFinished() {
        val = m_comboBox->currentText();
        emit finished( val );
    }
};


// label-value button
//
class LVBut : public QToolButton {
    Q_OBJECT
private:
    QMenu       *m_menu;
    QLabel      *m_labelWidget;
    QLabel      *m_valueWidget;
    LVEditor    *editor;
    QString     m_label,
                m_value;
public:
    LVBut(
        const QString   &label,
        const QString   &val,
        const QString   &butStyle,
        const QString   &lblStyle,
        const QString   &valStyle,
        LVEditor        *editor,
        QWidget         *parent = nullptr );

    void setValue( const QString &val )
    {m_value=val; editor->setValue( val ); updateButtonValue();}

    // Override sizeHint so the toolbar gives the button enough width
    QSize sizeHint() const override
    {
        // Get the size required by our custom labels
        QSize contentSize = layout()->sizeHint();

        QStyleOptionToolButton  opt;
        initStyleOption( &opt );
        opt.text = "";
        QSize butSize = style()->sizeFromContents(
                            QStyle::CT_ToolButton, &opt, contentSize, this );

        return butSize;
    }
signals:
    void valueChanged( QString val );
private:
    void updateButtonValue()    {m_valueWidget->setText( m_value );}
};

#endif  // TOOLBUT_H



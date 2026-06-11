
#include "ToolBut.h"

#include <QMenu>
#include <QVBoxLayout>
#include <QWidgetAction>


LVEditor::LVEditor( const QString &help, const QString &val, QWidget *parent )
    :   QWidget(parent), val(val)
{
    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->setContentsMargins( 8, 8, 8, 8 );
    layout->setSpacing( 6 );

    // Explanatory help text
    if( !help.isEmpty() ) {
        QLabel  *helpLabel = new QLabel( help, this );
        helpLabel->setStyleSheet( "font-size: 12px; color: #000000;" );
        helpLabel->setWordWrap( true );
        layout->addWidget( helpLabel );
    }
}


LVE_chk::LVE_chk(
    const QString   &name,
    const QString   &help,
    const QString   &val,
    QWidget         *parent )
    :   LVEditor(help, val, parent)
{
    m_checkBox = new QCheckBox( name, this );
    m_checkBox->setStyleSheet( "font-size: 24px; color: #000000;" );
    m_checkBox->setChecked( val == "1" );
    layout()->addWidget( m_checkBox );
    connect( m_checkBox, &QCheckBox::clicked, this, &LVE_chk::onFinished );
}


LVE_int::LVE_int( const QString &help, const QString &val, QWidget *parent )
    :   LVEditor(help, val, parent)
{
    m_spinBox = new QSpinBox( this );
    m_spinBox->setStyleSheet( "font-size: 24px; color: #000000;" );
    m_spinBox->setValue( val.toInt() );
    layout()->addWidget( m_spinBox );
    connect( m_spinBox, &QSpinBox::editingFinished, this, &LVE_int::onFinished );
}


LVE_dbl::LVE_dbl( const QString &help, const QString &val, QWidget *parent )
    :   LVEditor(help, val, parent)
{
    m_spinBox = new QDoubleSpinBox( this );
    m_spinBox->setStyleSheet( "font-size: 24px; color: #000000;" );
    m_spinBox->setValue( val.toDouble() );
    layout()->addWidget( m_spinBox );
    connect( m_spinBox, &QSpinBox::editingFinished, this, &LVE_dbl::onFinished );
}


LVE_cb::LVE_cb( const QString &help, QWidget *parent )
    :   LVEditor(help, "", parent)
{
    m_comboBox = new QComboBox( this );
    m_comboBox->setStyleSheet( "font-size: 20px; color: #000000;" );
    layout()->addWidget( m_comboBox );
    connect( m_comboBox, &QComboBox::currentIndexChanged, this, &LVE_cb::onFinished );
}


LVBut::LVBut(
    const QString   &label,
    const QString   &val,
    const QString   &butStyle,
    const QString   &lblStyle,
    const QString   &valStyle,
    LVEditor        *editor,
    QWidget         *parent )
    :   QToolButton(parent), editor(editor), m_label(label), m_value(val)
{
    setPopupMode( QToolButton::InstantPopup );
    setFocusPolicy( Qt::StrongFocus );

    // Create independent labels for the Label and the Value
    m_labelWidget = new QLabel( m_label, this );
    m_valueWidget = new QLabel( m_value, this );

    // Apply different colors using Qt Style Sheets
    setStyleSheet( butStyle );
    m_labelWidget->setStyleSheet( lblStyle );
    m_valueWidget->setStyleSheet( valStyle );

    // Pass mouse clicks through the labels to the underlying button
    m_labelWidget->setAttribute( Qt::WA_TransparentForMouseEvents );
    m_valueWidget->setAttribute( Qt::WA_TransparentForMouseEvents );

    // Arrange them inside the button using a layout
    QHBoxLayout *layout = new QHBoxLayout( this );
    layout->setContentsMargins( 4, 0, 0, 0 );
    layout->setSpacing( 2 );
    layout->addWidget( m_labelWidget );
    layout->addWidget( m_valueWidget );
    layout->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );

    // Keep the native button text empty so it doesn't double-draw
    setText("");

    m_menu = new QMenu( this );
    editor->setParent( m_menu );

    QWidgetAction *widgetAction = new QWidgetAction( m_menu );
    widgetAction->setDefaultWidget( editor );
    m_menu->addAction( widgetAction );
    setMenu( m_menu );

    connect( editor, &LVEditor::finished, this, [this](QString val) {
        if( m_value != val ) {
            m_value = val;
            updateButtonValue();
            emit valueChanged( val );
        }
        m_menu->close();
    });
}



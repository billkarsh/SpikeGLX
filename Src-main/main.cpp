
#include "MainApp.h"
#include "KVParams.h"
#include "FileViewerWindow.h"

#include <QProcess>
#include <QSurfaceFormat>




int main( int argc, char *argv[] )
{
    qRegisterMetaType<KeyValMap>("KeyValMap");
    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    qRegisterMetaType<QProcess::ProcessError>("QProcess::ProcessError");
    qRegisterMetaType<FileViewerWindow*>("FileViewerWindow*");

#ifdef Q_OS_LINUX
    qputenv( "QT_QPA_PLATFORM", "xcb" );
#endif

    // These lines remind me what's available to force use of opengl
    // or other rendering options in similar manner. However, these
    // have no current role.
    //
    // qputenv( "QT_RHI_BACKEND", "opengl" );
    // QCoreApplication::setAttribute( Qt::AA_UseDesktopOpenGL );

    // Can alter opengl behaviors here if needed.
    // Get default, modify it.
    //
    // QSurfaceFormat  fmt = QSurfaceFormat::defaultFormat();
    // QSurfaceFormat::setDefaultFormat( fmt );

    MainApp app( argc, argv );

#ifdef Q_OS_WIN
    app.setStyle( "fusion" );
#endif

    return app.exec();
}



#ifndef DFNAME_H
#define DFNAME_H

#include <QFileInfo>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DFName
{
public:
// ------
// Static
// ------

    static QString justExtension( const QString &name );
    static QString chopExtension( const QString &name );
    static QString chopType( const QString &name );
    static QString forceBinSuffix( const QString &name );
    static QString forceMetaSuffix( const QString &name );

    static int typeAndIP( int &ip, const QString &name, QString *error );

    static bool isFileReadable( const QFileInfo &fi, QString *error );
    static qint64 fileSize( const QFileInfo &fi, QString *error );

    static bool isValidInputFile(
        const QString   &name,
        QString         *error,
        const QString   &reqVers = "20160120" );
};

#endif  // DFNAME_H



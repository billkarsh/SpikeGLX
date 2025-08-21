
#include "KVParams.h"
#include "Util.h"

#include <QRegularExpression>




bool KVParams::parseOneLine( QString &line )
{
    line = line.trimmed();

    if( !line.length() )
        return true;

/* ------------------------------------------ */
/* Delete comments and ini-file group headers */
/* ------------------------------------------ */

// Semicolons are a popular comment character. However,
// ChanMap strings might include them, so exception is
// made for anything called 'map'.

    QRegularExpression comment("(\\[|\\s+;|^;|\\s+#|^#|\\s+//|^//).*");

    if( !line.contains( "file", Qt::CaseInsensitive )
        && !line.contains( "notes", Qt::CaseInsensitive )
        && !line.contains( "map", Qt::CaseInsensitive )
        && line.contains( comment ) ) {

        Debug()
            << "Params comment skipped: '"
            << comment.match( line ).captured(0) << "'";
        line.replace( comment, QString() );
        line = line.trimmed();

        if( !line.length() )
            return true;
    }

/* -------------------------- */
/* Capture (name)=(val) pairs */
/* -------------------------- */

    QRegularExpression re("^([^=]+)=(.*)$");    // whole line ^...$
    QRegularExpressionMatch match = re.match( line );

    if( match.hasMatch() ) {

        (*this)[match.captured(1).trimmed()] = match.captured(2).trimmed();
        return true;
    }
    else {
        Error() << "Bad params line [" << line << "].";
        return false;
    }
}


bool KVParams::fromString( const QString &s )
{
    QTextStream ts( (QString*)&s, QIODevice::ReadOnly | QIODevice::Text );
    QString     line;
    bool        ok = true;

    clear();

    while( !(line = ts.readLine()).isNull() )
        ok &= parseOneLine( line );

    return ok;
}


QString KVParams::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly | QIODevice::Text );

    for( const_iterator it = begin(); it != end(); ++it )
        ts << it.key() << "=" << it.value().toString() << "\n";

    return s;
}


bool KVParams::fromMetaFile( const QString &metaFile )
{
    QFile   f( metaFile );

    if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        QTextStream ts( &f );
        QString     s = ts.readAll();

        if( ts.status() == QTextStream::Ok ) {

            if( fromString( s ) )
                return true;
        }
        else {
            Error()
                << "Format error reading metafile '"
                << metaFile
                << "'.";
        }
    }
    else {
        Error()
            << "Couldn't open metafile for reading, error "
            << f.error()
            << " '"
            << metaFile
            << "'.";
    }

    clear();

    return false;
}


bool KVParams::toMetaFile( const QString &metaFile ) const
{
    QFile   f( metaFile );

    if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

        QTextStream ts( &f );

        ts << toString();

        if( ts.status() == QTextStream::Ok )
            return true;
        else {
            Error()
                << "Error writing metafile '"
                << metaFile
                << "'.";
        }
    }
    else {
        Error()
            << "Couldn't open metafile for writing, error "
            << f.error()
            << " '"
            << metaFile
            << "'.";
    }

    return false;
}



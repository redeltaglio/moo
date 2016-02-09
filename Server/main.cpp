#include "mooapp.h"
#include "mainwindow.h"
#include "connectionmanager.h"
#include "connection.h"
#include "listener.h"
#include "taskentry.h"
#include <QApplication>
#include <lua.hpp>
#include <QDateTime>
#include <QFile>
#include "objectmanager.h"
#include "oscreceive.h"

MainWindow		*gMainWindow = 0;

void myMessageOutput( QtMsgType type, const QMessageLogContext &context, const QString &msg )
{
	Q_UNUSED( context )

	QFile				LogFil( "moo.log" );
	const QDateTime		CurDat = QDateTime::currentDateTime();
	QString				DateTime = CurDat.toString( "dd-MMM-yyyy hh:mm:ss" );
	QString				LogMsg;

	switch( type )
	{
		default:
			LogMsg = QString( "%1 - %2" ).arg( DateTime ).arg( msg );
	}

	if( gMainWindow != 0 )
	{
		gMainWindow->log( LogMsg );
	}

	if( LogFil.open( QIODevice::Append | QIODevice::Text ) )
	{
		QTextStream		TxtOut( &LogFil );

		TxtOut << LogMsg << "\n";

		LogFil.close();
	}
}

int main( int argc, char *argv[] )
{
	QApplication	a( argc, argv );

	a.setApplicationName( "ArtMOO" );
	a.setOrganizationName( "Alex May" );
	a.setOrganizationDomain( "http://www.bigfug.com" );
	a.setApplicationVersion( "0.1" );

	MainWindow		w;

	gMainWindow = &w;

	qInstallMessageHandler( myMessageOutput );

	w.show();

	qDebug() << "ArtMOO v0.1 by Alex May - www.bigfug.com";

	QString			DataFileName = "moo.dat";
	quint16			ServerPort   = 1123;

	const QStringList	args = a.arguments();

	foreach( const QString &arg, args )
	{
		if( arg.startsWith( "-db=", Qt::CaseSensitive ) )
		{
			DataFileName = arg.mid( 4 );

			continue;
		}

		if( arg.startsWith( "-port=", Qt::CaseInsensitive ) )
		{
			ServerPort = arg.mid( 6 ).toInt();

			continue;
		}
	}

	int				 Ret = -1;

	mooApp			*App = new mooApp( DataFileName );

	if( App != 0 )
	{

	//ObjectManager	&OM = *ObjectManager::instance();

	//w.installModel( &OM );

		OSCReceive			*OSC = new OSCReceive( 2424 );

		if( OSC )
		{
			ConnectionManager	*CM = ConnectionManager::instance();

			if( CM != 0 )
			{
				Listener	*L = new Listener( 0, ServerPort, CM );

				if( L != 0 )
				{
					CM->doConnect( 0 );

					qDebug() << "ArtMOO listening for connections on port" << ServerPort;

					Ret = a.exec();

					qDebug() << "ArtMOO exiting\n";

					delete L;
				}

				delete CM;
			}

			delete OSC;
		}

		delete App;
	}

	return( Ret );
}

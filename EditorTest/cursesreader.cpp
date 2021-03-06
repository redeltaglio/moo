#include "cursesreader.h"

#include <QCoreApplication>
#include <QTextStream>

CursesReader::CursesReader()
{
	mWindow = initscr();

	raw();

	noecho();
}

void CursesReader::output( const QString &pData ) const
{
	QTextStream qout( stdout );

	qout << pData;

	refresh();
}

void CursesReader::run()
{
	int			 LastW = 0;
	int			 LastH = 0;

	while( !QThread::currentThread()->isInterruptionRequested() )
	{
		int		CurrW = getmaxx( mWindow ) - getbegx( mWindow );
		int		CurrH = getmaxy( mWindow ) - getbegy( mWindow );

		if( CurrW != LastW || CurrH != LastH )
		{
			emit resized( CurrW, CurrH );

			LastW = CurrW;
			LastH = CurrH;
		}

		const QChar	ch = wgetch( mWindow );

		emit input( ch );
	}
}

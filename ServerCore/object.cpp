#include "object.h"
#include "objectmanager.h"
#include <algorithm>
#include "verb.h"
#include "mooexception.h"

Object::Object( void )
{
	mData.mId = -1;
	mData.mParent = -1;
	mData.mOwner = -1;
	mData.mPlayer = false;
	mData.mLocation = -1;
	mData.mProgrammer = false;
	mData.mWizard = false;
	mData.mRead = false;
	mData.mWrite = false;
	mData.mFertile = false;
	mData.mRecycled = false;
}

Object::~Object( void )
{
}

void Object::verbAdd( const QString &pName, Verb &pVerb )
{
	mData.mVerbs.insert( pName, pVerb );
}

void Object::verbDelete( const QString &pName )
{
	mData.mVerbs.remove( pName );
}

Property * Object::propParent( const QString &pName )
{
	if( mData.mParent == -1 )
	{
		return( 0 );
	}

	Object      *ParentObject = ObjectManager::instance()->object( mData.mParent );

	if( ParentObject == 0 )
	{
		return( 0 );
	}

	Property        *P = ParentObject->prop( pName );

	if( P != 0 )
	{
		return( P );
	}

	return( ParentObject->propParent( pName ) );
}

Verb * Object::verbParent( const QString &pName, ObjectId pDirectObjectId, const QString &pPreposition, ObjectId pIndirectObjectId )
{
	if( mData.mParent == -1 )
	{
		return( 0 );
	}

	Object      *ParentObject = ObjectManager::instance()->object( mData.mParent );

	if( ParentObject == 0 )
	{
		return( 0 );
	}

	Verb		*V = ParentObject->verbMatch( pName, pDirectObjectId, pPreposition, pIndirectObjectId );

	if( V != 0 )
	{
		return( V );
	}

	return( ParentObject->verbParent( pName, pDirectObjectId, pPreposition, pIndirectObjectId ) );
}

Verb *Object::verbMatch( const QString &pName )
{
	for( QMap<QString,Verb>::iterator it = mData.mVerbs.begin() ; it != mData.mVerbs.end() ; it++ )
	{
		Verb				&v = it.value();

		if( pName.compare( it.key(), Qt::CaseInsensitive ) != 0 )
		{
			const QString		&a = v.aliases();

			if( a.isEmpty() )
			{
				continue;
			}

			if( !Verb::matchName( a, pName ) )
			{
				continue;
			}
		}

		return( &it.value() );
	}

	return( 0 );
}

Verb *Object::verbParent(const QString &pName)
{
	if( mData.mParent == -1 )
	{
		return( 0 );
	}

	Object      *ParentObject = ObjectManager::instance()->object( mData.mParent );

	if( ParentObject == 0 )
	{
		return( 0 );
	}

	Verb		*V = ParentObject->verbMatch( pName );

	if( V != 0 )
	{
		return( V );
	}

	return( ParentObject->verbParent( pName ) );
}

void Object::propAdd( const QString &pName, Property &pProp )
{
	mData.mProperties.insert( pName, pProp );
}

void Object::propDeleteRecurse(const QString &pName)
{
	ObjectManager	&OM = *ObjectManager::instance();

	mData.mProperties.remove( pName );

	for( ObjectId id : mData.mChildren )
	{
		Object	*O = OM.object( id );

		if( O != 0 )
		{
			O->propDeleteRecurse( pName );
		}
	}
}

void Object::propDelete( const QString &pName )
{
	QMap<QString,Property>::iterator	it = mData.mProperties.find( pName );

	if( it == mData.mProperties.end() )
	{
		return;
	}

	if( it.value().parent() != -1 )
	{
		mData.mProperties.remove( pName );
	}
	else
	{
		propDeleteRecurse( pName );
	}
}

void Object::propClear( const QString &pName )
{
	QMap<QString,Property>::iterator	it = mData.mProperties.find( pName );

	if( it == mData.mProperties.end() )
	{
		return;
	}

	if( it.value().parent() != OBJECT_NONE )
	{
		mData.mProperties.remove( pName );
	}
}

void Object::propSet( const QString &pName, const QVariant &pValue )
{
	Property			*P;

	if( ( P = prop( pName ) ) != 0 )
	{
		if( P->value().type() != pValue.type() )
		{
			return;
		}

		P->setValue( pValue );
	}
	else if( ( P = propParent( pName ) ) != 0 )
	{
		if( P->value().type() != pValue.type() )
		{
			return;
		}

		Property		C = *P;

		C.setParent( parent() );

		if( P->change() )
		{
			C.setOwner( id() );
		}

		C.setValue( pValue );

		mData.mProperties.insert( pName, C );
	}
}

bool Object::propFind( const QString &pName, Property **pProp, Object **pObject )
{
	return( propFindRecurse( pName, pProp, pObject ) );
}

bool Object::propFindRecurse( const QString &pName, Property **pProp, Object **pObject )
{
	if( ( *pProp = prop( pName ) ) != 0 )
	{
		*pObject = this;

		return( true );
	}

	Object		*Parent = ObjectManager::o( mData.mParent );

	if( Parent != 0 )
	{
		return( Parent->propFindRecurse( pName, pProp, pObject ) );
	}

	return( false );
}

void Object::ancestors( QList<ObjectId> &pList )
{
	ObjectManager	&OM = *ObjectManager::instance();
	Object			*PO = OM.object( mData.mParent );

	while( PO != 0 )
	{
		pList.push_back( PO->id() );

		PO = OM.object( PO->parent() );
	}
}

void Object::descendants( QList<ObjectId> &pList )
{
	ObjectManager	&OM = *ObjectManager::instance();

	pList.append( mData.mChildren );

	for( ObjectId id : mData.mChildren )
	{
		Object	*O = OM.object( id );

		if( O != 0 )
		{
			O->descendants( pList );
		}
	}
}

void Object::move( Object *pWhere )
{
	Object		*From = ( mData.mLocation == -1 ? 0 : ObjectManager::instance()->object( mData.mLocation ) );

	if( From != 0 )
	{
		int	Count = From->mData.mContents.removeAll( mData.mId );

		Q_ASSERT( Count == 1 );
	}

	if( pWhere != 0 )
	{
		Q_ASSERT( pWhere->mData.mContents.removeAll( mData.mId ) == 0 );

		pWhere->mData.mContents.push_back( mData.mId );
	}

	mData.mLocation = ( pWhere == 0 ? -1 : pWhere->id() );
}

void Object::setParent( ObjectId pNewParentId )
{
	if( mData.mParent == pNewParentId )
	{
		return;
	}

	if( mData.mParent != -1 )
	{
		Object		*O = ObjectManager::instance()->object( mData.mParent );

		Q_ASSERT( O != 0 );

		if( O != 0 )
		{
			int c = O->mData.mChildren.removeAll( mData.mId );

			Q_ASSERT( c == 1 );
		}
	}
	else
	{
		ObjectManager::instance()->topRem( this );
	}

	if( pNewParentId != -1 )
	{
		Object		*O = ObjectManager::instance()->object( pNewParentId );

		Q_ASSERT( O != 0 );

		if( O != 0 )
		{
			Q_ASSERT( O->mData.mChildren.removeAll( mData.mId ) == 0 );

			O->mData.mChildren.push_back( mData.mId );
		}
	}
	else
	{
		ObjectManager::instance()->topAdd( this );
	}

	mData.mParent = pNewParentId;
}

void Object::propNames( QStringList &pList )
{
	pList = mData.mProperties.keys();
}

Property *Object::prop( const QString &pName )
{
	QMap<QString,Property>::iterator	it = mData.mProperties.find( pName );

	if( it == mData.mProperties.end() )
	{
		return( 0 );
	}

	return( &it.value() );
}

quint16 Object::permissions( void ) const
{
	quint16			P = 0;

	if( mData.mRead    ) P |= READ;
	if( mData.mWrite   ) P |= WRITE;
	if( mData.mFertile ) P |= FERTILE;

	return( P );
}

void Object::setPermissions( quint16 pPerms )
{
	mData.mRead    = ( pPerms & READ );
	mData.mWrite   = ( pPerms & WRITE );
	mData.mFertile = ( pPerms & FERTILE );
}

bool Object::matchName( const QString &pName )
{
	if( mData.mName.startsWith( pName, Qt::CaseInsensitive ) )
	{
		return( true );
	}

	Property		*p = prop( "aliases" );

	if( p == 0 )
	{
		return( false );
	}

	if( p->value().type() == QVariant::String )
	{
		return( p->value().toString().startsWith( pName, Qt::CaseInsensitive ) );
	}

	if( p->value().type() == QVariant::Map )
	{
		for( const QVariant &V : p->value().toMap().values() )
		{
			if( V.type() == QVariant::String && V.toString().startsWith( pName, Qt::CaseInsensitive ) )
			{
				return( true );
			}
		}
	}

	return( false );
}

Verb * Object::verbMatch( const QString &pName, ObjectId DirectObjectId, const QString &pPreposition, ObjectId IndirectObjectId )
{
	for( QMap<QString,Verb>::iterator it = mData.mVerbs.begin() ; it != mData.mVerbs.end() ; it++ )
	{
		Verb				&v = it.value();

		if( !v.matchArgs( id(), DirectObjectId, pPreposition, IndirectObjectId ) )
		{
			continue;
		}

		if( pName.compare( it.key(), Qt::CaseInsensitive ) != 0 )
		{
			const QString		&a = v.aliases();

			if( a.isEmpty() )
			{
				continue;
			}

			if( !Verb::matchName( a, pName ) )
			{
				continue;
			}
		}

		return( &it.value() );
	}

	return( 0 );
}

bool Object::verbFind( const QString &pName, Verb **pVerb, Object **pObject, ObjectId pDirectObjectId, const QString &pPreposition, ObjectId pIndirectObjectId )
{
	return( verbFindRecurse( pName, pVerb, pObject, pDirectObjectId, pPreposition, pIndirectObjectId ) );
}

bool Object::verbFind( const QString &pName, Verb **pVerb, Object **pObject )
{
	return( verbFindRecurse( pName, pVerb, pObject ) );
}

Verb *Object::verb( const QString &pName )
{
	for( QMap<QString,Verb>::iterator it = mData.mVerbs.begin() ; it != mData.mVerbs.end() ; it++ )
	{
		if( pName.compare( it.key(), Qt::CaseInsensitive ) != 0 )
		{
			continue;
		}

		return( &it.value() );
	}

	return( 0 );
}

bool Object::verbFindRecurse( const QString &pName, Verb **pVerb, Object **pObject, ObjectId pDirectObjectId, const QString &pPreposition, ObjectId pIndirectObjectId )
{
	if( ( *pVerb = verbMatch( pName, pDirectObjectId, pPreposition, pIndirectObjectId ) ) != 0 )
	{
		*pObject = this;

		return( true );
	}

	Object	*P = ObjectManager::o( parent() );

	if( P == 0 )
	{
		return( false );
	}

	if( pDirectObjectId == id() )
	{
		pDirectObjectId = P->id();
	}

	if( pIndirectObjectId == id() )
	{
		pIndirectObjectId = P->id();
	}

	return( P->verbFindRecurse( pName, pVerb, pObject, pDirectObjectId, pPreposition, pIndirectObjectId ) );
}

bool Object::verbFindRecurse( const QString &pName, Verb **pVerb, Object **pObject )
{
	if( ( *pVerb = verbMatch( pName ) ) != 0 )
	{
		*pObject = this;

		return( true );
	}

	Object	*P = ObjectManager::o( parent() );

	if( P == 0 )
	{
		return( false );
	}

	return( P->verbFindRecurse( pName, pVerb, pObject ) );
}

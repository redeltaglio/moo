#include "lua_verb.h"
#include "lua_moo.h"
#include "verb.h"
#include "lua_utilities.h"
#include "mooexception.h"
#include "lua_task.h"
#include "lua_object.h"
#include "object.h"
#include "objectmanager.h"
#include "connectionmanager.h"
#include "connection.h"
#include "inputsinkprogram.h"
#include "inputsinkeditor.h"

const char	*lua_verb::mLuaName = "moo.verb";

LuaMap		lua_verb::mLuaMap;

const luaL_Reg lua_verb::mLuaStatic[] =
{
	{ 0, 0 }
};

const luaL_Reg lua_verb::mLuaInstance[] =
{
	{ "__index", lua_verb::luaGet },
	{ "__newindex", lua_verb::luaSet },
	{ "__gc", lua_verb::luaGC },
	{ 0, 0 }
};

const luaL_Reg lua_verb::mLuaInstanceFunctions[] =
{
	{ "aliasadd", lua_verb::luaAliasAdd },
	{ "aliasrem", lua_verb::luaAliasRem },
	{ "dump", lua_verb::luaDump },
	{ "program", lua_verb::luaProgram },
	{ "edit", lua_verb::luaEdit },
	{ 0, 0 }
};

void lua_verb::initialise()
{
//	lua_moo::addFunctions( mLuaStatic );

	// As we're overriding __index, build a static QMap of commands
	// pointing to their relevant functions (hopefully pretty fast)

	for( const luaL_Reg *FP = mLuaInstanceFunctions ; FP->name != 0 ; FP++ )
	{
		mLuaMap[ FP->name ] = FP->func;
	}
}

void lua_verb::luaRegisterState( lua_State *L )
{
	// Create the moo.object metatables that is used for all objects

	luaL_newmetatable( L, mLuaName );

	lua_pushstring( L, "__index" );
	lua_pushvalue( L, -2 );  /* pushes the metatable */
	lua_settable( L, -3 );  /* metatable.__index = metatable */

#if LUA_VERSION_NUM == 501
	luaL_openlib( L, NULL, lua_verb::mLuaInstance, 0 );
#else
#error LUA 5.1
#endif

	lua_pop( L, 1 );

	Q_ASSERT( lua_gettop( L ) == 0 );
}

void lua_verb::lua_pushverb( lua_State *L, Verb *V )
{
	luaVerb			*H = (luaVerb *)lua_newuserdata( L, sizeof( luaVerb ) );

	if( H == 0 )
	{
		luaL_error( L, "out of memory" );

		return;
	}

//	H->mName     = new QString( pName );
//	H->mObjectId = pObjectId;
	H->mVerb     = V;

	luaL_getmetatable( L, mLuaName );
	lua_setmetatable( L, -2 );
}

lua_verb::luaVerb *lua_verb::arg( lua_State *L, int pIndex )
{
	luaVerb *H = (luaVerb *)luaL_checkudata( L, pIndex, mLuaName );

	luaL_argcheck( L, H != NULL, pIndex, "`verb' expected" );

	return( H );
}

int lua_verb::luaGC( lua_State *L )
{
	luaVerb		*V = arg( L );

	if( V->mVerb->object() == OBJECT_NONE )
	{
		delete( V->mVerb );
	}

//	V->mObjectId = OBJECT_NONE;
	V->mVerb     = 0;

//	if( V->mName != 0 )
//	{
//		delete( V->mName );
//	}

	return( 0 );
}

int lua_verb::luaGet( lua_State *L )
{
	bool		LuaErr = false;

	try
	{
		const Task			&T = lua_task::luaGetTask( L )->task();
		luaVerb				*LV = arg( L );
		Verb				*V = LV->mVerb;
		const char			*s = luaL_checkstring( L, 2 );
		Object				*O = ObjectManager::o( V->object() );
		Object				*Player = ObjectManager::instance()->object( T.player() );
		const bool			 isOwner  = ( Player != 0 && O != 0 ? Player->id() == O->owner() : false );
		const bool			 isWizard = ( Player != 0 ? Player->wizard() : false );

		if( V == 0 )
		{
			throw( mooException( E_TYPE, "invalid object" ) );
		}

		// Look for function in mLuaMap

		lua_CFunction	 F;

		if( ( F = mLuaMap.value( s, 0 ) ) != 0 )
		{
			lua_pushcfunction( L, F );

			return( 1 );
		}

		if( strcmp( s, "name" ) == 0 )
		{
			lua_pushstring( L, V->name().toLatin1() );

			return( 1 );
		}

		if( strcmp( s, "aliases" ) == 0 )
		{
			const QStringList	AliasList = V->aliases();

			lua_newtable( L );

			for( int i = 0 ; i < AliasList.size() ; i++ )
			{
				lua_pushstring( L, AliasList[ i ].toLatin1() );

				lua_rawseti( L, -2, i + 1 );
			}

			return( 1 );
		}

		if( strcmp( s, "owner" ) == 0 )
		{
			lua_object::lua_pushobjectid( L, V->owner() );

			return( 1 );
		}

		if( strcmp( s, "r" ) == 0 )
		{
			lua_pushboolean( L, V->read() );

			return( 1 );
		}

		if( strcmp( s, "w" ) == 0 )
		{
			lua_pushboolean( L, V->write() );

			return( 1 );
		}

		if( strcmp( s, "x" ) == 0 )
		{
			lua_pushboolean( L, V->execute() );

			return( 1 );
		}

		if( strcmp( s, "script" ) == 0 )
		{
			if( !isWizard && !isOwner && !V->read() )
			{
				throw( mooException( E_TYPE, "not allowed to read script" ) );
			}

			lua_pushstring( L, V->script().toLatin1() );

			return( 1 );
		}

		if( strcmp( s, "dobj" ) == 0 )
		{
			lua_pushstring( L, Verb::argobj_name( V->directObject() ) );

			return( 1 );
		}

		if( strcmp( s, "iobj" ) == 0 )
		{
			lua_pushstring( L, Verb::argobj_name( V->indirectObject() ) );

			return( 1 );
		}

		if( strcmp( s, "prep" ) == 0 )
		{
			QString		Prep = V->preposition();

			if( Prep.isEmpty() )
			{
				lua_pushstring( L, Verb::argobj_name( V->prepositionType() ) );
			}
			else
			{
				lua_pushfstring( L, "%s", Prep.toLatin1().constData() );
			}

			return( 1 );
		}

		// Nothing found

		throw( mooException( E_PROPNF, QString( "property '%1' is not defined" ).arg( QString( s ) ) ) );
	}
	catch( mooException e )
	{
		e.lua_pushexception( L );

		LuaErr = true;
	}
	catch( ... )
	{

	}

	return( LuaErr ? lua_error( L ) : 0 );
}

int lua_verb::luaSet( lua_State *L )
{
	bool		LuaErr = false;

	try
	{
		lua_task			*Command = lua_task::luaGetTask( L );
		const Task			&T = Command->task();
		Object				*Player = ObjectManager::o( T.programmer() );

		if( Player == 0 )
		{
			throw mooException( E_PERM, "programmer is invalid" );
		}

		luaVerb				*LV = arg( L );

		if( LV == 0 )
		{
			throw( mooException( E_PERM, "verb is invalid" ) );
		}

		Verb				*V = LV->mVerb;
		Object				*O = ObjectManager::o( V->object() );
		const char			*N = luaL_checkstring( L, 2 );
		const bool			 isOwner  = ( Player != 0 && O != 0 ? Player->id() == O->owner() : false );
		const bool			 isWizard = ( Player != 0 ? Player->wizard() : false );

		if( strcmp( N, "name" ) == 0 )
		{
			throw( mooException( E_PERM, "can't set verb name" ) );
		}

		if( strcmp( N, "owner" ) == 0 )
		{
			if( !isWizard && !isOwner )
			{
				throw( mooException( E_PERM, "player is not owner or wizard" ) );
			}

			Object				*D = lua_object::argObj( L, 3 );

			V->setOwner( D->id() );

			return( 0 );
		}

		if( strcmp( N, "r" ) == 0 )
		{
			if( !isWizard && !isOwner )
			{
				throw( mooException( E_PERM, "player is not owner or wizard" ) );
			}

			bool		v = lua_toboolean( L, 3 );

			V->setRead( v );

			return( 0 );
		}

		if( strcmp( N, "w" ) == 0 )
		{
			if( !isWizard && !isOwner )
			{
				throw( mooException( E_PERM, "player is not owner or wizard" ) );
			}

			bool		v = lua_toboolean( L, 3 );

			V->setWrite( v );

			return( 0 );
		}

		if( strcmp( N, "x" ) == 0 )
		{
			if( !isWizard && !isOwner )
			{
				throw( mooException( E_PERM, "player is not owner or wizard" ) );
			}

			bool		v = lua_toboolean( L, 3 );

			V->setExecute( v );

			return( 0 );
		}

		if( strcmp( N, "script" ) == 0 )
		{
			if( !isWizard && !isOwner )
			{
				throw( mooException( E_PERM, "player is not owner or wizard" ) );
			}

			V->setScript( lua_tostring( L, 3 ) );

			return( 0 );
		}

		if( strcmp( N, "dobj" ) == 0 )
		{
			if( !isWizard && !isOwner )
			{
				throw( mooException( E_PERM, "player is not owner or wizard" ) );
			}

			const QString	Direct = luaL_checkstring( L, 3 );

			if( Direct == "this" )
			{
				V->setDirectObjectArgument( THIS );
			}
			else if( Direct == "any" )
			{
				V->setDirectObjectArgument( ANY );
			}
			else if( Direct == "none" )
			{
				V->setDirectObjectArgument( NONE );
			}
			else
			{
				throw( mooException( E_PROPNF, QString( "unknown direct object argument" ).arg( Direct ) ) );
			}

			return( 0 );
		}

		if( strcmp( N, "iobj" ) == 0 )
		{
			if( !isWizard && !isOwner )
			{
				throw( mooException( E_PERM, "player is not owner or wizard" ) );
			}

			const QString	Direct = luaL_checkstring( L, 3 );

			if( Direct == "this" )
			{
				V->setIndirectObjectArgument( THIS );
			}
			else if( Direct == "any" )
			{
				V->setIndirectObjectArgument( ANY );
			}
			else if( Direct == "none" )
			{
				V->setIndirectObjectArgument( NONE );
			}
			else
			{
				throw( mooException( E_PROPNF, QString( "unknown indirect object argument" ).arg( Direct ) ) );
			}

			return( 0 );
		}

		if( strcmp( N, "prep" ) == 0 )
		{
			if( !isWizard && !isOwner )
			{
				throw( mooException( E_PERM, "player is not owner or wizard" ) );
			}

			const char		*Prep = luaL_checkstring( L, 3 );
			bool			 PrepTypeOK;
			ArgObj			 PrepType = Verb::argobj_from( Prep, &PrepTypeOK );

			if( PrepTypeOK )
			{
				if( PrepType == ANY )
				{
					V->setPrepositionArgument( ANY );
				}
				else if( PrepType == NONE )
				{
					V->setPrepositionArgument( NONE );
				}
				else
				{
					throw( mooException( E_INVARG, QString( "bad prep type: %1" ).arg( Prep ) ) );
				}
				//V->setPrepositionArgument( PrepType );
			}
			else
			{
				V->setPrepositionArgument( Prep );
			}

			return( 0 );
		}

		throw( mooException( E_PROPNF, QString( "property '%1' is not defined" ).arg( N ) ) );
	}
	catch( mooException e )
	{
		e.lua_pushexception( L );

		LuaErr = true;
	}
	catch( ... )
	{

	}

	return( LuaErr ? lua_error( L ) : 0 );
}

int lua_verb::luaAliasAdd( lua_State *L )
{
	bool		LuaErr = false;

	luaL_checkstring( L, 2 );

	try
	{
		const Task			&T = lua_task::luaGetTask( L )->task();
		Object				*Player = ObjectManager::o( T.programmer() );

		if( Player == 0 )
		{
			throw mooException( E_PERM, "programmer is invalid" );
		}

		luaVerb				*LV = arg( L );

		if( LV == 0 )
		{
			throw( mooException( E_PERM, "verb is invalid" ) );
		}

		Verb				*V = LV->mVerb;
		const char			*N = luaL_checkstring( L, 2 );

		if( Player->id() != V->owner() && !Player->wizard() )
		{
			throw mooException( E_PERM, "programmer has no access" );
		}

		V->addAlias( QString( N ) );
	}
	catch( mooException e )
	{
		e.lua_pushexception( L );

		LuaErr = true;
	}
	catch( ... )
	{

	}

	return( LuaErr ? lua_error( L ) : 0 );
}

int lua_verb::luaAliasRem(lua_State *L)
{
	bool		LuaErr = false;

	luaL_checkstring( L, 2 );

	try
	{
		const Task			&T = lua_task::luaGetTask( L )->task();
		Object				*Player = ObjectManager::o( T.programmer() );

		if( Player == 0 )
		{
			throw mooException( E_PERM, "programmer is invalid" );
		}

		luaVerb				*LV = arg( L );

		if( LV == 0 )
		{
			throw( mooException( E_PERM, "verb is invalid" ) );
		}

		Verb				*V = LV->mVerb;
		const char			*N = luaL_checkstring( L, 2 );

		if( Player->id() != V->owner() && !Player->wizard() )
		{
			throw mooException( E_PERM, "programmer has no access" );
		}

		V->remAlias( QString( N ) );
	}
	catch( mooException e )
	{
		e.lua_pushexception( L );

		LuaErr = true;
	}
	catch( ... )
	{

	}

	return( LuaErr ? lua_error( L ) : 0 );
}

int lua_verb::luaDump( lua_State *L )
{
	bool		LuaErr = false;

	try
	{
		const Task			&T = lua_task::luaGetTask( L )->task();
		luaVerb				*LV = arg( L );
		Verb				*V = LV->mVerb;
		Object				*O = ObjectManager::o( V->object() );
		Object				*Player = ObjectManager::instance()->object( T.player() );
		const bool			 isOwner  = ( Player != 0 && O != 0 ? Player->id() == O->owner() : false );
		const bool			 isWizard = ( Player != 0 ? Player->wizard() : false );
		Connection			*C = ConnectionManager::instance()->connection( lua_task::luaGetTask( L )->connectionid() );

		if( V == 0 )
		{
			throw( mooException( E_TYPE, "invalid object" ) );
		}

		if( !isWizard && !isOwner && !V->read() )
		{
			throw( mooException( E_TYPE, "not allowed to read script" ) );
		}

		QStringList		Program = V->script().split( "\n" );

		for( QStringList::iterator it = Program.begin() ; it != Program.end() ; it++ )
		{
			C->notify( *it );
		}

		C->notify( "." );
	}
	catch( mooException e )
	{
		e.lua_pushexception( L );

		LuaErr = true;
	}
	catch( ... )
	{

	}

	return( LuaErr ? lua_error( L ) : 0 );
}

int lua_verb::luaProgram( lua_State *L )
{
	bool		LuaErr = false;

	try
	{
		const Task			&T = lua_task::luaGetTask( L )->task();
		luaVerb				*LV = arg( L );
		Verb				*V = LV->mVerb;
		Object				*O = ObjectManager::o( V->object() );
		Object				*Player = ObjectManager::instance()->object( T.player() );
		const bool			 isOwner  = ( Player != 0 && O != 0 ? Player->id() == O->owner() : false );
		const bool			 isWizard = ( Player != 0 ? Player->wizard() : false );
		Connection			*C = ConnectionManager::instance()->connection( lua_task::luaGetTask( L )->connectionid() );

		if( V == 0 )
		{
			throw( mooException( E_TYPE, "invalid object" ) );
		}

		if( !isWizard && !isOwner && !V->read() )
		{
			throw( mooException( E_TYPE, "not allowed to read script" ) );
		}

		InputSinkProgram	*IS = new InputSinkProgram( C, O->id(), V->name() );

		if( IS == 0 )
		{
			return( 0 );
		}

		C->pushInputSink( IS );
	}
	catch( mooException e )
	{
		e.lua_pushexception( L );

		LuaErr = true;
	}
	catch( ... )
	{

	}

	return( LuaErr ? lua_error( L ) : 0 );
}

int lua_verb::luaEdit( lua_State *L )
{
	bool		LuaErr = false;

	try
	{
		const Task			&T = lua_task::luaGetTask( L )->task();
		luaVerb				*LV = arg( L );
		Verb				*V = LV->mVerb;
		Object				*O = ObjectManager::o( V->object() );
		Object				*Player = ObjectManager::instance()->object( T.player() );
		const bool			 isOwner  = ( Player != 0 && O != 0 ? Player->id() == O->owner() : false );
		const bool			 isWizard = ( Player != 0 ? Player->wizard() : false );
		Connection			*C = ConnectionManager::instance()->connection( lua_task::luaGetTask( L )->connectionid() );

		if( !V )
		{
			throw( mooException( E_TYPE, "invalid object" ) );
		}

		if( !isWizard && !isOwner && !V->read() )
		{
			throw( mooException( E_NACC, "not allowed to read script" ) );
		}

		if( !C->supportsLineMode() )
		{
			throw( mooException( E_INVARG, "terminal doesn't support linemode" ) );
		}

		QStringList		Program = V->script().split( "\n" );

		InputSinkEditor	*IS = new InputSinkEditor( C, O->id(), V->name(), Program );

		if( !IS )
		{
			return( 0 );
		}

		C->pushInputSink( IS );
	}
	catch( mooException e )
	{
		e.lua_pushexception( L );

		LuaErr = true;
	}
	catch( ... )
	{

	}

	return( LuaErr ? lua_error( L ) : 0 );
}

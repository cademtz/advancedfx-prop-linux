//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//
#if !defined( SOURCESDK_CS2_IGAMEEVENTS_H )
#define SOURCESDK_CS2_IGAMEEVENTS_H
#ifdef _WIN32
#pragma once
#endif

#include "interfaces/interfaces.h"
//#include "tier1/bitbuf.h"

#ifndef NULL
#define NULL 0
#endif

namespace SOURCESDK {
namespace CS2 {

class CMsgSource1LegacyGameEvent;
class CPlayerSlot;
class CBasePlayer;
class CEntityIndex;
class CEntityHandle;
class CBaseEntity;
//-----------------------------------------------------------------------------
// Purpose: Engine interface into global game event management
//-----------------------------------------------------------------------------

/* 

The GameEventManager keeps track and fires of all global game events. Game events 
are fired by game.dll for events like player death or team wins. Each event has a
unique name and comes with a KeyValue structure providing informations about this
event. Some events are generated also by the engine.

Events are networked to connected clients and invoked there to. Therefore you
have to specify all data fields and there data types in an public resource
file which is parsed by server and broadcasted to it's clients. A typical game 
event is defined like this:

	"game_start"				// a new game starts
	{
		"roundslimit"	"long"		// max round
		"timelimit"		"long"		// time limit
		"fraglimit"		"long"		// frag limit
		"objective"		"string"	// round objective
	}

All events must have unique names (case sensitive) and may have a list
of data fields. each data field must specify a data type, so the engine
knows how to serialize/unserialize that event for network transmission.
Valid data types are string, float, long, short, byte & bool. If a 
data field should not be broadcasted to clients, use the type "local".
*/


#define MAX_EVENT_NAME_LENGTH	32		// max game event name length
#define MAX_EVENT_BITS			9		// max bits needed for an event index
#define MAX_EVENT_NUMBER		(1<<MAX_EVENT_BITS)		// max number of events allowed
#define MAX_EVENT_BYTES			1024	// max size in bytes for a serialized event

class KeyValues;
class CGameEvent;

/*
SOURCESDK_abstract_class IToolGameEventAPI
{
	virtual void unk001( void * ) = 0;
};*/

SOURCESDK_abstract_class IGameEvent
{
public:
	virtual ~IGameEvent() {};
	virtual const char *GetName() const = 0;	// get event name
	virtual int GetID() const = 0;

	virtual bool  IsReliable() const = 0; // if event handled reliable
	virtual bool  IsLocal() const = 0; // if event is never networked
	virtual bool  IsEmpty(const char *keyName = NULL) = 0; // check if data field exists
	
	// Data access
	virtual bool  GetBool( const char *keyName = NULL, bool defaultValue = false ) = 0;
	virtual int   GetInt( const char *keyName = NULL, int defaultValue = 0 ) = 0;
	virtual uint64 GetUint64( const char *keyName = NULL, uint64 defaultValue = 0 ) = 0;
	virtual float GetFloat( const char *keyName = NULL, float defaultValue = 0.0f ) = 0;
	virtual const char *GetString( const char *keyName = NULL, const char *defaultValue = "" ) = 0;
	virtual void *GetPtr( const char *keyName = NULL, void *defaultValue = NULL ) = 0;

	/* These function prototypes and names are very speculative and might be incorrect */
	virtual CEntityHandle GetEHandle( const char *keyName, CEntityHandle defaultValue ) = 0;
	virtual CEntityHandle GetStrictEHandle( const char *keyName, CEntityHandle defaultValue ) = 0;
	virtual CEntityHandle GetEHandle2( const char *keyName, CEntityHandle defaultValue ) = 0;

	virtual CPlayerSlot *GetPlayerSlot( const char *keyName = NULL ) = 0;
	virtual CBasePlayer *GetPlayer( const char *keyName = NULL ) = 0;

	virtual void *GetPlayerController( const char *keyName = NULL ) = 0;
	virtual CEntityHandle GetPlayerControllerEHandle( const char *keyName = NULL ) = 0;
	virtual CEntityHandle GetPlayerControllerEHandle2( const char *keyName = NULL ) = 0;
	/* ============================================================ */

	virtual void SetBool( const char *keyName, bool value ) = 0;
	virtual void SetInt( const char *keyName, int value ) = 0;
	virtual void SetUint64( const char *keyName, uint64 value ) = 0;
	virtual void SetFloat( const char *keyName, float value ) = 0;
	virtual void SetString( const char *keyName, const char *value ) = 0;
	virtual void SetPtr( const char *keyName, void *value ) = 0;

	/* These function prototypes and names are very speculative and might be incorrect */
	virtual void SetEHandleStrict( const char *keyName, CEntityHandle handle ) = 0;
	virtual void SetEHandle( const char *keyName, CEntityHandle handle ) = 0;

	// Also sets the _pawn key
	virtual void SetPlayerSlot( const char *keyName, CPlayerSlot value ) = 0;
	virtual void SetPlayer( const char *keyName, CBasePlayer *value ) = 0;
	/* ============================================================ */

	virtual void unk002() = 0;

	virtual bool HasKey( const char *keyName ) = 0;

	// Something script vm related
	virtual void unk001() = 0;
	
	virtual struct KeyValues3 * GetDataKeys() const = 0;
};

class CGameEventDescriptor
{
public:
	CGameEventDescriptor()
	{
		name_index = -1;
		eventid = -1;
		keys = NULL;
		local = false;
		reliable = true;
	}

public:
	int	name_index;
	int	eventid;
	KeyValues *keys;
	bool local;
	bool reliable;
	// ... more stuf we don't care about.
};

class CGameEventDescriptor;

class CGameEvent : public IGameEvent
{
public:
	CGameEventDescriptor	*m_pDescriptor;
	KeyValues				*m_pDataKeys;
};


/*
SOURCESDK_abstract_class IGameEventListener2
{
public:
	virtual	~IGameEventListener2( void ) {};

	// FireEvent is called by EventManager if event just occured
	// KeyValue memory will be freed by manager if not needed anymore
	virtual void FireGameEvent( IGameEvent *event ) = 0;
};

SOURCESDK_abstract_class IGameEventManager2 : public IBaseInterface, public IToolGameEventAPI
{
public:
	virtual	~IGameEventManager2( void ) {};

	// load game event descriptions from a file eg "resource\gameevents.res"
	virtual int LoadEventsFromFile( const char *filename, bool bSearchAll ) = 0;

	// removes all and anything
	virtual void  Reset() = 0;	

	// adds a listener for a particular event
	virtual bool AddListener( IGameEventListener2 *listener, const char *name, bool bServerSide ) = 0;

	// returns true if this listener is listens to given event
	virtual bool FindListener( IGameEventListener2 *listener, const char *name ) = 0;

	// removes a listener 
	virtual void RemoveListener( IGameEventListener2 *listener) = 0;

	// create an event by name, but doesn't fire it. returns NULL is event is not
	// known or no listener is registered for it. bForce forces the creation even if no listener is active
	virtual IGameEvent *CreateEvent( const char *name, bool bForce = false, int *pCookie = NULL ) = 0;

	// fires a server event created earlier, if bDontBroadcast is set, event is not send to clients
	virtual bool FireEvent( IGameEvent *event, bool bDontBroadcast = false ) = 0;

	// fires an event for the local client only, should be used only by client code
	virtual bool FireEventClientSide( IGameEvent *event ) = 0;

	// create a new copy of this event, must be free later
	virtual IGameEvent *DuplicateEvent( IGameEvent *event ) = 0;

	// if an event was created but not fired for some reason, it has to bee freed, same UnserializeEvent
	virtual void FreeEvent( IGameEvent *event ) = 0;

	// write/read event to/from bitbuffer
	virtual bool SerializeEvent( IGameEvent *event, CMsgSource1LegacyGameEvent *ev ) = 0;
	virtual IGameEvent *UnserializeEvent( const CMsgSource1LegacyGameEvent &ev ) = 0; // create new KeyValues, must be deleted
	
	virtual int LookupEventId( const char *name ) = 0;
	
	virtual void PrintEventToString( IGameEvent *event, CUtlString &out ) = 0;

	virtual bool HasEventDescriptor( const char *name ) = 0;
};
*/
} // namespace CS2 {
} // namespace SOURCESDK {

#endif // SOURCESDK_CS2_IGAMEEVENTS_H

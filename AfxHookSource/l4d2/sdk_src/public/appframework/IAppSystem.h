//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: An application framework 
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//

#ifndef SOURCESDK_L4D2_IAPPSYSTEM_H
#define SOURCESDK_L4D2_IAPPSYSTEM_H

#ifdef COMPILER_MSVC
#pragma once
#endif

//#include <l4d2/public/tier1/interface.h>
#include <l4d2/sdk_src/public/interfaces/interfaces.h>

#include <SourceSdkShared.h>

namespace SOURCESDK {
namespace L4D2 {


//-----------------------------------------------------------------------------
// Specifies a module + interface name for initialization
//-----------------------------------------------------------------------------
struct AppSystemInfo_t
{
	const char *m_pModuleName;
	const char *m_pInterfaceName;
};


//-----------------------------------------------------------------------------
// Client systems are singleton objects in the client codebase responsible for
// various tasks
// The order in which the client systems appear in this list are the
// order in which they are initialized and updated. They are shut down in
// reverse order from which they are initialized.
//-----------------------------------------------------------------------------
enum InitReturnVal_t
{
	INIT_FAILED = 0,
	INIT_OK,

	INIT_LAST_VAL,
};

enum AppSystemTier_t
{
	APP_SYSTEM_TIER0 = 0,
	APP_SYSTEM_TIER1,
	APP_SYSTEM_TIER2,
	APP_SYSTEM_TIER3,

	APP_SYSTEM_TIER_OTHER,
};


SOURCESDK_abstract_class IAppSystem
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory ) = 0;
	virtual void Disconnect() = 0;

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void *QueryInterface( const char *pInterfaceName ) = 0;

	// Init, shutdown
	virtual InitReturnVal_t Init() = 0;
	virtual void Shutdown() = 0;
};


//-----------------------------------------------------------------------------
// Helper empty implementation of an IAppSystem
//-----------------------------------------------------------------------------
template< class IInterface > 
class CBaseAppSystem : public IInterface
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory ) { return true; }
	virtual void Disconnect() {}

	// Here's where systems can access other interfaces implemented by this object
	// Returns NULL if it doesn't implement the requested interface
	virtual void *QueryInterface( const char *pInterfaceName ) { return NULL; }

	// Init, shutdown
	virtual InitReturnVal_t Init() { return INIT_OK; }
	virtual void Shutdown() {}

	virtual const AppSystemInfo_t* GetDependencies() { return NULL; }
	virtual AppSystemTier_t GetTier() { return APP_SYSTEM_TIER_OTHER; }

	virtual void Reconnect( CreateInterfaceFn factory, const char *pInterfaceName )
	{
		ReconnectInterface( factory, pInterfaceName );
	}
};


//-----------------------------------------------------------------------------
// Helper implementation of an IAppSystem for tier0
//-----------------------------------------------------------------------------
template< class IInterface > 
class CTier0AppSystem : public CBaseAppSystem< IInterface >
{
};

} // namespace SOURCESDK {
} // namespace L4D2 {


#endif // SOURCESDK_L4D2_IAPPSYSTEM_H


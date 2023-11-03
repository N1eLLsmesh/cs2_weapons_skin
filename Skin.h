#ifndef _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
#define _INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_

#include <ISmmPlugin.h>
#include <sh_vector.h>
#include <iserver.h>
#include <entity2/entitysystem.h>
#include "igameevents.h"
#include "vector.h"
#include <deque>
#include <functional>

class Skin final : public ISmmPlugin, public IMetamodListener
{
public:
	bool Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late);
	bool Unload(char* error, size_t maxlen);
	void NextFrame(std::function<void()> fn);
private:
	const char* GetAuthor();
	const char* GetName();
	const char* GetDescription();
	const char* GetURL();
	const char* GetLicense();
	const char* GetVersion();
	const char* GetDate();
	const char* GetLogTag();

private: // Hooks
	void StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession*, const char*);
	void GameFrame(bool simulating, bool bFirstTick, bool bLastTick);

	std::deque<std::function<void()>> m_nextFrame;
};

class CPlayerSpawnEvent : public IGameEventListener2
{
	void FireGameEvent(IGameEvent* event) override;
};

class CRoundPreStartEvent : public IGameEventListener2
{
	void FireGameEvent(IGameEvent* event) override;
};

class CEntityListener : public IEntityListener
{
	void OnEntitySpawned(CEntityInstance* pEntity) override;
	void OnEntityParentChanged(CEntityInstance *pEntity, CEntityInstance *pNewParent) override;
	// void OnEntityCreated(CEntityInstance* pEntity) override;
};

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_


#define CON_COMMAND_CHAT_FLAGS(name, description, flags)																								\
	void name##_callback(const CCommand &args, CCSPlayerController *player);																			\
	static CChatCommand name##_chat_command(#name, name##_callback, flags);																				\
	static void name##_con_callback(const CCommandContext &context, const CCommand &args)																\
	{																																					\
		CCSPlayerController *pController = nullptr;																										\
		if (context.GetPlayerSlot().Get() != -1)																										\
			pController = (CCSPlayerController *)g_pEntitySystem->GetBaseEntity((CEntityIndex)(context.GetPlayerSlot().Get() + 1));						\
																																						\
		name##_chat_command(args, pController);																											\
	}																																					\
	static ConCommandRefAbstract name##_ref;																											\
	static ConCommand name##_command(&name##_ref, COMMAND_PREFIX #name, name##_con_callback,															\
									description, FCVAR_CLIENT_CAN_EXECUTE | FCVAR_LINKED_CONCOMMAND);													\
	void name##_callback(const CCommand &args, CCSPlayerController *player)

#define CON_COMMAND_CHAT(name, description) CON_COMMAND_CHAT_FLAGS(name, description, ADMFLAG_NONE)
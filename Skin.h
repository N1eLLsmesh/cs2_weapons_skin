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
	void OnEntityCreated(CEntityInstance* pEntity) override;
	void OnEntityDeleted(CEntityInstance* pEntity) override;
	void OnEntityParentChanged(CEntityInstance *pEntity, CEntityInstance *pNewParent) override;
};

#endif //_INCLUDE_METAMOD_SOURCE_STUB_PLUGIN_H_
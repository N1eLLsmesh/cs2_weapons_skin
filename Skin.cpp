#include <stdio.h>
#include "Skin.h"
#include "metamod_oslink.h"
#include "utils.hpp"
#include <utlstring.h>
#include <KeyValues.h>
#include "sdk/schemasystem.h"
#include "sdk/CBaseEntity.h"
#include "sdk/CGameRulesProxy.h"
#include "sdk/CBasePlayerPawn.h"
#include "sdk/CCSPlayerController.h"
#include "sdk/CCSPlayer_ItemServices.h"
#include "sdk/CPlayer_ItemServices.cpp"
#include "sdk/CSmokeGrenadeProjectile.h"
#include <map>
#include <iostream>
#include <chrono>
#include <thread>
#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#else
#include "utils/module.h"
#endif
#include <string>
#include "utils/ctimer.h"

Skin g_Skin;
PLUGIN_EXPOSE(Skin, g_Skin);
IVEngineServer2* engine = nullptr;
IGameEventManager2* gameeventmanager = nullptr;
IGameResourceServiceServer* g_pGameResourceService = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CSchemaSystem* g_pCSchemaSystem = nullptr;
CCSGameRules* g_pGameRules = nullptr;
CPlayerSpawnEvent g_PlayerSpawnEvent;
CRoundPreStartEvent g_RoundPreStartEvent;
CEntityListener g_EntityListener;
bool g_bPistolRound;

float g_flUniversalTime;
float g_flLastTickedTime;
bool g_bHasTicked;

#define CHAT_PREFIX	" \x05[Cobra]\x01"
#define DEBUG_OUTPUT 1
#define FEATURE_STICKERS 0

typedef struct SkinParm
{
	int32_t m_iItemDefinitionIndex;
	int m_nFallbackPaintKit;
	int m_nFallbackSeed;
	float m_flFallbackWear;
	bool used = false;
}SkinParm;

typedef struct StickerParm
{
	int stickerDefIndex1;
	float stickerWear1;
	int stickerDefIndex2;
	float stickerWear2;
	int stickerDefIndex3;
	float stickerWear3;
	int stickerDefIndex4;
	float stickerWear4;
}StickerParm;

#ifdef _WIN32
typedef void*(FASTCALL* SubClassChange_t)(const CCommandContext &context, const CCommand &args);
typedef void*(FASTCALL* EntityRemove_t)(CGameEntitySystem*, void*, void*, uint64_t);
typedef void(FASTCALL* GiveNamedItem_t)(void* itemService, const char* pchName, void* iSubType, CEconItemView* pScriptItem, void* bForce, void* pOrigin);
typedef void(FASTCALL* UTIL_ClientPrintAll_t)(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4);
typedef void(FASTCALL *ClientPrint)(CBasePlayerController *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4);

extern SubClassChange_t FnSubClassChange;
extern EntityRemove_t FnEntityRemove;
extern GiveNamedItem_t FnGiveNamedItem;
extern UTIL_ClientPrintAll_t FnUTIL_ClientPrintAll;
extern ClientPrint_t FnUTIL_ClientPrint;


EntityRemove_t FnEntityRemove;
GiveNamedItem_t FnGiveNamedItem;
UTIL_ClientPrintAll_t FnUTIL_ClientPrintAll;
ClientPrint_t FnUTIL_ClientPrint;
SubClassChange_t FnSubClassChange;

#else
void (*FnEntityRemove)(CGameEntitySystem*, void*, void*, uint64_t) = nullptr;
void (*FnGiveNamedItem)(void* itemService, const char* pchName, void* iSubType, CEconItemView* pScriptItem, void* bForce, void* pOrigin) = nullptr;
void (*FnUTIL_ClientPrintAll)(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4) = nullptr;
void (*FnUTIL_ClientPrint)(CBasePlayerController *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4);
void (*FnSubClassChange)(const CCommandContext &context, const CCommand &args) = nullptr;
#endif

std::map<int, std::string> g_WeaponsMap;
std::map<int, std::string> g_KnivesMap;
std::map<int, int> g_ItemToSlotMap;
std::map<uint64_t, SkinParm> g_PlayerSkins;
std::map<uint64_t, StickerParm> g_PlayerStickers;
std::map<uint64_t, int> g_PlayerMessages;
int g_iItemIDHigh = 16384;

class GameSessionConfiguration_t { };
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&, ISource2WorldSession*, const char*);
SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);

CGlobalVars *GetGameGlobals()
{
	INetworkGameServer *server = g_pNetworkServerService->GetIGameServer();
	if(!server) {
		return nullptr;
	}
	return g_pNetworkServerService->GetIGameServer()->GetGlobals();
}

#ifdef _WIN32
inline void* FindSignature(const char* modname,const char* sig)
{
	DWORD64 hModule = (DWORD64)GetModuleHandle(modname);
	if (!hModule)
	{
		return NULL;
	}
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	MODULEENTRY32 mod = {sizeof(MODULEENTRY32)};
	
	while (Module32Next(hSnap, &mod))
	{
		if (!strcmp(modname, mod.szModule))
		{
			if(!strstr(mod.szExePath,"metamod"))
				break;
		}
	}
	CloseHandle(hSnap);
	byte* b_sig = (byte*)sig;
	int sig_len = strlen(sig);
	byte* addr = (byte*)mod.modBaseAddr;
	for (int i = 0; i < mod.modBaseSize; i++)
	{
		int flag = 0;
		for (int n = 0; n < sig_len; n++)
		{
			if (i + n >= mod.modBaseSize)break;
			if (*(b_sig + n)=='\x3F' || *(b_sig + n) == *(addr + i+ n))
			{
				flag++;
			}
		}
		if (flag == sig_len)
		{
			return (void*)(addr + i);
		}
	}
	return NULL;
}
#endif

bool Skin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceService, IGameResourceServiceServer, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	// Get CSchemaSystem
	{
		HINSTANCE m_hModule = dlmount(WIN_LINUX("schemasystem.dll", "libschemasystem.so"));
		g_pCSchemaSystem = reinterpret_cast<CSchemaSystem*>(reinterpret_cast<CreateInterfaceFn>(dlsym(m_hModule, "CreateInterface"))(SCHEMASYSTEM_INTERFACE_VERSION, nullptr));
		dlclose(m_hModule);
	}

	SH_ADD_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &Skin::StartupServer), true);
	SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &Skin::GameFrame), true);

	gameeventmanager = static_cast<IGameEventManager2*>(CallVFunc<IToolGameEventAPI*, 91>(g_pSource2Server));

	ConVar_Register(FCVAR_GAMEDLL);

	g_WeaponsMap = { {26,"weapon_bizon"},{17,"weapon_mac10"},{34,"weapon_mp9"},{19,"weapon_p90"},{24,"weapon_ump45"},{7,"weapon_ak47"},{8,"weapon_aug"},{10,"weapon_famas"},{13,"weapon_galilar"},{16,"weapon_m4a1"},{60,"weapon_m4a1_silencer"},{39,"weapon_sg556"},{9,"weapon_awp"},{11,"weapon_g3sg1"},{38,"weapon_scar20"},{40,"weapon_ssg08"},{27,"weapon_mag7"},{35,"weapon_nova"},{29,"weapon_sawedoff"},{25,"weapon_xm1014"},{14,"weapon_m249"},{9,"weapon_awp"},{28,"weapon_negev"},{1,"weapon_deagle"},{2,"weapon_elite"},{3,"weapon_fiveseven"},{4,"weapon_glock"},{32,"weapon_hkp2000"},{36,"weapon_p250"},{30,"weapon_tec9"},{61,"weapon_usp_silencer"},{63,"weapon_cz75a"},{64,"weapon_revolver"},{23, "weapon_mp5sd"},{33, "weapon_mp7"} };
	g_KnivesMap = { {59,"weapon_knife"},{42,"weapon_knife"},{526,"weapon_knife_kukri"},{508,"weapon_knife_m9_bayonet"},{500,"weapon_bayonet"},{514,"weapon_knife_survival_bowie"},{515,"weapon_knife_butterfly"},{512,"weapon_knife_falchion"},{505,"weapon_knife_flip"},{506,"weapon_knife_gut"},{509,"weapon_knife_tactical"},{516,"weapon_knife_push"},{520,"weapon_knife_gypsy_jackknife"},{522,"weapon_knife_stiletto"},{523,"weapon_knife_widowmaker"},{519,"weapon_knife_ursus"},{503,"weapon_knife_css"},{517,"weapon_knife_cord"},{518,"weapon_knife_canis"},{521,"weapon_knife_outdoor"},{525,"weapon_knife_skeleton"},{507,"weapon_knife_karambit"} };
	// g_KnivesMap = { {59,"weapon_knife"},{42,"weapon_knife"},{526,"weapon_knife"},{508,"weapon_knife"},{500,"weapon_knife"},{514,"weapon_knife"},{515,"weapon_knife"},{512,"weapon_knife"},{505,"weapon_knife"},{506,"weapon_knife"},{509,"weapon_knife"},{516,"weapon_knife"},{520,"weapon_knife"},{522,"weapon_knife"},{523,"weapon_knife"},{519,"weapon_knife"},{503,"weapon_knife"},{517,"weapon_knife"},{518,"weapon_knife"},{521,"weapon_knife"},{525,"weapon_knife"},{507,"weapon_knife"} };
	g_ItemToSlotMap = { {59, 0},{42, 0},{526, 0},{508, 0},{500, 0},{514, 0},{515, 0},{512, 0},{505, 0},{506, 0},{509, 0},{516, 0},{520, 0},{522, 0},{523, 0},{519, 0},{503, 0},{517, 0},{518, 0},{521, 0},{525, 0},{507, 0}, {42, 0}, {59, 0}, {32, 1}, {61, 1}, {1, 1}, {3, 1}, {2, 1}, {36, 1}, {63, 1}, {64, 1}, {30, 1}, {4, 1}, {14, 2}, {17, 2}, {23, 2}, {33, 2}, {28, 2}, {35, 2}, {19, 2}, {26, 2}, {29, 2}, {24, 2}, {25, 2}, {27, 2}, {34, 2}, {8, 3}, {9, 3}, {10, 3}, {60, 3}, {16, 3}, {38, 3}, {40, 3}, {7, 3}, {11, 3}, {13, 3}, {39, 3} };

	#ifdef _WIN32	
	byte* vscript = (byte*)FindSignature("vscript.dll", "\xBE\x01\x3F\x3F\x3F\x2B\xD6\x74\x61\x3B\xD6");
	if(vscript)
	{
		DWORD pOld;
		VirtualProtect(vscript, 2, PAGE_EXECUTE_READWRITE, &pOld);
		*(vscript + 1) = 2;
		VirtualProtect(vscript, 2, pOld, &pOld);
	}
	#endif
	return true;
}

bool Skin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &Skin::GameFrame), true);
	SH_REMOVE_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &Skin::StartupServer), true);

	gameeventmanager->RemoveListener(&g_PlayerSpawnEvent);
	gameeventmanager->RemoveListener(&g_RoundPreStartEvent);

	g_pGameEntitySystem->RemoveListenerEntity(&g_EntityListener);

	ConVar_Unregister();
	
	return true;
}

void Skin::NextFrame(std::function<void()> fn)
{
	m_nextFrame.push_back(fn);
}

void Skin::StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession*, const char*)
{
	#ifdef _WIN32
	FnUTIL_ClientPrintAll = (UTIL_ClientPrintAll_t)FindSignature("server.dll", "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x81\xEC\x70\x01\x3F\x3F\x8B\xE9");
	FnGiveNamedItem = (GiveNamedItem_t)FindSignature("server.dll", "\x48\x89\x5C\x24\x18\x48\x89\x74\x24\x20\x55\x57\x41\x54\x41\x56\x41\x57\x48\x8D\x6C\x24\xD9");
	FnEntityRemove = (EntityRemove_t)FindSignature("server.dll", "\x48\x85\xD2\x0F\x3F\x3F\x3F\x3F\x3F\x57\x48\x3F\x3F\x3F\x48\x89\x3F\x3F\x3F\x48\x8B\xF9\x48\x8B");
	FnSubClassChange = (SubClassChange_t)FindSignature("server.dll", "\x40\x55\x41\x57\x48\x83\xEC\x78\x83\xBA\x38\x04");
	#else
	CModule libserver(g_pSource2Server);
	FnUTIL_ClientPrintAll = libserver.FindPatternSIMD("55 48 89 E5 41 57 49 89 D7 41 56 49 89 F6 41 55 41 89 FD").RCast< decltype(FnUTIL_ClientPrintAll) >();
	// FnGiveNamedItem = libserver.FindPatternSIMD("55 48 89 E5 41 57 41 56 49 89 CE 41 55 49 89 F5 41 54 49 89 D4 53 48 89").RCast<decltype(FnGiveNamedItem)>();
	FnGiveNamedItem = libserver.FindPatternSIMD("55 48 89 E5 41 57 41 56 49 89 CE 41 55 49 89 F5 41 54 49 89 D4").RCast<decltype(FnGiveNamedItem)>();
	// FnGiveNamedItem = libserver.FindPatternSIMD("55 48 89 E5 41 57 41 56 45 31 F6 41 55 49 89 CD").RCast<decltype(FnGiveNamedItem)>();
	FnEntityRemove = libserver.FindPatternSIMD("48 85 F6 74 0B 48 8B 76 10 E9 B2 FE FF FF").RCast<decltype(FnEntityRemove)>();
	FnUTIL_ClientPrint = libserver.FindPatternSIMD("55 48 89 E5 41 57 49 89 CF 41 56 49 89 D6 41 55 41 89 F5 41 54 4C 8D A5 A0 FE FF FF").RCast<decltype(FnUTIL_ClientPrint)>();
	FnSubClassChange = libserver.FindPatternSIMD("55 48 89 E5 41 57 41 56 41 55 41 54 53 48 81 EC C8 00 00 00 83 BE 38 04 00 00 01 0F 8E 47 02").RCast<decltype(FnSubClassChange)>();
	#endif
	g_pGameRules = nullptr;

	static bool bDone = false;
	if (!bDone)
	{
		g_pGameEntitySystem = *reinterpret_cast<CGameEntitySystem**>(reinterpret_cast<uintptr_t>(g_pGameResourceService) + WIN_LINUX(0x58, 0x50));
		g_pEntitySystem = g_pGameEntitySystem;

		g_pGameEntitySystem->AddListenerEntity(&g_EntityListener);

		gameeventmanager->AddListener(&g_PlayerSpawnEvent, "player_spawn", true);
		gameeventmanager->AddListener(&g_RoundPreStartEvent, "round_prestart", true);

		bDone = true;
	}
}

void Skin::GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
	// CTimer
	
	if (simulating && g_bHasTicked) {
		g_flUniversalTime += GetGameGlobals()->curtime - g_flLastTickedTime;
	} else {
		g_flUniversalTime += GetGameGlobals()->interval_per_tick;
	}

	g_flLastTickedTime = GetGameGlobals()->curtime;
	g_bHasTicked = true;

	for (int i = g_timers.Tail(); i != g_timers.InvalidIndex();) {
		auto timer = g_timers[i];

		int prevIndex = i;
		i = g_timers.Previous(i);

		if (timer->m_flLastExecute == -1) {
			timer->m_flLastExecute = g_flUniversalTime;
		}

		// Timer execute
		if (timer->m_flLastExecute + timer->m_flTime <= g_flUniversalTime) {
			timer->Execute();
			if (!timer->m_bRepeat) {
				delete timer;
				g_timers.Remove(prevIndex);
			} else {
				timer->m_flLastExecute = g_flUniversalTime;
			}
		}
	}
	
	if (!g_pGameRules) {
		CCSGameRulesProxy* pGameRulesProxy = static_cast<CCSGameRulesProxy*>(UTIL_FindEntityByClassname(nullptr, "cs_gamerules"));
		if (pGameRulesProxy) {
			g_pGameRules = pGameRulesProxy->m_pGameRules();
		}
	}
	
	while (!m_nextFrame.empty()) {
		m_nextFrame.front()();
		m_nextFrame.pop_front();
	}
}

void CPlayerSpawnEvent::FireGameEvent(IGameEvent* event)
{
	if (!g_pGameRules || g_pGameRules->m_bWarmupPeriod())
	{
		return;
	}
	CBasePlayerController* pPlayerController = static_cast<CBasePlayerController*>(event->GetPlayerController("userid"));
	if (!pPlayerController || pPlayerController->m_steamID() == 0) // Ignore bots
	{
		return;
	}

	g_Skin.NextFrame([hPlayerController = CHandle<CBasePlayerController>(pPlayerController), pPlayerController = pPlayerController]()
	{
		int64_t steamid = pPlayerController->m_steamID();
		auto message = g_PlayerMessages.find(steamid);
		if (message != g_PlayerMessages.end())
		{
			return;
		}
		char buf[255] = { 0 };
		char buf2[255] = { 0 };
		sprintf(buf, "%s\x0b Welcome to my Skin Server!", CHAT_PREFIX);
		sprintf(buf2, "%s Console command: \x06skin \x04ItemDefIndex PaintKit PatternID Float\x01", CHAT_PREFIX);
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
		FnUTIL_ClientPrint(pPlayerController, 3, buf2, nullptr, nullptr, nullptr, nullptr);
		g_PlayerMessages[steamid] = 1;
	});
}

void CRoundPreStartEvent::FireGameEvent(IGameEvent* event)
{
	if (g_pGameRules)
	{
		g_bPistolRound = g_pGameRules->m_totalRoundsPlayed() == 0 || (g_pGameRules->m_bSwitchingTeamsAtRoundReset() && g_pGameRules->m_nOvertimePlaying() == 0) || g_pGameRules->m_bGameRestart();
	}
}

void CEntityListener::OnEntityParentChanged(CEntityInstance *pEntity, CEntityInstance *pNewParent) {
	CBasePlayerWeapon* pBasePlayerWeapon = dynamic_cast<CBasePlayerWeapon*>(pEntity);
	CEconEntity* pCEconEntityWeapon = dynamic_cast<CEconEntity*>(pEntity);
	if(!pBasePlayerWeapon) return;
	if (DEBUG_OUTPUT) { META_CONPRINTF("OnpBasePlayerWeaponParentChanged\n"); }
}

void CEntityListener::OnEntityCreated(CEntityInstance *pEntity) {
	CBasePlayerWeapon* pBasePlayerWeapon = dynamic_cast<CBasePlayerWeapon*>(pEntity);
	CEconEntity* pCEconEntityWeapon = dynamic_cast<CEconEntity*>(pEntity);
	if(!pBasePlayerWeapon) return;
	if (DEBUG_OUTPUT) { META_CONPRINTF("OnpBasePlayerWeaponCreated\n"); }
}

void CEntityListener::OnEntityDeleted(CEntityInstance *pEntity) { }

void CEntityListener::OnEntitySpawned(CEntityInstance* pEntity)
{
	CBasePlayerWeapon* pBasePlayerWeapon = dynamic_cast<CBasePlayerWeapon*>(pEntity);
	CEconEntity* pCEconEntityWeapon = dynamic_cast<CEconEntity*>(pEntity);
	if(!pBasePlayerWeapon) return;
	if (DEBUG_OUTPUT) { META_CONPRINTF("OnBasePlayerWeaponSpawned\n"); }
	g_Skin.NextFrame([pBasePlayerWeapon = pBasePlayerWeapon, pCEconEntityWeapon = pCEconEntityWeapon]()
	{
		if (DEBUG_OUTPUT) {
			META_CONPRINTF( "----------------Spawned ENTITY------------------------\n");
			META_CONPRINTF("Entity Classname: %s\n", pBasePlayerWeapon->GetClassname());
			META_CONPRINTF( "--------------------CEconEntity-----------------------\n");
			META_CONPRINTF("pCEconEntityWeapon->m_OriginalOwnerXuidLow: %d\n", pCEconEntityWeapon->m_OriginalOwnerXuidLow());
			META_CONPRINTF("pCEconEntityWeapon->m_OriginalOwnerXuidHigh: %d\n", pCEconEntityWeapon->m_OriginalOwnerXuidHigh());
			META_CONPRINTF("pCEconEntityWeapon->m_nFallbackPaintKit: %d\n", pCEconEntityWeapon->m_nFallbackPaintKit());
			META_CONPRINTF("pCEconEntityWeapon->m_nFallbackSeed: %d\n", pCEconEntityWeapon->m_nFallbackSeed());
			META_CONPRINTF("pCEconEntityWeapon->m_flFallbackWear: %f\n", pCEconEntityWeapon->m_flFallbackWear());
			META_CONPRINTF("pCEconEntityWeapon->m_nFallbackStatTrak: %d\n", pCEconEntityWeapon->m_nFallbackStatTrak());
			META_CONPRINTF( "--------------------CEconItemView---------------------\n");
			META_CONPRINTF("pCEconEntityWeapon->m_iItemDefinitionIndex: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex());
			META_CONPRINTF("pCEconEntityWeapon->m_iEntityQuality: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iEntityQuality());
			META_CONPRINTF("pCEconEntityWeapon->m_iEntityLevel: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iEntityLevel());
			META_CONPRINTF("pCEconEntityWeapon->m_iItemID: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemID());
			META_CONPRINTF("pCEconEntityWeapon->m_iItemIDLow: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemIDLow());
			META_CONPRINTF("pCEconEntityWeapon->m_iItemIDHigh: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemIDHigh());
			META_CONPRINTF("pCEconEntityWeapon->m_iAccountID: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iAccountID());
			META_CONPRINTF("pCEconEntityWeapon->m_iInventoryPosition: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iInventoryPosition());
			META_CONPRINTF("pCEconEntityWeapon->m_bInitialized: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_bInitialized());
			META_CONPRINTF( "--------------------ENTITY----------------------------\n");
		}
		
		int64_t steamid = pCEconEntityWeapon->m_OriginalOwnerXuidLow() | (static_cast<int64_t>(pCEconEntityWeapon->m_OriginalOwnerXuidHigh()) << 32);
		int64_t weaponId = pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex();
		if(!steamid) {
			return;
		}

		auto skin_parm = g_PlayerSkins.find(steamid);
		if(skin_parm == g_PlayerSkins.end()) {
			return;
		}

		if(skin_parm->second.m_iItemDefinitionIndex == -1 || skin_parm->second.m_nFallbackPaintKit == -1 || skin_parm->second.m_nFallbackSeed == -1 || skin_parm->second.m_flFallbackWear == -1) {
			return;
		}

		pCEconEntityWeapon->m_nFallbackPaintKit() = skin_parm->second.m_nFallbackPaintKit;
		pCEconEntityWeapon->m_nFallbackSeed() = skin_parm->second.m_nFallbackSeed;
		pCEconEntityWeapon->m_flFallbackWear() = skin_parm->second.m_flFallbackWear;
		pCEconEntityWeapon->m_nFallbackStatTrak() = -1;

		pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex() = skin_parm->second.m_iItemDefinitionIndex;
		pCEconEntityWeapon->m_AttributeManager().m_Item().m_iInventoryPosition() = 0;

		uint64_t newItemID = 16384;
		uint32_t newItemIDLow = newItemID & 0xFFFFFFFF;
		uint32_t newItemIDHigh = newItemID >> 32;

		pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemIDLow() = newItemIDLow;
		pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemIDHigh() = newItemIDHigh;
		pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemID() = newItemID;

		if (DEBUG_OUTPUT) { META_CONPRINTF("Before Stickers\n"); }

		auto sticker_parm = g_PlayerStickers.find(steamid);
		if(sticker_parm != g_PlayerStickers.end() && FEATURE_STICKERS) {
			// Work in progress
			if (sticker_parm->second.stickerDefIndex1 != 0) {
				pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(113 + 1 * 4, sticker_parm->second.stickerDefIndex1);
				if (sticker_parm->second.stickerWear1 != 0) {
					pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(114 + 1 * 4, sticker_parm->second.stickerWear1);
				}
				if (DEBUG_OUTPUT) { META_CONPRINTF("sticker_parm->second.stickerDefIndex1: %d\n", sticker_parm->second.stickerDefIndex1); }
				if (DEBUG_OUTPUT) { META_CONPRINTF("sticker_parm->second.stickerWear1: %f\n", sticker_parm->second.stickerWear1); }
			}

			if (sticker_parm->second.stickerDefIndex2 != 0) {
				pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(117 + 2 * 4, sticker_parm->second.stickerDefIndex2);
				if (sticker_parm->second.stickerWear2 != 0) {
					pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(118 + 2 * 4, sticker_parm->second.stickerWear2);
				}
				if (DEBUG_OUTPUT) { META_CONPRINTF("sticker_parm->second.stickerDefIndex2: %d\n", sticker_parm->second.stickerDefIndex2); }
				if (DEBUG_OUTPUT) { META_CONPRINTF("sticker_parm->second.stickerWear2: %f\n", sticker_parm->second.stickerWear2); }
			}

			if (sticker_parm->second.stickerDefIndex3 != 0) {
				pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(121 + 3 * 4, sticker_parm->second.stickerDefIndex3);
				if (sticker_parm->second.stickerWear3 != 0) {
					pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(122 + 3 * 4, sticker_parm->second.stickerWear3);
				}
				if (DEBUG_OUTPUT) { META_CONPRINTF("sticker_parm->second.stickerDefIndex3: %d\n", sticker_parm->second.stickerDefIndex3); }
				if (DEBUG_OUTPUT) { META_CONPRINTF("sticker_parm->second.stickerWear3: %f\n", sticker_parm->second.stickerWear3); }
			}

			if (sticker_parm->second.stickerDefIndex4 != 0) {
				pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(125 + 4 * 4, sticker_parm->second.stickerDefIndex4);
				if (sticker_parm->second.stickerWear4 != 0) {
					pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(126 + 4 * 4, sticker_parm->second.stickerWear4);
				}
				if (DEBUG_OUTPUT) { META_CONPRINTF("sticker_parm->second.stickerDefIndex4: %d\n", sticker_parm->second.stickerDefIndex4); }
				if (DEBUG_OUTPUT) { META_CONPRINTF("sticker_parm->second.stickerWear4: %f\n", sticker_parm->second.stickerWear4); }
			}



			sticker_parm->second.stickerDefIndex1 = 0;
			sticker_parm->second.stickerDefIndex2 = 0;
			sticker_parm->second.stickerDefIndex3 = 0;
			sticker_parm->second.stickerDefIndex4 = 0;
			sticker_parm->second.stickerWear1 = 0;
			sticker_parm->second.stickerWear2 = 0;
			sticker_parm->second.stickerWear3 = 0;
			sticker_parm->second.stickerWear4 = 0;

			META_CONPRINTF("m_AttributeList().m_Attributes().Count(): %d\n", pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().m_Attributes.Count());
		}

		if (DEBUG_OUTPUT) { META_CONPRINTF("After Stickers\n"); }

		if(pBasePlayerWeapon->m_CBodyComponent() && pBasePlayerWeapon->m_CBodyComponent()->m_pSceneNode())
		{
			pBasePlayerWeapon->m_CBodyComponent()->m_pSceneNode()->GetSkeletonInstance()->m_modelState().m_MeshGroupMask() = 2;
		}
		auto knife_name = g_KnivesMap.find(weaponId);
		if(knife_name != g_KnivesMap.end()) {
			char buf[64] = {0};
			int index = static_cast<CEntityInstance*>(pBasePlayerWeapon)->m_pEntity->m_EHandle.GetEntryIndex();
			sprintf(buf, "i_subclass_change %d %d", skin_parm->second.m_iItemDefinitionIndex, index);
			engine->ServerCommand(buf);
			if (DEBUG_OUTPUT) { META_CONPRINTF( "i_subclass_change triggered\n"); }
		}

		skin_parm->second.m_iItemDefinitionIndex = -1;
		skin_parm->second.m_nFallbackPaintKit = -1;
		skin_parm->second.m_nFallbackSeed = -1;
		skin_parm->second.m_flFallbackWear = 0;
		if (DEBUG_OUTPUT) {
			META_CONPRINTF( "--------------------ENTITY----------------------------\n");
			META_CONPRINTF("Entity Classname: %s\n", pBasePlayerWeapon->GetClassname());
			META_CONPRINTF( "--------------------CEconEntity-----------------------\n");
			META_CONPRINTF("pCEconEntityWeapon->m_OriginalOwnerXuidLow: %d\n", pCEconEntityWeapon->m_OriginalOwnerXuidLow());
			META_CONPRINTF("pCEconEntityWeapon->m_OriginalOwnerXuidHigh: %d\n", pCEconEntityWeapon->m_OriginalOwnerXuidHigh());
			META_CONPRINTF("pCEconEntityWeapon->m_nFallbackPaintKit: %d\n", pCEconEntityWeapon->m_nFallbackPaintKit());
			META_CONPRINTF("pCEconEntityWeapon->m_nFallbackSeed: %d\n", pCEconEntityWeapon->m_nFallbackSeed());
			META_CONPRINTF("pCEconEntityWeapon->m_flFallbackWear: %f\n", pCEconEntityWeapon->m_flFallbackWear());
			META_CONPRINTF("pCEconEntityWeapon->m_nFallbackStatTrak: %d\n", pCEconEntityWeapon->m_nFallbackStatTrak());
			META_CONPRINTF( "--------------------CEconItemView---------------------\n");
			META_CONPRINTF("pCEconEntityWeapon->m_iItemDefinitionIndex: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex());
			META_CONPRINTF("pCEconEntityWeapon->m_iEntityQuality: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iEntityQuality());
			META_CONPRINTF("pCEconEntityWeapon->m_iEntityLevel: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iEntityLevel());
			META_CONPRINTF("pCEconEntityWeapon->m_iItemID: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemID());
			META_CONPRINTF("pCEconEntityWeapon->m_iItemIDLow: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemIDLow());
			META_CONPRINTF("pCEconEntityWeapon->m_iItemIDHigh: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iItemIDHigh());
			META_CONPRINTF("pCEconEntityWeapon->m_iAccountID: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iAccountID());
			META_CONPRINTF("pCEconEntityWeapon->m_iInventoryPosition: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_iInventoryPosition());
			META_CONPRINTF("pCEconEntityWeapon->m_bInitialized: %d\n", pCEconEntityWeapon->m_AttributeManager().m_Item().m_bInitialized());
			META_CONPRINTF("Player SteamID: %d\n", steamid);
			META_CONPRINTF( "--------------------ENTITY----------------------------\n");
		}
	});
}

CON_COMMAND_F(skin, "modify skin", FCVAR_CLIENT_CAN_EXECUTE) {
    if (context.GetPlayerSlot() == -1) {
		return;
	}

    CCSPlayerController* pPlayerController = (CCSPlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(context.GetPlayerSlot().Get() + 1));
    CCSPlayerPawnBase* pPlayerPawn = pPlayerController->m_hPlayerPawn();
    if (!pPlayerPawn || pPlayerPawn->m_lifeState() != LIFE_ALIVE) {
		return;
	}

    char buf[255] = { 0 };
	int32_t argDefIndex = 0;
	int64_t argPaintIndex = 0;
	int64_t argPattern = 0;
	float argWear = 0;
	int argStickerDefIndex1 = 0;
	float argStickerWear1 = 0;
	int argStickerDefIndex2 = 0;
	float argStickerWear2 = 0;
	int argStickerDefIndex3 = 0;
	float argStickerWear3 = 0;
	int argStickerDefIndex4 = 0;
	float argStickerWear4 = 0;

	if (args.Arg(1)) { argDefIndex = atoi(args.Arg(1)); }
	if (args.Arg(2)) { argPaintIndex = atoi(args.Arg(2)); }
	if (args.Arg(3)) { argPattern = atoi(args.Arg(3)); }
	if (args.Arg(4)) { argWear = atof(args.Arg(4)); }
	if (args.Arg(5)) { argStickerDefIndex1 = atoi(args.Arg(5)); }
	if (args.Arg(6)) { argStickerWear1 = atof(args.Arg(6)); }
	if (args.Arg(7)) { argStickerDefIndex2 = atoi(args.Arg(7)); }
	if (args.Arg(8)) { argStickerWear2 = atof(args.Arg(8)); }
	if (args.Arg(9)) { argStickerDefIndex3 = atoi(args.Arg(9)); }
	if (args.Arg(10)) { argStickerWear3 = atof(args.Arg(10)); }
	if (args.Arg(11)) { argStickerDefIndex4 = atoi(args.Arg(11)); }
	if (args.Arg(12)) { argStickerWear4 = atof(args.Arg(12)); }

	// if argWear is 0 or less, set it to 0.00000001, if its set to 1 or higher, set it to 0.99999999
	if (argWear <= 0) { argWear = 0.00000001; }
	if (argWear >= 1) { argWear = 0.99999999; }

	char bufWear[255] = { 0 };
	sprintf(bufWear, "%.8f", argWear);
	argWear = atof(bufWear);

	// if argPattern is 0 or less, set it to 1, if its set to 1001 or higher, set it to 1000
	if (argPattern <= 0) { argPattern = 1; }
	if (argPattern >= 1001) { argPattern = 1000; }

	if (argDefIndex == 0 || argPaintIndex == 0) {
        char buf2[255] = { 0 };
		sprintf(buf, "%s\x02 Wrong usage!", CHAT_PREFIX);
		sprintf(buf2, "%s Console command: \x06skin \x04ItemDefIndex PaintKit PatternID Float\x01", CHAT_PREFIX);
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
		FnUTIL_ClientPrint(pPlayerController, 3, buf2, nullptr, nullptr, nullptr, nullptr);
        return;
    }

    auto weapon_name = g_WeaponsMap.find(argDefIndex);
	bool isKnife = false;
	int64_t steamid = pPlayerController->m_steamID();
    CPlayer_WeaponServices* pWeaponServices = pPlayerPawn->m_pWeaponServices();

	if (weapon_name == g_WeaponsMap.end()) {
		weapon_name = g_KnivesMap.find(argDefIndex);
		isKnife = true;
	}

	if (weapon_name == g_KnivesMap.end()) {
		sprintf(buf, "%s\x02 Unknown Weapon/Knife ID", CHAT_PREFIX);
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
		return;
	}

	g_PlayerSkins[steamid].m_iItemDefinitionIndex = argDefIndex;
	g_PlayerSkins[steamid].m_nFallbackPaintKit = argPaintIndex;
	g_PlayerSkins[steamid].m_nFallbackSeed = argPattern;
	g_PlayerSkins[steamid].m_flFallbackWear = argWear;

	g_PlayerStickers[steamid].stickerDefIndex1 = argStickerDefIndex1;
	g_PlayerStickers[steamid].stickerWear1 = argStickerWear1;
	g_PlayerStickers[steamid].stickerDefIndex2 = argStickerDefIndex2;
	g_PlayerStickers[steamid].stickerWear2 = argStickerWear2;
	g_PlayerStickers[steamid].stickerDefIndex3 = argStickerDefIndex3;
	g_PlayerStickers[steamid].stickerWear3 = argStickerWear3;
	g_PlayerStickers[steamid].stickerDefIndex4 = argStickerDefIndex4;
	g_PlayerStickers[steamid].stickerWear4 = argStickerWear4;

    CBasePlayerWeapon* pPlayerWeapon = pWeaponServices->m_hActiveWeapon();
	const auto pPlayerWeapons = pWeaponServices->m_hMyWeapons();
	auto weapon_slot_map = g_ItemToSlotMap.find(argDefIndex);
	if (weapon_slot_map == g_ItemToSlotMap.end()) {
		sprintf(buf, "%s\x02 Unknown Weapon/Knife ID", CHAT_PREFIX);
		FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
		return;
	}
	auto weapon_slot = weapon_slot_map->second;
	for (size_t i = 0; i < pPlayerWeapons.m_size; i++)
	{
		auto currentWeapon = pPlayerWeapons.m_data[i];
		if (!currentWeapon)
			continue;
		auto weapon = static_cast<CEconEntity*>(currentWeapon.Get());
		if (!weapon)
			continue;
		auto weapon_slot_map_my_weapon = g_ItemToSlotMap.find(weapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex());
		if (weapon_slot_map_my_weapon == g_ItemToSlotMap.end()) {
			continue;
		}
		auto weapon_slot_my_weapon = weapon_slot_map_my_weapon->second;
		if (weapon_slot == weapon_slot_my_weapon) {
			pWeaponServices->RemoveWeapon(static_cast<CBasePlayerWeapon*>(currentWeapon.Get()));
			FnEntityRemove(g_pGameEntitySystem, static_cast<CBasePlayerWeapon*>(currentWeapon.Get()), nullptr, -1);
		}
	}

	FnGiveNamedItem(pPlayerPawn->m_pItemServices(), weapon_name->second.c_str(), nullptr, nullptr, nullptr, nullptr);
	if (DEBUG_OUTPUT) { META_CONPRINTF("called by %lld\n", steamid); }
	sprintf(buf, "%s\x04 Success!\x01 ItemDefIndex:\x04 %d\x01 PaintKit:\x04 %d\x01 PatternID:\x04 %d\x01 Float:\x04 %f\x01", CHAT_PREFIX, g_PlayerSkins[steamid].m_iItemDefinitionIndex, g_PlayerSkins[steamid].m_nFallbackPaintKit, g_PlayerSkins[steamid].m_nFallbackSeed, g_PlayerSkins[steamid].m_flFallbackWear);
	FnUTIL_ClientPrint(pPlayerController, 3, buf, nullptr, nullptr, nullptr, nullptr);
}

CON_COMMAND_F(test, "test", FCVAR_CLIENT_CAN_EXECUTE) {
    if (context.GetPlayerSlot() == -1) {
		return;
	}

    CCSPlayerController* pPlayerController = (CCSPlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(context.GetPlayerSlot().Get() + 1));
    CCSPlayerPawnBase* pPlayerPawn = pPlayerController->m_hPlayerPawn();
    if (!pPlayerPawn || pPlayerPawn->m_lifeState() != LIFE_ALIVE) {
		return;
	}

	CPlayer_WeaponServices* pWeaponServices = pPlayerPawn->m_pWeaponServices();
	CBasePlayerWeapon* pPlayerWeapon = pWeaponServices->m_hActiveWeapon();


	// pWeaponServices->RemoveWeapon(pPlayerWeapon);
	// FnEntityRemove(g_pGameEntitySystem, pPlayerWeapon, nullptr, -1);


	new CTimer(3.0f, false, false, [pPlayerPawn]() {

		CEconItemView* econItemView = new CEconItemView();
		econItemView.SetItemDefinitionIndex(507)
		econItemView.SetInitialized(true)

		META_CONPRINTF( "--------------------CEconItemView---------------------\n");
		META_CONPRINTF("econItemView->m_iItemDefinitionIndex: %d\n", econItemView->m_iItemDefinitionIndex());
		META_CONPRINTF("econItemView->m_iEntityQuality: %d\n", econItemView->m_iEntityQuality());
		META_CONPRINTF("econItemView->m_iEntityLevel: %d\n", econItemView->m_iEntityLevel());
		META_CONPRINTF("econItemView->m_iItemID: %d\n", econItemView->m_iItemID());
		META_CONPRINTF("econItemView->m_iItemIDLow: %d\n", econItemView->m_iItemIDLow());
		META_CONPRINTF("econItemView->m_iItemIDHigh: %d\n", econItemView->m_iItemIDHigh());
		META_CONPRINTF("econItemView->m_iAccountID: %d\n", econItemView->m_iAccountID());
		META_CONPRINTF("econItemView->m_iInventoryPosition: %d\n", econItemView->m_iInventoryPosition());
		META_CONPRINTF("econItemView->m_bInitialized: %d\n", econItemView->m_bInitialized());
		META_CONPRINTF( "--------------------ENTITY----------------------------\n");


        char buf[255] = { 0 };
		sprintf(buf, "%s Timer executed", CHAT_PREFIX);
		FnUTIL_ClientPrintAll(3, buf,nullptr, nullptr, nullptr, nullptr);
		FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife", nullptr, econItemView, nullptr, nullptr);
	});

	new CTimer(6.0f, false, false, [pPlayerPawn]() {
        
		CEconItemView* econItemView = new CEconItemView();
		econItemView.SetItemDefinitionIndex(500);
		econItemView.SetInitialized(true);

		META_CONPRINTF( "--------------------CEconItemView---------------------\n");
		META_CONPRINTF("econItemView->m_iItemDefinitionIndex: %d\n", econItemView->m_iItemDefinitionIndex());
		META_CONPRINTF("econItemView->m_iEntityQuality: %d\n", econItemView->m_iEntityQuality());
		META_CONPRINTF("econItemView->m_iEntityLevel: %d\n", econItemView->m_iEntityLevel());
		META_CONPRINTF("econItemView->m_iItemID: %d\n", econItemView->m_iItemID());
		META_CONPRINTF("econItemView->m_iItemIDLow: %d\n", econItemView->m_iItemIDLow());
		META_CONPRINTF("econItemView->m_iItemIDHigh: %d\n", econItemView->m_iItemIDHigh());
		META_CONPRINTF("econItemView->m_iAccountID: %d\n", econItemView->m_iAccountID());
		META_CONPRINTF("econItemView->m_iInventoryPosition: %d\n", econItemView->m_iInventoryPosition());
		META_CONPRINTF("econItemView->m_bInitialized: %d\n", econItemView->m_bInitialized());
		META_CONPRINTF( "--------------------ENTITY----------------------------\n");

		
		char buf[255] = { 0 };
		sprintf(buf, "%s Timer executed", CHAT_PREFIX);
		FnUTIL_ClientPrintAll(3, buf,nullptr, nullptr, nullptr, nullptr);
		FnGiveNamedItem(pPlayerPawn->m_pItemServices(), "weapon_knife", nullptr, econItemView, nullptr, nullptr);
	});
	char buf[255] = { 0 };
	sprintf(buf, "%s Timer started", CHAT_PREFIX);
	FnUTIL_ClientPrintAll(3, buf,nullptr, nullptr, nullptr, nullptr);
}


CON_COMMAND_F(i_subclass_change, "subclass change", FCVAR_NONE)
{
	FnSubClassChange(context,args);
}

const char* Skin::GetLicense()
{
	return "GPL";
}

const char* Skin::GetVersion()
{
	return "1.0.0";
}

const char* Skin::GetDate()
{
	return __DATE__;
}

const char* Skin::GetLogTag()
{
	return "skin";
}

const char* Skin::GetAuthor()
{
	return "Cobra";
}

const char* Skin::GetDescription()
{
	return "!ws for CS2";
}

const char* Skin::GetName()
{
	return "Change Skin";
}

const char* Skin::GetURL()
{
	return "https://google.com";
}
#pragma once
#include "CPlayerPawnComponent.h"
#include "schemasystem.h"
#include "ehandle.h"
#include "CBaseFlex.h"

class CEconEntity : public CBaseFlex
{
public:
};

class CEconItemAttribute
{
public:
	SCHEMA_FIELD(uint16_t, CEconItemAttribute, m_iAttributeDefinitionIndex);
	SCHEMA_FIELD(float, CEconItemAttribute, m_flValue);
	SCHEMA_FIELD(float, CEconItemAttribute, m_flInitialValue);
};

class CAttributeList
{
public:
	SCHEMA_FIELD(int64_t, CAttributeList, m_Attributes);
};

class CEconItemView
{
public:
	SCHEMA_FIELD(uint16_t, CEconItemView, m_iItemDefinitionIndex);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iEntityQuality);
	SCHEMA_FIELD(uint32_t, CEconItemView, m_iEntityLevel);
	SCHEMA_FIELD(uint64_t, CEconItemView, m_iItemID);
	SCHEMA_FIELD(uint32_t, CEconItemView, m_iItemIDHigh);
	SCHEMA_FIELD(uint32_t, CEconItemView, m_iItemIDLow);
	SCHEMA_FIELD(uint32_t, CEconItemView, m_iAccountID);
	SCHEMA_FIELD(bool, CEconItemView, m_bInitialized);
	SCHEMA_FIELD(CAttributeList, CEconItemView, m_AttributeList);
	SCHEMA_FIELD(char[161], CEconItemView, m_szCustomName);
	SCHEMA_FIELD(char[161], CEconItemView, m_szCustomNameOverride);
};

class CAttributeContainer
{
public:
	SCHEMA_FIELD(CEconItemView, CAttributeContainer, m_Item);
};

class CBasePlayerWeapon : public CEconEntity
{
public:
	SCHEMA_FIELD(CAttributeContainer, CEconEntity, m_AttributeManager);
	SCHEMA_FIELD(uint32_t, CEconEntity, m_OriginalOwnerXuidLow);
	SCHEMA_FIELD(uint32_t, CEconEntity, m_OriginalOwnerXuidHigh);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackPaintKit);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackSeed);
	SCHEMA_FIELD(float, CEconEntity, m_flFallbackWear);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackStatTrak);
	SCHEMA_FIELD(uint32_t, CBaseEntity, m_iOldOwnerClass);
};

class CPlayer_WeaponServices : public CPlayerPawnComponent
{
public:
	virtual ~CPlayer_WeaponServices() = 0;
	SCHEMA_FIELD(CHandle<CBasePlayerWeapon>, CPlayer_WeaponServices, m_hActiveWeapon);
	// SCHEMA_FIELD(CNetworkUtlVectorBase<CHandle<CBasePlayerWeapon>>, CPlayer_WeaponServices, m_hMyWeapons);
	auto RemoveWeapon(CBasePlayerWeapon* weapon) {
        return CALL_VIRTUAL(void, 20, this, weapon, nullptr, nullptr);
    }
};

class CPlayer_ItemServices : public CPlayerPawnComponent
{
public:
	virtual ~CPlayer_ItemServices() = 0;
};
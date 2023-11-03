#pragma once
#include "CPlayerPawnComponent.h"
#include "schemasystem.h"
#include "ehandle.h"
#include "CBaseFlex.h"

template <typename T>
class CUtlVector_NativeSdk {
   public:
    auto begin() const { return m_data; }
    auto end() const { return m_data + m_size; }

    bool Exists(T val) const {
        for (const auto& it : *this)
            if (it == val) return true;
        return false;
    }
    bool Empty() const { return m_size == 0; }

    int m_size;
    char pad0[0x4];  // no idea
    T* m_data;
    char pad1[0x8];  // no idea
};

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
	SCHEMA_FIELD(uint16_t, CEconItemView, m_nDefIndex);
};

class CAttributeList
{
public:
	SCHEMA_FIELD(int64_t, CAttributeList, m_Attributes);
};

class CEconItemView
{
public:
	SCHEMA_FIELD(CAttributeList, CEconItemView, m_AttributeList);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iEntityLevel);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iEntityQuality);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iItemIDLow);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iItemIDHigh);
	SCHEMA_FIELD(int32_t, CEconItemView, m_OriginalOwnerXuidLow);
	SCHEMA_FIELD(int32_t, CEconItemView, m_OriginalOwnerXuidHigh);
	SCHEMA_FIELD(uint32_t, CEconItemView, m_iAccountID);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iItemDefinitionIndex);
	SCHEMA_FIELD(bool, CEconItemView, m_bInitialized);
	SCHEMA_FIELD(char[32], CEconItemView, m_szCustomName);
};

class CAttributeContainer
{
public:
	SCHEMA_FIELD(CEconItemView, CAttributeContainer, m_Item);
};

class CModelState
{
public:
	SCHEMA_FIELD(uint64_t, CModelState, m_MeshGroupMask);
};

class CSkeletonInstance
{
public:
	SCHEMA_FIELD(CModelState, CSkeletonInstance, m_modelState);
};

class CGameSceneNode
{
public:
	CSkeletonInstance* GetSkeletonInstance() {
        return CALL_VIRTUAL(CSkeletonInstance*, 8, this);
    }
};

class CBodyComponent
{
public:
	SCHEMA_FIELD(CGameSceneNode*, CBodyComponent, m_pSceneNode);
};

class CBasePlayerWeapon : public CEconEntity
{
public:
	SCHEMA_FIELD(CBodyComponent*, CBaseEntity, m_CBodyComponent);
	SCHEMA_FIELD(CAttributeContainer, CEconEntity, m_AttributeManager);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackPaintKit);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackSeed);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackStatTrak);
	SCHEMA_FIELD(float, CEconEntity, m_flFallbackWear);
	SCHEMA_FIELD(uint64_t, CEconEntity, m_OriginalOwnerXuidLow);
	SCHEMA_FIELD(uint32_t, CBaseEntity, m_nSubclassID);
	SCHEMA_FIELD(int32_t, CBaseEntity, m_iOldOwnerClass);
};

class CPlayer_WeaponServices : public CPlayerPawnComponent
{
public:
	virtual ~CPlayer_WeaponServices() = 0;
	SCHEMA_FIELD(CHandle<CBasePlayerWeapon>, CPlayer_WeaponServices, m_hActiveWeapon);
	SCHEMA_FIELD(CHandle<CBasePlayerWeapon>, CPlayer_WeaponServices, m_hMyWeapons[48]);
	auto RemoveWeapon(CBasePlayerWeapon* weapon) {
        return CALL_VIRTUAL(void, 20, this, weapon, nullptr, nullptr);
    }
};

class CPlayer_ItemServices : public CPlayerPawnComponent
{
public:
	virtual ~CPlayer_ItemServices() = 0;
};
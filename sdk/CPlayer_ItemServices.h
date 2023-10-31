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

class CEconItemDefinition {
   public:
    auto GetModelName() {
        return *reinterpret_cast<const char**>((uintptr_t)(this) + 0xD8);
    }

    auto GetStickersSupportedCount() {
        return *reinterpret_cast<int*>((uintptr_t)(this) + 0x100);
    }

    auto GetSimpleWeaponName() {
        return *reinterpret_cast<const char**>((uintptr_t)(this) + 0x210);
    }

    auto GetLoadoutSlot() {
        return *reinterpret_cast<int*>((uintptr_t)(this) + 0x2E8);
    }

    char pad0[0x8];  // vtable
    void* m_pKVItem;
    uint16_t m_nDefIndex;
    CUtlVector_NativeSdk<uint16_t> m_nAssociatedItemsDefIndexes;
    bool m_bEnabled;
    const char* m_szPrefab;
    uint8_t m_unMinItemLevel;
    uint8_t m_unMaxItemLevel;
    uint8_t m_nItemRarity;
    uint8_t m_nItemQuality;
    uint8_t m_nForcedItemQuality;
    uint8_t m_nDefaultDropItemQuality;
    uint8_t m_nDefaultDropQuantity;
    CUtlVector_NativeSdk<void*> m_vecStaticAttributes;
    uint8_t m_nPopularitySeed;
    void* m_pPortraitsKV;
    const char* m_pszItemBaseName;
    bool m_bProperName;
    const char* m_pszItemTypeName;
    uint32_t m_unItemTypeID;
    const char* m_pszItemDesc;
};

class CEconItemView
{
public:
	SCHEMA_FIELD(CAttributeList, CEconItemView, m_AttributeList);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iItemIDLow);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iItemIDHigh);
	SCHEMA_FIELD(int32_t, CEconItemView, m_OriginalOwnerXuidLow);
	SCHEMA_FIELD(int32_t, CEconItemView, m_OriginalOwnerXuidHigh);
	SCHEMA_FIELD(int32_t, CEconItemView, m_hPrevOwner);
	SCHEMA_FIELD(uint32_t, CEconItemView, m_iAccountID);
	SCHEMA_FIELD(uint16_t, CEconItemView, m_iItemDefinitionIndex);
	SCHEMA_FIELD(bool, CEconItemView, m_bInitialized);
    auto GetStaticData() {
        return CALL_VIRTUAL(CEconItemDefinition*, 13, this);
    }

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
	auto RemoveWeapon(CBasePlayerWeapon* weapon) {
        return CALL_VIRTUAL(void, 20, this, weapon, nullptr, nullptr);
    }
};

class CPlayer_ItemServices : public CPlayerPawnComponent
{
public:
	virtual ~CPlayer_ItemServices() = 0;
};
#pragma once
#include "CBaseCombatCharacter.h"
#include "CPlayer_ItemServices.h"
#include "CBasePlayerController.h"
#include "ehandle.h"
#include "schemasystem.h"

class CCSPlayer_ViewModelServices {
   public:
	PSCHEMA_FIELD(m_hViewModel, "CCSPlayer_ViewModelServices", "m_hViewModel", CHandle);
};

class CBasePlayerPawn : public CBaseCombatCharacter
{
public:
	SCHEMA_FIELD(CPlayer_WeaponServices*, CBasePlayerPawn, m_pWeaponServices);
	SCHEMA_FIELD(CPlayer_ItemServices*, CBasePlayerPawn, m_pItemServices);
	SCHEMA_FIELD(CHandle<CBasePlayerController>, CBasePlayerPawn, m_hController);	
	SCHEMA_FIELD(CCSPlayer_ViewModelServices, CBasePlayerPawn, m_pViewModelServices);	
};
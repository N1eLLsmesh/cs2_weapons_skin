#pragma once
#include "CCSPlayerPawnBase.h"
#include "schemasystem.h"

class CCSPlayerPawn : public CCSPlayerPawnBase
{
public:
    auto ForceRespawnPlayer() { 
        return CALL_VIRTUAL(void, 324, this);
    }
    SCHEMA_FIELD(CEconItemView, CCSPlayerPawn, m_EconGloves);
};
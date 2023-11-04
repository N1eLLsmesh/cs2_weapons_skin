#pragma once
#include "CCSPlayerPawnBase.h"
#include "schemasystem.h"

class CCSPlayerPawn : public CCSPlayerPawnBase
{
public:
    auto ForceRespawnPlayer() { 
        return CALL_VIRTUAL(void, 324, this);
    }
};
// Add a custom rally point entity for NPC spawners

#include "cbase.h"
#include "so_zombie_spawner_rallypoint.h"

#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( so_zombie_spawner_rallypoint, CNPCRallyPoint );

CNPCRallyPoint::CNPCRallyPoint()
{
}

void CNPCRallyPoint::Spawn()
{
	Precache();

	SetSolid( SOLID_NONE );
	AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
	AddEffects( EF_NODRAW );
	SetMoveType( MOVETYPE_NONE );
}

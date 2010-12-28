// Add a custom rally point entity for NPC spawners

#ifndef SO_ZOMBIE_SPAWNER_RALLYPOINT_H
#define SO_ZOMBIE_SPAWNER_RALLYPOINT_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

class CNPCRallyPoint : public CBaseAnimating
{
public:
	DECLARE_CLASS( CNPCRallyPoint, CBaseAnimating );

	CNPCRallyPoint();

	void Spawn( void );
};

#endif // SO_ZOMBIE_SPAWNER_RALLYPOINT_H

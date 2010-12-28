#ifndef NPC_SO_BASEZOMBIE_H
#define NPC_SO_BASEZOMBIE_H
#ifdef _WIN32
#pragma once
#endif

#include "npc_BaseZombie.h"

// Add a custom rally point entity for NPC spawners
#include "so_zombie_spawner_rallypoint.h"

///////////////
// SCHEDULES //
///////////////
enum
{
	// Add a custom rally point entity for NPC spawners
	SCHED_ZOMBIE_AMBUSH_MODE,
};

////////////////////////////////////
// DEFINE ALL ZOMBIE CLASSES HERE //
////////////////////////////////////
/// Remember to update FGD too!  ///
////////////////////////////////////

static const char *ZombieNPCs[] = {
	"npc_creeper"
};

////////////////////////////////////

class CNPC_SO_BaseZombie : public CNPC_BaseZombie
{
	DECLARE_CLASS( CNPC_SO_BaseZombie, CNPC_BaseZombie );

public:
	CNPC_SO_BaseZombie( void );
	~CNPC_SO_BaseZombie( void );

	void Spawn( void );

	void StartNPC( void );
	void NPCThink( void );
	virtual void Event_Killed( const CTakeDamageInfo &info );

	// Improve zombies' perception of their surroundings
	// http://developer.valvesoftware.com/wiki/AI_Perception_Behavior_Enhancement
	int SelectAlertSchedule();

	// Should allow this NPC to see all other players and NPCs
	bool FInViewCone( CBaseEntity *pEntity );
	bool FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker );

	// Add a custom rally point entity for NPC spawners
	void BuildScheduleTestBits( void );

	// Should allow this NPC to see all other NPCs
	bool ShouldNotDistanceCull() { return true; }

	// Add a custom rally point entity for NPC spawners
	void SetRallyPoint( CNPCRallyPoint *rallyPoint ) { m_RallyPoint = rallyPoint; }

protected:
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;

private:
	// Add a custom rally point entity for NPC spawners
	void ForceMove( const Vector &targetPos, const Vector &traceDir, bool bRun );
	CNPCRallyPoint *m_RallyPoint;
	bool m_bShouldMoveTowardsRallyPoint;	// this boolean might not seem necessary if you follow the rally point logic from start to finish, but trust me, it is!
};

#endif // NPC_SO_BASEZOMBIE_H

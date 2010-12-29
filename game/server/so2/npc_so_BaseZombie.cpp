#include "cbase.h"
#include "npc_so_BaseZombie.h"

#include "so_gamerules.h"
#include "team.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CNPC_SO_BaseZombie )
END_DATADESC()

CNPC_SO_BaseZombie::CNPC_SO_BaseZombie()
{
	// Add a custom rally point entity for NPC spawners
	//m_RallyPoint = NULL;	// this might interfere with our logic, but I'm not sure...better comment it for now!
	m_bShouldMoveTowardsRallyPoint = false;
}

CNPC_SO_BaseZombie::~CNPC_SO_BaseZombie()
{
}

void CNPC_SO_BaseZombie::Spawn( void )
{
	m_fIsHeadless = true;	// no headcrabs!

	SetBloodColor( BLOOD_COLOR_RED );	// normal red blood, not that stupid zombie/green blood crap!

	m_flFieldOfView	= 1.0;	// zombies see all!

	BaseClass::Spawn();

	// GetTeamNumber() support
	ChangeTeam( TEAM_ZOMBIES );
}

void CNPC_SO_BaseZombie::StartNPC( void )
{
	BaseClass::StartNPC();

	// Add a custom rally point entity for NPC spawners
	m_bShouldMoveTowardsRallyPoint = true;	// move towards our rally point as soon as possible (assuming we have one)
}

void CNPC_SO_BaseZombie::NPCThink( void )
{
	BaseClass::NPCThink();

	// Add a custom rally point entity for NPC spawners
	if ( m_bShouldMoveTowardsRallyPoint )
	{
		if ( m_RallyPoint )
		{
			Vector forward;
			Vector vecGoal = m_RallyPoint->GetAbsOrigin();
			ForceMove( vecGoal, forward, true );
		}
		
		// Either we have a rally point and are now moving to it or we don't
		// In either case, we're done searching
		m_bShouldMoveTowardsRallyPoint = false;
	}
}

int CNPC_SO_BaseZombie::SelectAlertSchedule()
{
	// Improve zombies' perception of their surroundings
	// http://developer.valvesoftware.com/wiki/AI_Perception_Behavior_Enhancement
	if ( HasCondition ( COND_HEAR_DANGER ) ||
		 HasCondition ( COND_HEAR_PLAYER ) ||
		 HasCondition ( COND_HEAR_WORLD  ) ||
		 HasCondition ( COND_HEAR_BULLET_IMPACT ) ||
		 HasCondition ( COND_HEAR_COMBAT ) )
	{
		return SCHED_INVESTIGATE_SOUND;
	}

	return BaseClass::SelectAlertSchedule();
}

bool CNPC_SO_BaseZombie::FInViewCone( CBaseEntity *pEntity )
{
	// Should allow this NPC to see all other players and NPCs
	if ( pEntity->IsPlayer() || pEntity->IsNPC() )
		return true;

	return BaseClass::FInViewCone( pEntity );
}

bool CNPC_SO_BaseZombie::FVisible( CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker )
{
	// Should allow this NPC to see all other players and NPCs
	if ( pEntity->IsPlayer() || pEntity->IsNPC() )
		return true;

	return BaseClass::FVisible( pEntity, traceMask, ppBlocker );
}

void CNPC_SO_BaseZombie::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	// Add a custom rally point entity for NPC spawners
	if( IsCurSchedule( SCHED_ZOMBIE_AMBUSH_MODE ) )
		SetCustomInterruptCondition( COND_RECEIVED_ORDERS );
}

void CNPC_SO_BaseZombie::ForceMove( const Vector &targetPos, const Vector &traceDir, bool bRun ) 
{
	// Make sure our zombie gets the message (sometimes it is stubborn)
	// This bit should force our zombie to listen to us
	SetEnemy( NULL );
	SetSchedule( SCHED_ZOMBIE_AMBUSH_MODE );

	Vector chasePosition = targetPos;

	Vector vUpBit = chasePosition;
	vUpBit.z += 1;

	trace_t tr;
	AI_TraceHull( chasePosition, vUpBit, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	m_vecLastPosition = chasePosition;

	if ( m_hCine != NULL )
		ExitScriptedSequence();

	SetCondition( COND_RECEIVED_ORDERS );

	if ( bRun )
		SetSchedule( SCHED_FORCED_GO_RUN );
	else
		SetSchedule( SCHED_FORCED_GO );

	m_flMoveWaitFinished = gpGlobals->curtime;
}

void CNPC_SO_BaseZombie::Event_Killed( const CTakeDamageInfo &info )
{
	CBaseEntity *pEnt = info.GetAttacker();
	if ( pEnt && pEnt->IsPlayer() )
	{
		CSO_Player *pPlayer = ToSOPlayer( pEnt );
		if ( pPlayer )
		{
			// Give players some credit for killing zombie NPCs
			pPlayer->IncrementFragCount( 1 );
			GetGlobalTeam( pPlayer->GetTeamNumber() )->AddScore( 1 );
		}
	}

	BaseClass::Event_Killed( info );
}

AI_BEGIN_CUSTOM_NPC( so_base_zombie, CNPC_SO_BaseZombie )

	// Add a custom rally point entity for NPC spawners
	DEFINE_SCHEDULE
	(
		SCHED_ZOMBIE_AMBUSH_MODE,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_WAIT_PVS			0"
		"	"
		"	Interrupts"
		"		COND_RECEIVED_ORDERS" 
	)

AI_END_CUSTOM_NPC()

#include "cbase.h"
#include "basegrenade_shared.h"
#include "so_fraggrenade.h"
#include "soundent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FRAG_GRENADE_GRACE_TIME_AFTER_PICKUP 1.5f
#define FRAG_GRENADE_WARN_TIME 1.5f

const float GRENADE_COEFFICIENT_OF_RESTITUTION = 0.2f;

#define GRENADE_MODEL "models/Weapons/w_eq_fraggrenade_thrown.mdl"

class CSOFragGrenade : public CBaseGrenade
{
	DECLARE_CLASS( CSOFragGrenade, CBaseGrenade );

#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif
					
	~CSOFragGrenade( void );

public:
	void	Spawn( void );
	void	Precache( void );
	bool	CreateVPhysics( void );
	void	SetTimer( float detonateDelay, float warnDelay );
	void	SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity );
	int		OnTakeDamage( const CTakeDamageInfo &inputInfo );
	void	DelayThink();
	void	VPhysicsUpdate( IPhysicsObject *pPhysics );
	void	OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	void	SetCombineSpawned( bool combineSpawned ) { m_combineSpawned = combineSpawned; }
	bool	IsCombineSpawned( void ) const { return m_combineSpawned; }
	void	SetPunted( bool punt ) { m_punted = punt; }
	bool	WasPunted( void ) const { return m_punted; }

	// this function only used in episodic.
#ifdef HL2_EPISODIC
	bool	HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt);
#endif 

	void	InputSetTimer( inputdata_t &inputdata );

protected:
	bool	m_inSolid;
	bool	m_combineSpawned;
	bool	m_punted;
};

LINK_ENTITY_TO_CLASS( so_fraggrenade, CSOFragGrenade );

BEGIN_DATADESC( CSOFragGrenade )

	// Fields
	DEFINE_FIELD( m_inSolid, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_combineSpawned, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_punted, FIELD_BOOLEAN ),
	
	// Function Pointers
	DEFINE_THINKFUNC( DelayThink ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetTimer", InputSetTimer ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CSOFragGrenade::~CSOFragGrenade( void )
{
}

void CSOFragGrenade::Spawn( void )
{
	Precache( );

	SetModel( GRENADE_MODEL );

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= 1;

	SetSize( -Vector(4,4,4), Vector(4,4,4) );
	SetCollisionGroup( COLLISION_GROUP_WEAPON );
	CreateVPhysics();

	AddSolidFlags( FSOLID_NOT_STANDABLE );

	m_combineSpawned	= false;
	m_punted			= false;

	BaseClass::Spawn();
}

bool CSOFragGrenade::CreateVPhysics()
{
	// Create the object in the physics system
	VPhysicsInitNormal( SOLID_BBOX, 0, false );
	return true;
}

// this will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
class CTraceFilterCollisionGroupDelta : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCollisionGroupDelta );
	
	CTraceFilterCollisionGroupDelta( const IHandleEntity *passentity, int collisionGroupAlreadyChecked, int newCollisionGroup )
		: m_pPassEnt(passentity), m_collisionGroupAlreadyChecked( collisionGroupAlreadyChecked ), m_newCollisionGroup( newCollisionGroup )
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;
		CBaseEntity *pEntity = EntityFromEntityHandle( pHandleEntity );

		if ( pEntity )
		{
			if ( g_pGameRules->ShouldCollide( m_collisionGroupAlreadyChecked, pEntity->GetCollisionGroup() ) )
				return false;
			if ( g_pGameRules->ShouldCollide( m_newCollisionGroup, pEntity->GetCollisionGroup() ) )
				return true;
		}

		return false;
	}

protected:
	const IHandleEntity *m_pPassEnt;
	int		m_collisionGroupAlreadyChecked;
	int		m_newCollisionGroup;
};

void CSOFragGrenade::VPhysicsUpdate( IPhysicsObject *pPhysics )
{
	BaseClass::VPhysicsUpdate( pPhysics );
	Vector vel;
	AngularImpulse angVel;
	pPhysics->GetVelocity( &vel, &angVel );
	
	Vector start = GetAbsOrigin();
	// find all entities that my collision group wouldn't hit, but COLLISION_GROUP_NONE would and bounce off of them as a ray cast
	CTraceFilterCollisionGroupDelta filter( this, GetCollisionGroup(), COLLISION_GROUP_NONE );
	trace_t tr;

	// UNDONE: Hull won't work with hitboxes - hits outer hull.  But the whole point of this test is to hit hitboxes.
#if 0
	UTIL_TraceHull( start, start + vel * gpGlobals->frametime, CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );
#else
	UTIL_TraceLine( start, start + vel * gpGlobals->frametime, CONTENTS_HITBOX|CONTENTS_MONSTER|CONTENTS_SOLID, &filter, &tr );
#endif
	if ( tr.startsolid )
	{
		if ( !m_inSolid )
		{
			// UNDONE: Do a better contact solution that uses relative velocity?
			vel *= -GRENADE_COEFFICIENT_OF_RESTITUTION; // bounce backwards
			pPhysics->SetVelocity( &vel, NULL );
		}
		m_inSolid = true;
		return;
	}
	m_inSolid = false;
	if ( tr.DidHit() )
	{
		Vector dir = vel;
		VectorNormalize(dir);
		// send a tiny amount of damage so the character will react to getting bonked
		CTakeDamageInfo info( this, GetThrower(), pPhysics->GetMass() * vel, GetAbsOrigin(), 0.1f, DMG_CRUSH );
		tr.m_pEnt->TakeDamage( info );

		// reflect velocity around normal
		vel = -2.0f * tr.plane.normal * DotProduct(vel,tr.plane.normal) + vel;
		
		// absorb 80% in impact
		vel *= GRENADE_COEFFICIENT_OF_RESTITUTION;
		angVel *= -0.5f;
		pPhysics->SetVelocity( &vel, &angVel );
	}
}


void CSOFragGrenade::Precache( void )
{
	PrecacheModel( GRENADE_MODEL );

	BaseClass::Precache();
}

void CSOFragGrenade::SetTimer( float detonateDelay, float warnDelay )
{
	m_flDetonateTime = gpGlobals->curtime + detonateDelay;
	m_flWarnAITime = gpGlobals->curtime + warnDelay;
	SetThink( &CSOFragGrenade::DelayThink );
	SetNextThink( gpGlobals->curtime );
}

void CSOFragGrenade::OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason )
{
	SetThrower( pPhysGunUser );

#ifdef HL2MP
	SetTimer( FRAG_GRENADE_GRACE_TIME_AFTER_PICKUP, FRAG_GRENADE_GRACE_TIME_AFTER_PICKUP / 2);

	m_bHasWarnedAI = true;
#else
	if( IsX360() )
	{
		// Give 'em a couple of seconds to aim and throw. 
		SetTimer( 2.0f, 1.0f);
	}
#endif

#ifdef HL2_EPISODIC
	SetPunted( true );
#endif

	BaseClass::OnPhysGunPickup( pPhysGunUser, reason );
}

void CSOFragGrenade::DelayThink() 
{
	if( gpGlobals->curtime > m_flDetonateTime )
	{
		Detonate();
		return;
	}

	if( !m_bHasWarnedAI && gpGlobals->curtime >= m_flWarnAITime )
	{
#if !defined( CLIENT_DLL )
		CSoundEnt::InsertSound ( SOUND_DANGER, GetAbsOrigin(), 400, 1.5, this );
#endif
		m_bHasWarnedAI = true;
	}

	SetNextThink( gpGlobals->curtime + 0.1 );
}

void CSOFragGrenade::SetVelocity( const Vector &velocity, const AngularImpulse &angVelocity )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();
	if ( pPhysicsObject )
	{
		pPhysicsObject->AddVelocity( &velocity, &angVelocity );
	}
}

int CSOFragGrenade::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// Manually apply vphysics because BaseCombatCharacter takedamage doesn't call back to CBaseEntity OnTakeDamage
	VPhysicsTakeDamage( inputInfo );

	// Grenades only suffer blast damage and burn damage.
	if( !(inputInfo.GetDamageType() & (DMG_BLAST|DMG_BURN) ) )
		return 0;

	return BaseClass::OnTakeDamage( inputInfo );
}

#ifdef HL2_EPISODIC
extern int	g_interactionBarnacleVictimGrab; ///< usually declared in ai_interactions.h but no reason to haul all of that in here.
extern int g_interactionBarnacleVictimBite;
extern int g_interactionBarnacleVictimReleased;
bool CSOFragGrenade::HandleInteraction(int interactionType, void *data, CBaseCombatCharacter* sourceEnt)
{
	// allow fragnades to be grabbed by barnacles. 
	if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		// give the grenade another five seconds seconds so the player can have the satisfaction of blowing up the barnacle with it
		float timer = m_flDetonateTime - gpGlobals->curtime + 5.0f;
		SetTimer( timer, timer - FRAG_GRENADE_WARN_TIME );

		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimBite )
	{
		// detonate the grenade immediately 
		SetTimer( 0, 0 );
		return true;
	}
	else if ( interactionType == g_interactionBarnacleVictimReleased )
	{
		// take the five seconds back off the timer.
		float timer = max(m_flDetonateTime - gpGlobals->curtime - 5.0f,0.0f);
		SetTimer( timer, timer - FRAG_GRENADE_WARN_TIME );
		return true;
	}
	else
	{
		return BaseClass::HandleInteraction( interactionType, data, sourceEnt );
	}
}
#endif

void CSOFragGrenade::InputSetTimer( inputdata_t &inputdata )
{
	SetTimer( inputdata.value.Float(), inputdata.value.Float() - FRAG_GRENADE_WARN_TIME );
}

CBaseGrenade *SO_Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned )
{
	// Don't set the owner here, or the player can't interact with grenades he's thrown
	CSOFragGrenade *pGrenade = (CSOFragGrenade *)CBaseEntity::Create( "so_fraggrenade", position, angles, pOwner );
	
	pGrenade->SetTimer( timer, timer - FRAG_GRENADE_WARN_TIME );
	pGrenade->SetVelocity( velocity, angVelocity );
	pGrenade->SetThrower( ToBaseCombatCharacter( pOwner ) );
	pGrenade->m_takedamage = DAMAGE_EVENTS_ONLY;
	pGrenade->SetCombineSpawned( combineSpawned );

	return pGrenade;
}

bool SO_Fraggrenade_WasPunted( const CBaseEntity *pEntity )
{
	const CSOFragGrenade *pFrag = dynamic_cast<const CSOFragGrenade *>( pEntity );
	if ( pFrag )
	{
		return pFrag->WasPunted();
	}

	return false;
}

bool SO_Fraggrenade_WasCreatedByCombine( const CBaseEntity *pEntity )
{
	const CSOFragGrenade *pFrag = dynamic_cast<const CSOFragGrenade *>( pEntity );
	if ( pFrag )
	{
		return pFrag->IsCombineSpawned();
	}

	return false;
}
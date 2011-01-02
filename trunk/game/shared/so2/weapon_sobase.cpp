#include "cbase.h"
#include "weapon_sobase.h"
#include "hl2mp_weapon_parse.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_so_player.h"
#else
	#include "so_player.h"
#endif

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSOBase, DT_WeaponSOBase )

BEGIN_NETWORK_TABLE( CWeaponSOBase, DT_WeaponSOBase )
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponSOBase )
END_PREDICTION_DATA()
#endif

// Add support for CS:S player animations
// I know there's a better way to replace these animations, but I'm extremely lazy
acttable_t CWeaponSOBase::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE,	ACT_IDLE,			false },
	{ ACT_MP_CROUCH_IDLE,	ACT_CROUCHIDLE,		false },
	{ ACT_MP_RUN,			ACT_RUN,			false },
	{ ACT_MP_WALK,			ACT_WALK,			false },
	{ ACT_MP_CROUCHWALK,	ACT_RUN_CROUCH,		false },
	{ ACT_MP_JUMP,			ACT_HOP,			false },
};
IMPLEMENT_ACTTABLE( CWeaponSOBase );

CWeaponSOBase::CWeaponSOBase()
{
}

void CWeaponSOBase::Spawn( void )
{
	BaseClass::Spawn();

	// Make weapons that are on the ground blink so that they are easier for players to see
	AddEffects( EF_ITEM_BLINK );
}

void CWeaponSOBase::ItemPostFrame( void )
{
	CSO_Player *pOwner = ToSOPlayer( GetOwner() );
	if ( !pOwner )
		return;

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	if ( pOwner->GetHolsteredWeapon() == this )
		return;

	BaseClass::ItemPostFrame();
}

float CWeaponSOBase::GetAccuracyModifier()
{
	float weaponAccuracy = 1.0f; // by default, don't make any alterations

	CSO_Player *pPlayer = ToSOPlayer( GetOwner() );
	if ( pPlayer )
	{
		// If the player isn't moving at all, they are even more accurate
		if( !fabs(pPlayer->GetAbsVelocity().x) && !fabs(pPlayer->GetAbsVelocity().y) )
			weaponAccuracy *= 0.75f;
		// If the player is ducking, they are slightly more accurate
		else if( !!( pPlayer->GetFlags() & FL_DUCKING ) )
			weaponAccuracy *= 0.80f;
		// If the player is walking, they are more accurate
		else if( pPlayer->IsWalking() )
			weaponAccuracy *= 0.85f;
	}

	return weaponAccuracy;
}

// Add weapon bob

#if defined( CLIENT_DLL )

#define	HL2_BOB_CYCLE_MIN	0.5f	// HL2 default is 1.0f
#define	HL2_BOB_CYCLE_MAX	0.225f	// HL2 default 0.45f
#define	HL2_BOB			0.002f
#define	HL2_BOB_UP		0.5f

extern float	g_lateralBob;
extern float	g_verticalBob;

static ConVar	cl_bobcycle( "cl_bobcycle","0.8" );
static ConVar	cl_bob( "cl_bob","0.002" );
static ConVar	cl_bobup( "cl_bobup","0.5" );

// Register these cvars if needed for easy tweaking
static ConVar	v_iyaw_cycle( "v_iyaw_cycle", "2", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iroll_cycle( "v_iroll_cycle", "0.5", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_ipitch_cycle( "v_ipitch_cycle", "1", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iyaw_level( "v_iyaw_level", "0.3", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_iroll_level( "v_iroll_level", "0.1", FCVAR_REPLICATED | FCVAR_CHEAT );
static ConVar	v_ipitch_level( "v_ipitch_level", "0.3", FCVAR_REPLICATED | FCVAR_CHEAT );

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CWeaponSOBase::CalcViewmodelBob( void )
{
	static	float bobtime;
	static	float lastbobtime;
	float	cycle;
	
	CBasePlayer *player = ToBasePlayer( GetOwner() );
	//Assert( player );

	//NOTENOTE: For now, let this cycle continue when in the air, because it snaps badly without it

	if ( ( !gpGlobals->frametime ) || ( player == NULL ) )
	{
		//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
		return 0.0f;// just use old value
	}

	//Find the speed of the player
	float speed = player->GetLocalVelocity().Length2D();

	//FIXME: This maximum speed value must come from the server.
	//		 MaxSpeed() is not sufficient for dealing with sprinting - jdw

	speed = clamp( speed, -320, 320 );

	float bob_offset = RemapVal( speed, 0, 320, 0.0f, 1.0f );
	
	bobtime += ( gpGlobals->curtime - lastbobtime ) * bob_offset;
	lastbobtime = gpGlobals->curtime;

	//Calculate the vertical bob
	cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX)*HL2_BOB_CYCLE_MAX;
	cycle /= HL2_BOB_CYCLE_MAX;

	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
	}
	
	g_verticalBob = speed*0.005f;
	g_verticalBob = g_verticalBob*0.3 + g_verticalBob*0.7*sin(cycle);

	g_verticalBob = clamp( g_verticalBob, -7.0f, 4.0f );

	//Calculate the lateral bob
	cycle = bobtime - (int)(bobtime/HL2_BOB_CYCLE_MAX*2)*HL2_BOB_CYCLE_MAX*2;
	cycle /= HL2_BOB_CYCLE_MAX*2;

	if ( cycle < HL2_BOB_UP )
	{
		cycle = M_PI * cycle / HL2_BOB_UP;
	}
	else
	{
		cycle = M_PI + M_PI*(cycle-HL2_BOB_UP)/(1.0 - HL2_BOB_UP);
	}

	g_lateralBob = speed*0.005f;
	g_lateralBob = g_lateralBob*0.3 + g_lateralBob*0.7*sin(cycle);
	g_lateralBob = clamp( g_lateralBob, -7.0f, 4.0f );
	
	//NOTENOTE: We don't use this return value in our case (need to restructure the calculation function setup!)
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CWeaponSOBase::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
	Vector	forward, right;
	AngleVectors( angles, &forward, &right, NULL );

	CalcViewmodelBob();

	// Apply bob, but scaled down to 40%
	VectorMA( origin, g_verticalBob * 0.1f, forward, origin );
	
	// Z bob a bit more
	origin[2] += g_verticalBob * 0.1f;
	
	// bob the angles
	angles[ ROLL ]	+= g_verticalBob * 0.5f;
	angles[ PITCH ]	-= g_verticalBob * 0.4f;

	angles[ YAW ]	-= g_lateralBob  * 0.3f;

	VectorMA( origin, g_lateralBob * 0.8f, right, origin );
}

#else

// Server stubs
float CWeaponSOBase::CalcViewmodelBob( void )
{
	return 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &origin - 
//			&angles - 
//			viewmodelindex - 
//-----------------------------------------------------------------------------
void CWeaponSOBase::AddViewmodelBob( CBaseViewModel *viewmodel, Vector &origin, QAngle &angles )
{
}

#endif

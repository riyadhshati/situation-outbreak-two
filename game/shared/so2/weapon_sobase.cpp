#include "cbase.h"
#include "weapon_sobase.h"
#include "hl2mp_weapon_parse.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_so_player.h"
#else
	#include "so_player.h"

	// Weapon respawn fix
	// http://developer.valvesoftware.com/wiki/Weapon_Respawn_Fix
	#include "vphysics/constraints.h"
#endif

// Weapon reorigin system
// http://developer.valvesoftware.com/wiki/Adding_Ironsights
// Modified a bit from the wiki version considering our system's purpose
//forward declarations of callbacks used by viewmodel_adjust_enable and viewmodel_adjust_fov
void vm_adjust_enable_callback( IConVar *pConVar, char const *pOldString, float flOldValue );
void vm_adjust_fov_callback( IConVar *pConVar, const char *pOldString, float flOldValue );
ConVar viewmodel_adjust_forward( "viewmodel_adjust_forward", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_right( "viewmodel_adjust_right", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_up( "viewmodel_adjust_up", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_pitch( "viewmodel_adjust_pitch", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_yaw( "viewmodel_adjust_yaw", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_roll( "viewmodel_adjust_roll", "0", FCVAR_REPLICATED );
ConVar viewmodel_adjust_fov( "viewmodel_adjust_fov", "0", FCVAR_REPLICATED, "Note: this feature is not available during any kind of zoom", vm_adjust_fov_callback );
ConVar viewmodel_adjust_enabled( "viewmodel_adjust_enabled", "0", FCVAR_REPLICATED | FCVAR_CHEAT, "enabled viewmodel adjusting", vm_adjust_enable_callback );

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSOBase, DT_WeaponSOBase )

BEGIN_NETWORK_TABLE( CWeaponSOBase, DT_WeaponSOBase )
#ifdef CLIENT_DLL
	// Weapon scope system
	RecvPropBool( RECVINFO( m_bIsScoped ) ),
#else
	// Weapon scope system
	SendPropBool( SENDINFO( m_bIsScoped ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponSOBase )
	// Weapon scope system
	DEFINE_PRED_FIELD( m_bIsScoped, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),	// I believe the client may want this predicted for actually displaying the scope
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
	// Weapon scope system
	m_bIsScoped = false;
}

void CWeaponSOBase::Spawn( void )
{
	BaseClass::Spawn();

	// Make weapons that are on the ground blink so that they are easier for players to see
	AddEffects( EF_ITEM_BLINK );
}

bool CWeaponSOBase::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( BaseClass::Holster(pSwitchingTo) )
	{
		// Weapon scope system
		if ( HasScope() )
			ExitScope( false );

		return true;
	}

	return false;
}

bool CWeaponSOBase::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		// Weapon scope system
		if ( HasScope() )
			ExitScope();

		return true;
	}

	return false;
}

// Taken and modified from CWeaponHL2MPBase::Reload
//Tony; override for animation purposes.
bool CWeaponSOBase::Reload( void )
{
	bool fRet = DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );

	if ( fRet )
	{
		// Weapon scope system
		// Unscope while reloading
		if ( HasScope() )
			ExitScope();

		ToHL2MPPlayer( GetOwner() )->DoAnimationEvent( PLAYERANIMEVENT_RELOAD );
	}

	return fRet;
}

void CWeaponSOBase::ItemPostFrame( void )
{
	// Weapon scope system
	// We're not allowed to scope right now yet we are scoped, so unscope right away!
	if ( !CanScope() && m_bIsScoped )
		ExitScope();

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

// The following is taken and modified from CBaseCombatWeapon::PrimaryAttack
void CWeaponSOBase::PrimaryAttack( void )
{
	// Weapon scope system
	int preShotAmmo = m_iClip1;

	// If my clip is empty (and I use clips) start reload
	if ( UsesClipsForAmmo1() && !m_iClip1 ) 
	{
		Reload();
		return;
	}

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( GetPrimaryAttackActivity() );

	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );

	FireBulletsInfo_t info;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();
	
	info.m_vecDirShooting = pPlayer->GetAutoaimVector( AUTOAIM_SCALE_DEFAULT );

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while ( m_flNextPrimaryAttack <= gpGlobals->curtime )
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound( SINGLE, m_flNextPrimaryAttack );
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if ( !fireRate )
			break;
	}

	// Make sure we don't fire more than the amount in the clip
	if ( UsesClipsForAmmo1() )
	{
		info.m_iShots = min( info.m_iShots, m_iClip1 );
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = min( info.m_iShots, pPlayer->GetAmmoCount(m_iPrimaryAmmoType) );
		pPlayer->RemoveAmmo( info.m_iShots, m_iPrimaryAmmoType );
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;

	// Weapon scope system
	// Other players should see someone's tracers regardless of whether or not that individual's weapon is scoped, so let's try to do that here...
	// ^^^ Doesn't work, probably due to lag compensation or something like that...oh well... ^^^
//#ifdef CLIENT_DLL
	// Don't show tracers for weapons that are scoped, which aren't visible (that'd be dumb as the tracers would seemingly come from nowhere)
	if ( m_bIsScoped )
		info.m_iTracerFreq = 0;
	else
		info.m_iTracerFreq = 2;	// 50% (?)
/*#else
	info.m_iTracerFreq = 2;	// 50% (?)
#endif*/

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = pPlayer->GetAttackSpread( this );
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetActiveWeapon()->GetBulletSpread();
#endif // CLIENT_DLL

	pPlayer->FireBullets( info );

	if ( !m_iClip1 && (pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0) )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}

	//Add our view kick in
	AddViewKick();

	// Weapon scope system
	if ( HasScope() && UnscopeAfterShot() && (preShotAmmo > 0) )
		ExitScope();	// done after actually shooting because it is logical and should prevent any unnecessary accuracy changes
}

// Weapon scope system
bool CWeaponSOBase::CanScope( void )
{
	// If this weapon doesn't have a scope, how are we expected to use it?
	// We really shouldn't have to check for this by the time this function is called, but just in case...
	if ( !HasScope() )
		return false;

	// If this weapon requires the player to unscope after firing, wait until we're allowed to fire again before the scope can be used again
	if ( UnscopeAfterShot() && (gpGlobals->curtime < m_flNextPrimaryAttack) )
		return false;

	CSO_Player *pOwner = ToSOPlayer( GetOwner() );
	if ( !pOwner )
		return false;

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	// We should not be allowed to scope while our weapon is holstered (this takes care of conditions like climbing a ladder and sprinting)
	if ( pOwner->GetHolsteredWeapon() == this )
		return false;

	// Scoping while not on the ground is not allowed, because that'd just be silly
    if ( !(pOwner->GetFlags() & FL_ONGROUND) )
		return false;

	return true;
}

// Weapon scope system
void CWeaponSOBase::EnterScope( void )
{
	if ( !CanScope() )
		return;	// don't scope if we're not allowed to right now!

	CSO_Player *pOwner = ToSOPlayer( GetOwner() );
	if ( !pOwner )
		return;
	
	// Only scope and stuff if we have an owner
	m_bIsScoped = true;
	pOwner->SetFOV( pOwner, GetScopeFOV(), 0.1f );	// zoom
	SetWeaponVisible( false );	// hide the view model

	m_flNextSecondaryAttack	= gpGlobals->curtime + 0.25f;	// make a bit of a delay between zooming/unzooming to prevent spam and possibly some bugs
}

// Weapon scope system
void CWeaponSOBase::ExitScope( bool unhideWeapon )
{
	m_bIsScoped = false;	// unscope regardless of whether or not we have an owner (should prevent some bugs)

	CSO_Player *pOwner = ToSOPlayer( GetOwner() );
	if ( !pOwner )
		return;

	pOwner->SetFOV( pOwner, pOwner->GetDefaultFOV(), 0.1f );	// unzoom

	if ( unhideWeapon )	// there are some situations where we may not want to do this to prevent interfering with other systems
		SetWeaponVisible( true );	// show the view model again

	m_flNextSecondaryAttack	= gpGlobals->curtime + 0.25f;	// make a bit of a delay between zooming/unzooming to prevent spam and possibly some bugs
}

void CWeaponSOBase::SecondaryAttack( void )
{
	// Weapon scope system
	// Overrides secondary attack of weapons with scopes
	// This may not be desired, but that's just how it works, so adjust all scoped weapons accordingly and keep this in mind!
	if ( HasScope() )
	{
		// Toggle scope
		if ( m_bIsScoped )
			ExitScope();
		else
			EnterScope();
	}
	else
	{
		BaseClass::SecondaryAttack();
	}
}

// Weapon accuracy system
float CWeaponSOBase::GetAccuracyModifier()
{
	float weaponAccuracy = 1.0f; // by default, don't make any alterations

	CSO_Player *pPlayer = ToSOPlayer( GetOwner() );
	if ( pPlayer )
	{
		if( !fabs(pPlayer->GetAbsVelocity().x) && !fabs(pPlayer->GetAbsVelocity().y) )	// player isn't moving
			weaponAccuracy *= 0.75f;
		else if( !!( pPlayer->GetFlags() & FL_DUCKING ) )	// player is ducking
			weaponAccuracy *= 0.80f;
		else if( pPlayer->IsWalking() )	// player is walking
			weaponAccuracy *= 0.85f;
		// Weapon scope system
		else if ( m_bIsScoped )	// player's weapon is scoped (snipers override this because they are super-accurate when scoped)
			weaponAccuracy *= 0.90f;
	}

	return weaponAccuracy;
}

// Taken and modified from CBaseCombatWeapon::SetWeaponVisible
void CWeaponSOBase::SetWeaponVisible( bool visible )
{
	CBaseViewModel *vm = NULL;

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner )
		vm = pOwner->GetViewModel( m_nViewModelIndex );

	if ( visible )
	{
		// Fix a weapon disappearing bug
		// Only having the client do this should fix an issue that makes players' weapons disappear to other players
		// In a majority of cases, this is not desired (I actually can't think of a single one where it would be)
#ifdef CLIENT_DLL
		RemoveEffects( EF_NODRAW );
#endif
		if ( vm )
			vm->RemoveEffects( EF_NODRAW );
	}
	else
	{
		// Fix a weapon disappearing bug
		// Only having the client do this should fix an issue that makes players' weapons disappear to other players
		// In a majority of cases, this is not desired (I actually can't think of a single one where it would be)
#ifdef CLIENT_DLL
		AddEffects( EF_NODRAW );
#endif
		if ( vm )
			vm->AddEffects( EF_NODRAW );
	}
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

// Weapon reorigin system
// http://developer.valvesoftware.com/wiki/Adding_Ironsights
// Modified a bit from the wiki version considering our system's purpose
Vector CWeaponSOBase::GetIronsightPositionOffset( void ) const
{
	if( viewmodel_adjust_enabled.GetBool() )
		return Vector( viewmodel_adjust_forward.GetFloat(), viewmodel_adjust_right.GetFloat(), viewmodel_adjust_up.GetFloat() );
	return GetHL2MPWpnData().vecIronsightPosOffset;
}
QAngle CWeaponSOBase::GetIronsightAngleOffset( void ) const
{
	if( viewmodel_adjust_enabled.GetBool() )
		return QAngle( viewmodel_adjust_pitch.GetFloat(), viewmodel_adjust_yaw.GetFloat(), viewmodel_adjust_roll.GetFloat() );
	return GetHL2MPWpnData().angIronsightAngOffset;
}
float CWeaponSOBase::GetIronsightFOVOffset( void ) const
{
	if( viewmodel_adjust_enabled.GetBool() )
		return viewmodel_adjust_fov.GetFloat();
	return GetHL2MPWpnData().flIronsightFOVOffset;
}

// Weapon reorigin system
// http://developer.valvesoftware.com/wiki/Adding_Ironsights
// Modified a bit from the wiki version considering our system's purpose
void vm_adjust_enable_callback( IConVar *pConVar, char const *pOldString, float flOldValue )
{
	ConVarRef sv_cheats( "sv_cheats" );
	if( !sv_cheats.IsValid() || sv_cheats.GetBool() )
		return;
 
	ConVarRef var( pConVar );
 
	if( var.GetBool() )
		var.SetValue( "0" );
}
void vm_adjust_fov_callback( IConVar *pConVar, char const *pOldString, float flOldValue )
{
	if( !viewmodel_adjust_enabled.GetBool() )
		return;
 
	ConVarRef var( pConVar );
 
	CBasePlayer *pPlayer = 
#ifdef GAME_DLL
		UTIL_GetLocalPlayer();
#else
		C_BasePlayer::GetLocalPlayer();
#endif
	if( !pPlayer )
		return;
 
	if( !pPlayer->SetFOV( pPlayer, pPlayer->GetDefaultFOV()+var.GetFloat(), 0.1f ) )
	{
		Warning( "Could not set FOV\n" );
		var.SetValue( "0" );
	}
}

// Weapon respawn fix
// http://developer.valvesoftware.com/wiki/Weapon_Respawn_Fix
void CWeaponSOBase::FallInit( void )
{
#ifndef CLIENT_DLL
	SetModel( GetWorldModel() );
	VPhysicsDestroyObject();

	if ( HasSpawnFlags(SF_NORESPAWN) == false )
	{
		SetMoveType( MOVETYPE_NONE );
		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_TRIGGER );

		UTIL_DropToFloor( this, MASK_SOLID );
	}
	else
	{
		if ( !VPhysicsInitNormal(SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false) )
		{
			SetMoveType( MOVETYPE_NONE );
			SetSolid( SOLID_BBOX );
			AddSolidFlags( FSOLID_TRIGGER );
		}
		else
		{
			// Constrained start?
			if ( HasSpawnFlags(SF_WEAPON_START_CONSTRAINED) )
			{
				//Constrain the weapon in place
				IPhysicsObject *pReferenceObject, *pAttachedObject;
				
				pReferenceObject = g_PhysWorldObject;
				pAttachedObject = VPhysicsGetObject();

				if ( pReferenceObject && pAttachedObject )
				{
					constraint_fixedparams_t fixed;
					fixed.Defaults();
					fixed.InitWithCurrentObjectState( pReferenceObject, pAttachedObject );
					
					fixed.constraint.forceLimit	= lbs2kg( 10000 );
					fixed.constraint.torqueLimit = lbs2kg( 10000 );

					IPhysicsConstraint *pConstraint = GetConstraint();
					pConstraint = physenv->CreateFixedConstraint( pReferenceObject, pAttachedObject, NULL, fixed );
					pConstraint->SetGameData( (void *) this );
				}
			}
		}
	}

	SetPickupTouch();
	
	SetThink( &CWeaponSOBase::FallThink );

	SetNextThink( gpGlobals->curtime + 0.1f );
#endif	// !CLIENT_DLL
}

// Weapon respawn fix
// http://developer.valvesoftware.com/wiki/Weapon_Respawn_Fix
#ifdef GAME_DLL
void CWeaponSOBase::FallThink(void)
{
	// Prevent the common HL2DM weapon respawn bug from happening
	// When a weapon is spawned, the following chain of events occurs:
	// - Spawn() is called (duh), which then calls FallInit()
	// - FallInit() is called, and prepares the weapon's 'Think' function (CBaseCombatWeapon::FallThink())
	// - FallThink() is called, and performs several checks before deciding whether the weapon should Materialize()
	// - Materialize() is called (the HL2DM version above), which sets the weapon's respawn location.
	// The problem occurs when a weapon isn't placed properly by a level designer.
	// If the weapon is unable to move from its location (e.g. if its bounding box is halfway inside a wall), Materialize() never gets called.
	// Since Materialize() never gets called, the weapon's respawn location is never set, so if a person picks it up, it respawns forever at
	// 0 0 0 on the map (infinite loop of fall, wait, respawn, not nice at all for performance and bandwidth!)
	
	if ( HasSpawnFlags(SF_NORESPAWN) == false )
	{
		if ( GetOriginalSpawnOrigin() == vec3_origin )
		{
			m_vOriginalSpawnOrigin = GetAbsOrigin();
			m_vOriginalSpawnAngles = GetAbsAngles();
		}
	}

	return BaseClass::FallThink();
}
#endif // GAME_DLL

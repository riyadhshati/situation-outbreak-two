#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_so_player.h"
	#include "c_te_effect_dispatch.h"
#else
	#include "so_player.h"
	#include "te_effect_dispatch.h"
	#include "so_fraggrenade.h"
#endif

#include "weapon_ar2.h"
#include "effect_dispatch_data.h"
#include "weapon_fraggrenade.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GRENADE_TIMER 3.5f	// in seconds	// realistic timer is 4 seconds, but I subtracted half a second to account for the pin pulling (priming) animation not ending immediately after the pin is pulled (primed)
#define GRENADE_RADIUS 4.0f	// in inches
#define GRENADE_DAMAGE_RADIUS 590.1f	// in inches (?), about 15 meters
#define RETHROW_DELAY 0.5	// in seconds (?)

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFragGrenade, DT_WeaponFragGrenade )

BEGIN_NETWORK_TABLE( CWeaponFragGrenade, DT_WeaponFragGrenade )

#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bRedraw ) ),
	RecvPropBool( RECVINFO( m_bIsPrimed ) ),
	RecvPropFloat( RECVINFO( m_flCookTime ) ),
#else
	SendPropBool( SENDINFO( m_bRedraw ) ),
	SendPropBool( SENDINFO( m_bIsPrimed ) ),
	SendPropFloat( SENDINFO( m_flCookTime ) ),
#endif
	
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponFragGrenade )
	DEFINE_PRED_FIELD( m_bRedraw, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bIsPrimed, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flCookTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_fraggrenade, CWeaponFragGrenade );
PRECACHE_WEAPON_REGISTER( weapon_fraggrenade );

CWeaponFragGrenade::CWeaponFragGrenade( void ) :
	CBaseSOCombatWeapon()
{
	m_bRedraw = false;
	m_bIsPrimed = false;
	m_flCookTime = 0.0f;
}

void CWeaponFragGrenade::Precache( void )
{
	BaseClass::Precache();

#ifndef CLIENT_DLL
	UTIL_PrecacheOther( "npc_grenade_frag" );
#endif

	PrecacheScriptSound( "WeaponFragGrenade.Throw" );
}

bool CWeaponFragGrenade::Deploy( void )
{
	m_bRedraw = false;
	m_bIsPrimed = false;
	m_flCookTime = 0.0f;

	return BaseClass::Deploy();
}

bool CWeaponFragGrenade::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	m_bRedraw = false;
	m_bIsPrimed = false;
	m_flCookTime = 0.0f;

	return BaseClass::Holster( pSwitchingTo );
}

bool CWeaponFragGrenade::Reload( void )
{
	if ( (m_bRedraw) && (m_flNextPrimaryAttack <= gpGlobals->curtime) )
	{
#ifndef CLIENT_DLL
		if ( !HasAnyAmmo() )
		{
			m_bRedraw = false;
			CBasePlayer *pOwner = ToBasePlayer( GetOwner() );	// not sure if we have to get the owner before we remove the weapon or not, but just in case...
			UTIL_Remove( this );	// we're out of grenades, so strip the player of this weapon

			if ( pOwner )
				pOwner->SwitchToNextBestWeapon( this );	// switch to our next best weapon now that we're out of grenades

			return false;	// we're done, so let's get the hell out of here
		}
#endif

		//Redraw the weapon
		SendWeaponAnim( ACT_VM_DRAW );

		//Update our times
		m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
		m_flTimeWeaponIdle = gpGlobals->curtime + SequenceDuration();

		m_bRedraw = false;
	}

	return true;
}

void CWeaponFragGrenade::PrimaryAttack( void )
{
	if ( m_bRedraw || m_bIsPrimed )
		return;

	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	// Note that this is a primary attack and prepare the grenade attack to pause.
	SendWeaponAnim( ACT_VM_PULLBACK_HIGH );
	m_flCookTime = gpGlobals->curtime + SequenceDuration() + GRENADE_TIMER;
	m_bIsPrimed = true;
	
	// Put both of these off indefinitely. We do not know how long
	// the player will hold the grenade.
	m_flTimeWeaponIdle = FLT_MAX;
	m_flNextPrimaryAttack = FLT_MAX;
}

void CWeaponFragGrenade::ItemPostFrame( void )
{
	if ( m_bRedraw )
	{
		if ( IsViewModelSequenceFinished() )
			Reload();
	}
	else if( m_bIsPrimed )
	{
		if ( IsViewModelSequenceFinished() )
		{
			CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
			if ( pOwner )
			{
				if ( !(pOwner->m_nButtons & IN_ATTACK) || (gpGlobals->curtime >= m_flCookTime) )
				{
					SendWeaponAnim( ACT_VM_THROW );
					ToSOPlayer(pOwner)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_GRENADE );
				}
			}
		}
	}

	BaseClass::ItemPostFrame();
}

#ifndef CLIENT_DLL
void CWeaponFragGrenade::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	if ( pEvent->event == EVENT_WEAPON_THROW )
	{
		if ( gpGlobals->curtime >= m_flCookTime )
			ThrowGrenade( pOwner, 0.0f );	// our owner has held us too long! she's gonna blow!
		else
			ThrowGrenade( pOwner, m_flCookTime - gpGlobals->curtime );	// set the grenade timer to the amount of time that has elapsed since priming the grenade

		m_flNextPrimaryAttack = gpGlobals->curtime + RETHROW_DELAY;
		m_flTimeWeaponIdle = FLT_MAX; //NOTE: This is set once the animation has finished up!
		m_bIsPrimed = false;
		m_flCookTime = 0.0f;
	}
	else
	{
		BaseClass::Operator_HandleAnimEvent( pEvent, pOperator );
	}
}
#endif

// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
void CWeaponFragGrenade::CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc )
{
	trace_t tr;

	UTIL_TraceHull( vecEye, vecSrc, -Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), Vector(GRENADE_RADIUS+2,GRENADE_RADIUS+2,GRENADE_RADIUS+2), 
		pPlayer->PhysicsSolidMaskForEntity(), pPlayer, pPlayer->GetCollisionGroup(), &tr );
	
	if ( tr.DidHit() )
		vecSrc = tr.endpos;
}

void CWeaponFragGrenade::ThrowGrenade( CBasePlayer *pPlayer, float cookTimeRemaining )
{
#ifndef CLIENT_DLL
	Vector vecEye = pPlayer->EyePosition();
	Vector vForward, vRight;

	pPlayer->EyeVectors( &vForward, &vRight, NULL );
	Vector vecSrc = vecEye + vForward * 18.0f + vRight * 8.0f;
	CheckThrowPosition( pPlayer, vecEye, vecSrc );
//	vForward[0] += 0.1f;
	vForward[2] += 0.1f;

	Vector vecThrow;
	pPlayer->GetVelocity( &vecThrow, NULL );
	vecThrow += vForward * 1200;

	CBaseGrenade *pGrenade = SO_Fraggrenade_Create( vecSrc, vec3_angle, vecThrow, AngularImpulse(600,random->RandomInt(-1200,1200),0), pPlayer, cookTimeRemaining, false );
	if ( pGrenade )
	{
		if ( pPlayer && (pPlayer->m_lifeState != LIFE_ALIVE) )
		{
			pPlayer->GetVelocity( &vecThrow, NULL );

			IPhysicsObject *pPhysicsObject = pGrenade->VPhysicsGetObject();
			if ( pPhysicsObject )
				pPhysicsObject->SetVelocity( &vecThrow, NULL );
		}
		
		pGrenade->SetDamage( GetHL2MPWpnData().m_iPlayerDamage );
		pGrenade->SetDamageRadius( GRENADE_DAMAGE_RADIUS );
	}
#endif

	DecrementAmmo();
	m_bRedraw = true;

	WeaponSound( SINGLE );
	
	// player "shoot" animation
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
}

void CWeaponFragGrenade::DecrementAmmo( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !pOwner )
		return;

	pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
}

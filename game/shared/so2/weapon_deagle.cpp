
//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_so_player.h"
#else
	#include "so_player.h"
#endif

#include "weapon_sobasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponDeagle C_WeaponDeagle
#endif

//-----------------------------------------------------------------------------
// CWeaponDeagle
//-----------------------------------------------------------------------------

class CWeaponDeagle : public CBaseSOCombatWeapon
{
	DECLARE_CLASS( CWeaponDeagle, CBaseSOCombatWeapon );
public:

	CWeaponDeagle( void );

	void	PrimaryAttack( void );
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "Pistol"; }

private:
	
	CWeaponDeagle( const CWeaponDeagle & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponDeagle, DT_WeaponDeagle )

BEGIN_NETWORK_TABLE( CWeaponDeagle, DT_WeaponDeagle )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponDeagle )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_deagle, CWeaponDeagle );
PRECACHE_WEAPON_REGISTER( weapon_deagle );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponDeagle::CWeaponDeagle( void )
{
	m_bReloadsSingly	= false;
	m_bFiresUnderwater	= false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CWeaponDeagle::PrimaryAttack( void )
{
	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( !pPlayer )
	{
		return;
	}

	if ( m_iClip1 <= 0 )
	{
		if ( !m_bFireOnEmpty )
		{
			Reload();
		}
		else
		{
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

	WeaponSound( SINGLE );
	pPlayer->DoMuzzleFlash();

	SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	pPlayer->SetAnimation( PLAYER_ATTACK1 );
	ToHL2MPPlayer(pPlayer)->DoAnimationEvent( PLAYERANIMEVENT_ATTACK_PRIMARY );

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.75;
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.75;

	m_iClip1--;

	Vector vecSrc		= pPlayer->Weapon_ShootPosition();
	Vector vecAiming	= pPlayer->GetAutoaimVector( AUTOAIM_5DEGREES );	

	FireBulletsInfo_t info( 1, vecSrc, vecAiming, vec3_origin, MAX_TRACE_LENGTH, m_iPrimaryAmmoType );
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accuracy
	pPlayer->FireBullets( info );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();

	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

#ifndef CLIENT_DLL
	pPlayer->SnapEyeAngles( angles );
#endif

	pPlayer->ViewPunch( QAngle( -8, random->RandomFloat( -2, 2 ), 0 ) );

	if ( !m_iClip1 && pPlayer->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 )
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate( "!HEV_AMO0", FALSE, 0 ); 
	}
}

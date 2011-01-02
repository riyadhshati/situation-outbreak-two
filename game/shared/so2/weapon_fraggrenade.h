#ifndef WEAPON_FRAG_GRENADE_H
#define WEAPON_FRAG_GRENADE_H

#include "weapon_sobasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponFragGrenade C_WeaponFragGrenade
#endif

class CWeaponFragGrenade: public CBaseSOCombatWeapon
{
	DECLARE_CLASS( CWeaponFragGrenade, CBaseSOCombatWeapon );

public:
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponFragGrenade();

	void Precache( void );
	void PrimaryAttack( void );
	void SecondaryAttack( void ) { return; }
	void ItemPostFrame( void );

	bool Deploy( void );
	bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );
	
	bool Reload( void );

#ifndef CLIENT_DLL
	void Operator_HandleAnimEvent( animevent_t *pEvent, CBaseCombatCharacter *pOperator );
#endif

	void ThrowGrenade( CBasePlayer *pPlayer, float cookTimeRemaining );
	void DecrementAmmo( void );

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "GREN"; }
	bool IsPrimed( void ) { return m_bIsPrimed; }

	// Allowing players to drop grenades causes a lot of problems, so let's prevent them from doing so (at least for now)
	// Allow players to drop their active weapons
	// Only drop weapons we're allowed to drop
	bool CanDrop( void ) { return false; }
	
private:
	CWeaponFragGrenade( const CWeaponFragGrenade & );

	// check a throw from vecSrc.  If not valid, move the position back along the line to vecEye
	void CheckThrowPosition( CBasePlayer *pPlayer, const Vector &vecEye, Vector &vecSrc );

	CNetworkVar( bool, m_bRedraw );	//Draw the weapon again after throwing a grenade
	CNetworkVar( bool, m_bIsPrimed );
	CNetworkVar( float, m_flCookTime );
};

#endif	// WEAPON_FRAG_GRENADE_H

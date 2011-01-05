// Originally implemented by Stephen 'SteveUK' Swires for SO
// It has since been modified to support SO2
// Thanks Steve =)

#ifndef SO_WEAPON_BASE_SNIPER_H
#define SO_WEAPON_BASE_SNIPER_H

#include "cbase.h"
#include "weapon_sobase.h"

#ifdef CLIENT_DLL
#define CWeaponSOBaseSniper C_WeaponSOBaseSniper
#endif

class CWeaponSOBaseSniper : public CWeaponSOBase
{
	DECLARE_CLASS( CWeaponSOBaseSniper, CWeaponSOBase );

public:
	CWeaponSOBaseSniper( void );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	virtual void PrimaryAttack( void );
	virtual void ItemPostFrame( void );

	virtual float GetAccuracyModifier( void );
	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_5DEGREES;	// unscoped snipers are not very accurate (actually, 5 degrees is probably pretty generous too)
		return cone;
	}

	virtual bool Deploy( void );
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL );

	// Weapon scope system
	virtual bool HasScope( void ) { return true; }	// all snipers have a scope...right?
	virtual bool UnscopeAfterShot( void ) { return true; } // by default, unscope after shooting since most of our snipers (all at the time of typing this) are bolt-action

	// bolt action snipers
	virtual bool IsBoltAction( void ) { return false; }
	virtual bool IsCocking( void ) { return m_bIsCocking; }
	virtual void CockBolt( void );
	virtual void FinishCocking( void );

private:
	CNetworkVar( bool, m_bNeedsCocking );
	CNetworkVar( bool, m_bIsCocking );
	CNetworkVar( float, m_flEndCockTime );

	CWeaponSOBaseSniper( const CWeaponSOBaseSniper & );
};

#endif	// SO_WEAPON_BASE_SNIPER_H

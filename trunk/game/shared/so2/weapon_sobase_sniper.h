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
	virtual float GetAccuracyModifier( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;

		if ( m_bIsScoped )
			cone = Vector( 0, 0, 0 );	// do not take bullet spread into account when scoped
		else
			cone = VECTOR_CONE_10DEGREES;	// unscoped snipers are not at all accurate

		return cone;
	}

	// Weapon scope system
	virtual bool HasScope( void ) { return true; }	// all snipers have a scope...right?
	virtual bool UnscopeAfterShot( void ) { return true; } // by default, unscope after shooting since most of our snipers (all at the time of typing this) are bolt-action
	virtual float GetScopeFOV( void ) { return 22.5f; }	// zoomy zoomy! this is a sniper after all!

	virtual bool ShouldDrawCrosshair( void ) { return false; }	// snipers don't have crosshairs for aiming, only scopes

private:
	CWeaponSOBaseSniper( const CWeaponSOBaseSniper & );
};

#endif	// SO_WEAPON_BASE_SNIPER_H

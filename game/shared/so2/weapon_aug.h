//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Projectile shot from the AR2 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef	WEAPONAR2_H
#define	WEAPONAR2_H

#include "weapon_sobase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponAUG C_WeaponAUG
#endif

class CWeaponAUG : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponAUG, CSOMachineGun );

	CWeaponAUG();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	ItemPostFrame( void );

	void	AddViewKick( void );

	int		GetMinBurst( void ) { return 2; }
	int		GetMaxBurst( void ) { return 5; }
	float	GetFireRate( void ) { return 0.1f; }

	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		
		cone = VECTOR_CONE_3DEGREES;

		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "AUG"; }

private:
	CWeaponAUG( const CWeaponAUG & );

protected:

	int						m_nVentPose;
};


#endif	//WEAPONAR2_H

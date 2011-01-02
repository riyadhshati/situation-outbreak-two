#ifndef WEAPON_BASESOCOMBATWEAPON_SHARED_H
#define WEAPON_BASESOCOMBATWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "c_so_player.h"
#else
	#include "so_player.h"
#endif

#include "weapon_sobase.h"

#if defined( CLIENT_DLL )
#define CBaseSOCombatWeapon C_BaseSOCombatWeapon
#endif

class CBaseSOCombatWeapon : public CWeaponSOBase
{
#if !defined( CLIENT_DLL )
	DECLARE_DATADESC();
#endif

	DECLARE_CLASS( CBaseSOCombatWeapon, CWeaponSOBase );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

	CBaseSOCombatWeapon();

	virtual bool	WeaponShouldBeLowered( void );

	virtual bool	Ready( void );
	virtual bool	Lower( void );
	virtual bool	Deploy( void );
	virtual void	WeaponIdle( void );

	virtual Vector	GetBulletSpread( WeaponProficiency_t proficiency );
	virtual float	GetSpreadBias( WeaponProficiency_t proficiency );

	virtual const	WeaponProficiencyInfo_t *GetProficiencyValues();
	static const	WeaponProficiencyInfo_t *GetDefaultProficiencyValues();

protected:

	bool			m_bLowered;			// Whether the viewmodel is raised or lowered
	float			m_flRaiseTime;		// If lowered, the time we should raise the viewmodel

private:
	
	CBaseSOCombatWeapon( const CBaseSOCombatWeapon & );
};

#endif // WEAPON_BASESOCOMBATWEAPON_SHARED_H

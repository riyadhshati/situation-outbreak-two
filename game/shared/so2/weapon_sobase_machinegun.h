//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "weapon_sobase.h"

#ifndef BASESOCOMBATWEAPON_H
#define BASESOCOMBATWEAPON_H
#ifdef _WIN32
#pragma once
#endif

#if defined( CLIENT_DLL )
	#define CSOMachineGun C_SOMachineGun
#endif

//=========================================================
// Machine gun base class
//=========================================================
class CSOMachineGun : public CWeaponSOBase
{
public:
	DECLARE_CLASS( CSOMachineGun, CWeaponSOBase );
	DECLARE_DATADESC();

	CSOMachineGun();
	
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	PrimaryAttack( void );

	// Default calls through to m_hOwner, but plasma weapons can override and shoot projectiles here.
	virtual void	ItemPostFrame( void );
	virtual void	FireBullets( const FireBulletsInfo_t &info );
	virtual bool	Deploy( void );

	virtual const Vector &GetBulletSpread( void );

	int				WeaponSoundRealtime( WeaponSound_t shoot_type );

	// utility function
	static void DoMachineGunKick( CBasePlayer *pPlayer, float dampEasy, float maxVerticleKickAngle, float fireDurationTime, float slideLimitTime );

private:
	
	CSOMachineGun( const CSOMachineGun & );

protected:

	int	m_nShotsFired;	// Number of consecutive shots fired

	float	m_flNextSoundTime;	// real-time clock of when to make next sound
};

#endif // BASESOCOMBATWEAPON_H

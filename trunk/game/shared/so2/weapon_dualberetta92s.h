#ifndef WEAPON_DUAL_BERETTA_92S_H
#define WEAPON_DUAL_BERETTA_92S_H

#include "weapon_sobase_machinegun.h"

#define	PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME	3.0f	// Maximum penalty to deal out	// doubled from 1.5f

#ifdef CLIENT_DLL
#define CWeaponDualBeretta92s C_WeaponDualBeretta92s
#endif

class CWeaponDualBeretta92s : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponDualBeretta92s, CSOMachineGun );

	CWeaponDualBeretta92s(void);

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	Precache( void );
	void	ItemPostFrame( void );
	void	ItemPreFrame( void );
	void	ItemBusyFrame( void );
	void	PrimaryAttack( void );
	void	DoAttack( void );
	void	AddViewKick( void );

	int		GetTracerAttachment( void );

	void	UpdatePenaltyTime( void );

	Activity	GetPrimaryAttackActivity( void );

	virtual bool Reload( void );

	virtual const Vector& GetBulletSpread( void )
	{		
		static Vector cone;

		float ramp = RemapValClamped(	m_flAccuracyPenalty, 
											0.0f, 
											PISTOL_ACCURACY_MAXIMUM_PENALTY_TIME, 
											0.0f, 
											1.0f ); 

		// We lerp from very accurate to inaccurate over time
		VectorLerp( VECTOR_CONE_2DEGREES, VECTOR_CONE_8DEGREES, ramp, cone );	// increased (was VECTOR_CONE_1DEGREES and VECTOR_CONE_6DEGREES)

		return cone;
	}
	
	virtual int	GetMinBurst() 
	{ 
		return 1; 
	}

	virtual int	GetMaxBurst() 
	{ 
		return 1; 
	}

	virtual float GetFireRate( void ) 
	{
		return 0.5f; 
	}

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "ELITES"; }

	// Fix dual Beretta 92s muzzleflash issue
	bool ShouldFlip( void ) { return m_bFlip; }

private:
	CNetworkVar( float,	m_flSoonestPrimaryAttack );
	CNetworkVar( float,	m_flLastAttackTime );
	CNetworkVar( float,	m_flAccuracyPenalty );
	CNetworkVar( int,	m_nNumShotsFired );
	CNetworkVar( bool,	m_bFlip );

private:
	CWeaponDualBeretta92s( const CWeaponDualBeretta92s & );
};

#endif	// WEAPON_DUAL_BERETTA_92S_H

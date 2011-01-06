#include "cbase.h"
#include "weapon_sobase_sniper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define AWM_KICKBACK 10	// Range for punchangle when firing.

#ifdef CLIENT_DLL
#define CWeaponAWM C_WeaponAWM
#endif

class CWeaponAWM : public CWeaponSOBaseSniper
{
	DECLARE_CLASS( CWeaponAWM, CWeaponSOBaseSniper );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	CWeaponAWM( void );
	
	void AddViewKick( void );

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "AWP"; }

private:
	CWeaponAWM( const CWeaponAWM & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAWM, DT_WeaponAWM )

BEGIN_NETWORK_TABLE( CWeaponAWM, DT_WeaponAWM )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAWM )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_awm, CWeaponAWM );
PRECACHE_WEAPON_REGISTER( weapon_awm );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponAWM::CWeaponAWM( void )
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 43307;	// in inches; about 1,100 meters
}

void CWeaponAWM::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if( !pPlayer )
		return;

	QAngle vecPunch( random->RandomFloat(-AWM_KICKBACK, -AWM_KICKBACK), 0, 0 );
	pPlayer->ViewPunch( vecPunch );

	//Disorient the player
	QAngle angles = pPlayer->GetLocalAngles();
	angles.x += random->RandomInt( -1, 1 );
	angles.y += random->RandomInt( -1, 1 );
	angles.z = 0;

#ifndef CLIENT_DLL
	pPlayer->SnapEyeAngles( angles );
#endif
}

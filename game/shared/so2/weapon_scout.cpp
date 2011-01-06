#include "cbase.h"
#include "weapon_sobase_sniper.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SCOUT_KICKBACK 7.5	// Range for punchangle when firing.

#ifdef CLIENT_DLL
#define CWeaponScout C_WeaponScout
#endif

class CWeaponScout : public CWeaponSOBaseSniper
{
	DECLARE_CLASS( CWeaponScout, CWeaponSOBaseSniper );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

public:
	CWeaponScout( void );
	
	void AddViewKick( void );

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "SCOUT"; }

private:
	CWeaponScout( const CWeaponScout & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponScout, DT_WeaponScout )

BEGIN_NETWORK_TABLE( CWeaponScout, DT_WeaponScout )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponScout )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_scout, CWeaponScout );
PRECACHE_WEAPON_REGISTER( weapon_scout );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CWeaponScout::CWeaponScout( void )
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 31496;	// in inches; about 800 meters
}

void CWeaponScout::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if( !pPlayer )
		return;

	QAngle vecPunch( random->RandomFloat(-SCOUT_KICKBACK, -SCOUT_KICKBACK), 0, 0 );
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

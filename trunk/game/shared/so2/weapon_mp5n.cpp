#include "cbase.h"
#include "weapon_sobase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponMP5N C_WeaponMP5N
#endif

class CWeaponMP5N : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponMP5N, CSOMachineGun );

	CWeaponMP5N();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void AddViewKick( void );

	int GetMinBurst( void ) { return 1; }
	int GetMaxBurst( void ) { return 1; }
	float GetFireRate( void ) { return 0.075f; }	// about 13.3Hz

	Activity GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_4DEGREES;
		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "MP5"; }

private:
	CWeaponMP5N( const CWeaponMP5N & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponMP5N, DT_WeaponMP5N )

BEGIN_NETWORK_TABLE( CWeaponMP5N, DT_WeaponMP5N )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponMP5N )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_mp5n, CWeaponMP5N );
PRECACHE_WEAPON_REGISTER( weapon_mp5n );

CWeaponMP5N::CWeaponMP5N()
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 7874;	// in inches; about 200 meters
}

Activity CWeaponMP5N::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponMP5N::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	#define	EASY_DAMPEN 0.5f
	#define	MAX_VERTICAL_KICK 2.0f	// in degrees
	#define	SLIDE_LIMIT 2.0f	// in seconds

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

const WeaponProficiencyInfo_t *CWeaponMP5N::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 10.0/3.0, 0.75	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}

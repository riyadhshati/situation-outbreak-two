#include "cbase.h"
#include "weapon_sobase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponFAMAS C_WeaponFAMAS
#endif

class CWeaponFAMAS : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponFAMAS, CSOMachineGun );

	CWeaponFAMAS();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void AddViewKick( void );

	int GetMinBurst( void ) { return 1; }
	int GetMaxBurst( void ) { return 1; }
	float GetFireRate( void ) { return 0.06f; }	// 16.666 (repeating, of course) Hz

	Activity GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_4DEGREES;
		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "FAMAS"; }

private:
	CWeaponFAMAS( const CWeaponFAMAS & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponFAMAS, DT_WeaponFAMAS )

BEGIN_NETWORK_TABLE( CWeaponFAMAS, DT_WeaponFAMAS )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponFAMAS )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_famas, CWeaponFAMAS );
PRECACHE_WEAPON_REGISTER( weapon_famas );

CWeaponFAMAS::CWeaponFAMAS()
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 11811;	// in inches; about 300 meters
}

Activity CWeaponFAMAS::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponFAMAS::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	#define	EASY_DAMPEN 0.5f
	#define	MAX_VERTICAL_KICK 8.0f	// in degrees
	#define	SLIDE_LIMIT 2.0f	// in seconds

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

const WeaponProficiencyInfo_t *CWeaponFAMAS::GetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
	{
		{ 7.0,		0.75	},
		{ 5.00,		0.75	},
		{ 3.0,		0.85	},
		{ 5.0/3.0,	0.75	},
		{ 1.00,		1.0		},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1 );

	return proficiencyTable;
}

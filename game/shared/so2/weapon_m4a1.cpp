#include "cbase.h"
#include "weapon_sobase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponM4A1 C_WeaponM4A1
#endif

class CWeaponM4A1 : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponM4A1, CSOMachineGun );

	CWeaponM4A1();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void AddViewKick( void );

	int GetMinBurst( void ) { return 1; }
	int GetMaxBurst( void ) { return 1; }
	float GetFireRate( void ) { return 0.075f; }	// about 13.333 (repeating, of course) Hz

	Activity GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_4DEGREES;
		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "M4"; }

private:
	CWeaponM4A1( const CWeaponM4A1 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponM4A1, DT_WeaponM4A1 )

BEGIN_NETWORK_TABLE( CWeaponM4A1, DT_WeaponM4A1 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponM4A1 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_m4a1, CWeaponM4A1 );
PRECACHE_WEAPON_REGISTER( weapon_m4a1 );

CWeaponM4A1::CWeaponM4A1()
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 23622;	// in inches; about 600 meters
}

Activity CWeaponM4A1::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponM4A1::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	#define	EASY_DAMPEN 0.5f
	#define	MAX_VERTICAL_KICK 8.0f	// in degrees
	#define	SLIDE_LIMIT 3.0f	// in seconds

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

const WeaponProficiencyInfo_t *CWeaponM4A1::GetProficiencyValues()
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

#include "cbase.h"
#include "weapon_sobase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

	void AddViewKick( void );

	int GetMinBurst( void ) { return 1; }
	int GetMaxBurst( void ) { return 1; }
	float GetFireRate( void ) { return 0.086f; }	// about 11.666 (repeating, of course) Hz

	Activity GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_3DEGREES;
		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "AUG"; }

	// Weapon scope system
	virtual bool HasScope( void ) { return true; }	// this weapon has a scope

private:
	CWeaponAUG( const CWeaponAUG & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAUG, DT_WeaponAUG )

BEGIN_NETWORK_TABLE( CWeaponAUG, DT_WeaponAUG )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAUG )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_aug, CWeaponAUG );
PRECACHE_WEAPON_REGISTER( weapon_aug );

CWeaponAUG::CWeaponAUG()
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 19685;	// in inches; about 500 meters
}

Activity CWeaponAUG::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponAUG::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	#define	EASY_DAMPEN 0.5f
	#define	MAX_VERTICAL_KICK 8.0f	// in degrees
	#define	SLIDE_LIMIT 5.0f	// in seconds

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

const WeaponProficiencyInfo_t *CWeaponAUG::GetProficiencyValues()
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

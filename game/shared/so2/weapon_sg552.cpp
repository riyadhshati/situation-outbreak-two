#include "cbase.h"
#include "weapon_sobase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponSG552 C_WeaponSG552
#endif

class CWeaponSG552 : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponSG552, CSOMachineGun );

	CWeaponSG552();

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
	const char *GetWeaponSuffix( void ) { return "SG552"; }

	// Weapon scope system
	virtual bool HasScope( void ) { return true; }	// this weapon has a scope

private:
	CWeaponSG552( const CWeaponSG552 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSG552, DT_WeaponSG552 )

BEGIN_NETWORK_TABLE( CWeaponSG552, DT_WeaponSG552 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSG552 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_sg552, CWeaponSG552 );
PRECACHE_WEAPON_REGISTER( weapon_sg552 );

CWeaponSG552::CWeaponSG552()
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 15748;	// in inches; about 400 meters
}

Activity CWeaponSG552::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponSG552::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	#define	EASY_DAMPEN 0.5f
	#define	MAX_VERTICAL_KICK 5.0f	// in degrees
	#define	SLIDE_LIMIT 2.0f	// in seconds

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

const WeaponProficiencyInfo_t *CWeaponSG552::GetProficiencyValues()
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

#include "cbase.h"
#include "weapon_sobase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponGalil C_WeaponGalil
#endif

class CWeaponGalil : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponGalil, CSOMachineGun );

	CWeaponGalil();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void AddViewKick( void );

	int GetMinBurst( void ) { return 1; }
	int GetMaxBurst( void ) { return 1; }
	float GetFireRate( void ) { return 0.08f; }	// 12.5Hz

	Activity GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_4DEGREES;
		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "GALIL"; }

private:
	CWeaponGalil( const CWeaponGalil & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponGalil, DT_WeaponGalil )

BEGIN_NETWORK_TABLE( CWeaponGalil, DT_WeaponGalil )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponGalil )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_galil, CWeaponGalil );
PRECACHE_WEAPON_REGISTER( weapon_galil );

CWeaponGalil::CWeaponGalil()
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 19685;	// in inches; about 500 meters
}

Activity CWeaponGalil::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponGalil::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	#define	EASY_DAMPEN 0.5f
	#define	MAX_VERTICAL_KICK 8.0f	// in degrees
	#define	SLIDE_LIMIT 3.0f	// in seconds

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

const WeaponProficiencyInfo_t *CWeaponGalil::GetProficiencyValues()
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

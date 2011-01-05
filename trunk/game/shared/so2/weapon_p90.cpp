#include "cbase.h"
#include "weapon_sobase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponP90 C_WeaponP90
#endif

class CWeaponP90 : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponP90, CSOMachineGun );

	CWeaponP90();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void AddViewKick( void );

	int GetMinBurst( void ) { return 1; }
	int GetMaxBurst( void ) { return 1; }
	float GetFireRate( void ) { return 0.067f; }	// about 15Hz

	Activity GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static Vector cone;
		cone = VECTOR_CONE_5DEGREES;
		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "P90"; }

	// Weapon scope system
	// TODO: Make a red dot scope specifically for the P90...maybe...=P
	//virtual bool HasScope( void ) { return true; }	// this weapon has a scope

private:
	CWeaponP90( const CWeaponP90 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponP90, DT_WeaponP90 )

BEGIN_NETWORK_TABLE( CWeaponP90, DT_WeaponP90 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponP90 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_p90, CWeaponP90 );
PRECACHE_WEAPON_REGISTER( weapon_p90 );

CWeaponP90::CWeaponP90()
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 7874;	// in inches; about 200 meters
}

Activity CWeaponP90::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponP90::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	#define	EASY_DAMPEN 0.5f
	#define	MAX_VERTICAL_KICK 2.0f	// in degrees
	#define	SLIDE_LIMIT 1.0f	// in seconds

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

const WeaponProficiencyInfo_t *CWeaponP90::GetProficiencyValues()
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

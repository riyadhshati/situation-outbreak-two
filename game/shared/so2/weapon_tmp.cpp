#include "cbase.h"
#include "weapon_sobase_machinegun.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef CLIENT_DLL
#define CWeaponTMP C_WeaponTMP
#endif

class CWeaponTMP : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponTMP, CSOMachineGun );

	CWeaponTMP();

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
		cone = VECTOR_CONE_6DEGREES;
		return cone;
	}
	
	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "TMP"; }

	// Fix CS:S muzzleflashes
	// http://developer.valvesoftware.com/wiki/Muzzle_Flash_(CSS_Style)
	// Not mentioned in the tutorial, although it appears necessary to get other players' muzzleflashes to work
	bool ShouldDrawMuzzleFlash( void ) { return false; }	// this is supposed to be a silenced weapon, so that means no muzzleflash

private:
	CWeaponTMP( const CWeaponTMP & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponTMP, DT_WeaponTMP )

BEGIN_NETWORK_TABLE( CWeaponTMP, DT_WeaponTMP )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponTMP )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_tmp, CWeaponTMP );
PRECACHE_WEAPON_REGISTER( weapon_tmp );

CWeaponTMP::CWeaponTMP()
{
	m_fMinRange1 = 0;	// in inches; no minimum range
	m_fMaxRange1 = 3937;	// in inches; about 100 meters
}

Activity CWeaponTMP::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

void CWeaponTMP::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	#define	EASY_DAMPEN 0.5f
	#define	MAX_VERTICAL_KICK 3.0f	// in degrees
	#define	SLIDE_LIMIT 1.0f	// in seconds

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

const WeaponProficiencyInfo_t *CWeaponTMP::GetProficiencyValues()
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

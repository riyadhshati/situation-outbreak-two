#include "cbase.h"
#include "weapon_sobasebasebludgeon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	KNIFE_RANGE 45.0f
#define	KNIFE_REFIRE 0.5f

#ifdef CLIENT_DLL
#define CWeaponKnife C_WeaponKnife
#endif

class CWeaponKnife : public CBaseSOBludgeonWeapon
{
public:
	DECLARE_CLASS( CWeaponKnife, CBaseSOBludgeonWeapon );

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	CWeaponKnife();

	void Precache( void );
	float GetDamageForActivity( Activity hitActivity );
	void AddViewKick( void );

	float GetRange( void );
	float GetFireRate( void );

	CWeaponKnife( const CWeaponKnife & );

	void SecondaryAttack( void ) { return; }	// no secondary attack (at least for now)

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "KNIFE"; }

	// Allow players to drop their active weapons
	// Only drop weapons we're allowed to drop
	virtual bool CanDrop( void ) { return false; }
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponKnife, DT_WeaponKnife )

BEGIN_NETWORK_TABLE( CWeaponKnife, DT_WeaponKnife )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponKnife )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_knife, CWeaponKnife );
PRECACHE_WEAPON_REGISTER( weapon_knife );

CWeaponKnife::CWeaponKnife( void )
{
}

void CWeaponKnife::Precache( void )
{
	PrecacheScriptSound( "Weapon_Knife.Slash" );
	PrecacheScriptSound( "Weapon_Knife.Hit" );
	PrecacheScriptSound( "Weapon_Knife.HitWall" );

	BaseClass::Precache();
}

float CWeaponKnife::GetDamageForActivity( Activity hitActivity )
{
	return 75.0f;	// this appears to be an approximation to the amount of damage actually done (?)
}

void CWeaponKnife::AddViewKick( void )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );
	if ( !pPlayer )
		return;

	QAngle punchAng;

	punchAng.x = SharedRandomFloat( "knifepax", 1.0f, 2.0f );
	punchAng.y = SharedRandomFloat( "knifepay", -2.0f, -1.0f );
	punchAng.z = 0.0f;
	
	pPlayer->ViewPunch( punchAng ); 
}

float CWeaponKnife::GetRange( void )
{
	return KNIFE_RANGE;	
}

float CWeaponKnife::GetFireRate( void )
{
	return KNIFE_REFIRE;	
}

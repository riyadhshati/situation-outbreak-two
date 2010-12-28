//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_so_player.h"
#else
	#include "so_player.h"
#endif

#include "weapon_sobase.h"
#include "weapon_sobase_machinegun.h"

#ifdef CLIENT_DLL
#define CWeaponP90 C_WeaponP90
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CWeaponP90 : public CSOMachineGun
{
public:
	DECLARE_CLASS( CWeaponP90, CSOMachineGun );

	CWeaponP90();

	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

	void	AddViewKick( void );

	int		GetMinBurst() { return 2; }
	int		GetMaxBurst() { return 5; }

	virtual void Equip( CBaseCombatCharacter *pOwner );

	float	GetFireRate( void ) { return 0.075f; }	// 13.3hz
	Activity	GetPrimaryAttackActivity( void );

	virtual const Vector& GetBulletSpread( void )
	{
		static const Vector cone = VECTOR_CONE_5DEGREES;
		return cone;
	}

	const WeaponProficiencyInfo_t *GetProficiencyValues();

	// Add support for CS:S player animations
	const char *GetWeaponSuffix( void ) { return "P90"; }
	
private:
	CWeaponP90( const CWeaponP90 & );
};

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponP90, DT_WeaponP90 )

BEGIN_NETWORK_TABLE( CWeaponP90, DT_WeaponP90 )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponP90 )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_p90, CWeaponP90 );
PRECACHE_WEAPON_REGISTER(weapon_p90);


//=========================================================
CWeaponP90::CWeaponP90( )
{
	m_fMinRange1		= 0;// No minimum range. 
	m_fMaxRange1		= 1400;
}

//-----------------------------------------------------------------------------
// Purpose: Give this weapon longer range when wielded by an ally NPC.
//-----------------------------------------------------------------------------
void CWeaponP90::Equip( CBaseCombatCharacter *pOwner )
{
	m_fMaxRange1 = 1400;

	BaseClass::Equip( pOwner );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponP90::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponP90::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	1.0f	//Degrees
	#define	SLIDE_LIMIT			2.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
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

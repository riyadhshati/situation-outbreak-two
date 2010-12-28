//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "npcevent.h"

#include "weapon_aug.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
//=========================================================


IMPLEMENT_NETWORKCLASS_ALIASED( WeaponAUG, DT_WeaponAUG )

BEGIN_NETWORK_TABLE( CWeaponAUG, DT_WeaponAUG )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponAUG )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_aug, CWeaponAUG );
PRECACHE_WEAPON_REGISTER(weapon_aug);

CWeaponAUG::CWeaponAUG( )
{
	m_fMinRange1	= 65;
	m_fMaxRange1	= 2048;

	m_nShotsFired	= 0;
	m_nVentPose		= -1;
}

//-----------------------------------------------------------------------------
// Purpose: Handle grenade detonate in-air (even when no ammo is left)
//-----------------------------------------------------------------------------
void CWeaponAUG::ItemPostFrame( void )
{
	// Update our pose parameter for the vents
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner )
	{
		CBaseViewModel *pVM = pOwner->GetViewModel();

		if ( pVM )
		{
			if ( m_nVentPose == -1 )
			{
				m_nVentPose = pVM->LookupPoseParameter( "VentPoses" );
			}
			
			float flVentPose = RemapValClamped( m_nShotsFired, 0, 5, 0.0f, 1.0f );
			pVM->SetPoseParameter( m_nVentPose, flVentPose );
		}
	}

	BaseClass::ItemPostFrame();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CWeaponAUG::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponAUG::AddViewKick( void )
{
	#define	EASY_DAMPEN			0.5f
	#define	MAX_VERTICAL_KICK	8.0f	//Degrees
	#define	SLIDE_LIMIT			5.0f	//Seconds
	
	//Get the view kick
	CBasePlayer *pPlayer = ToBasePlayer( GetOwner() );

	if (!pPlayer)
		return;

	DoMachineGunKick( pPlayer, EASY_DAMPEN, MAX_VERTICAL_KICK, m_fFireDuration, SLIDE_LIMIT );
}

//-----------------------------------------------------------------------------
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

	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}

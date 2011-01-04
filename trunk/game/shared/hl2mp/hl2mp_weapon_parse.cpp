//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include <KeyValues.h>
#include "hl2mp_weapon_parse.h"
#include "ammodef.h"

FileWeaponInfo_t* CreateWeaponInfo()
{
	return new CHL2MPSWeaponInfo;
}

CHL2MPSWeaponInfo::CHL2MPSWeaponInfo()
{
	m_iPlayerDamage = 0;
}

void CHL2MPSWeaponInfo::Parse( KeyValues *pKeyValuesData, const char *szWeaponName )
{
	BaseClass::Parse( pKeyValuesData, szWeaponName );

	m_iPlayerDamage = pKeyValuesData->GetInt( "damage", 0 );

/////

	// SO2 - James
	// Weapon reorigin system
	// http://developer.valvesoftware.com/wiki/Adding_Ironsights
	// Modified a bit from the wiki version considering our system's purpose
	KeyValues *pSights = pKeyValuesData->FindKey( "Reorigin" );
	if ( pSights )
	{
		vecIronsightPosOffset.x = pSights->GetFloat( "forward", 0.0f );
		vecIronsightPosOffset.y = pSights->GetFloat( "right", 0.0f );
		vecIronsightPosOffset.z = pSights->GetFloat( "up", 0.0f );
 
		angIronsightAngOffset[PITCH] = pSights->GetFloat( "pitch", 0.0f );
		angIronsightAngOffset[YAW] = pSights->GetFloat( "yaw", 0.0f );
		angIronsightAngOffset[ROLL] = pSights->GetFloat( "roll", 0.0f );
 
		flIronsightFOVOffset = pSights->GetFloat( "fov", 0.0f );
	}
	else
	{
		//note: you can set a bool here if you'd like to disable ironsights for weapons with no IronSight-key
		vecIronsightPosOffset = vec3_origin;
		angIronsightAngOffset.Init();
		flIronsightFOVOffset = 0.0f;
	}

/////

}

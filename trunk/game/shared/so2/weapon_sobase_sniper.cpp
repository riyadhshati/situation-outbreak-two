// Originally implemented by Stephen 'SteveUK' Swires for SO
// It has since been modified to support SO2
// Thanks Steve =)

#include "cbase.h"
#include "weapon_sobase_sniper.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSOBaseSniper, DT_WeaponSOBaseSniper )

BEGIN_NETWORK_TABLE( CWeaponSOBaseSniper, DT_WeaponSOBaseSniper )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CWeaponSOBaseSniper )
END_PREDICTION_DATA()

CWeaponSOBaseSniper::CWeaponSOBaseSniper()
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;
}

void CWeaponSOBaseSniper::PrimaryAttack( void )
{
	BaseClass::PrimaryAttack();

	m_flNextPrimaryAttack = gpGlobals->curtime + SequenceDuration();
}

// make snipers extremely accurate when scoped
float CWeaponSOBaseSniper::GetAccuracyModifier()
{
	if( m_bIsScoped )
		return 0.01f;

	return BaseClass::GetAccuracyModifier();
}

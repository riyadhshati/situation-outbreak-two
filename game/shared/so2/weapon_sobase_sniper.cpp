// Originally implemented by Stephen 'SteveUK' Swires for SO
// It has since been modified to support SO2
// Thanks Steve =)

#include "cbase.h"
#include "weapon_sobase_sniper.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponSOBaseSniper, DT_WeaponSOBaseSniper )

BEGIN_NETWORK_TABLE( CWeaponSOBaseSniper, DT_WeaponSOBaseSniper )
#ifdef CLIENT_DLL
	RecvPropBool( RECVINFO( m_bNeedsCocking ) ),
	RecvPropBool( RECVINFO( m_bIsCocking ) ),
	RecvPropFloat( RECVINFO( m_flEndCockTime ) ),
#else
	SendPropBool( SENDINFO( m_bNeedsCocking ) ),
	SendPropBool( SENDINFO( m_bIsCocking ) ),
	SendPropFloat( SENDINFO( m_flEndCockTime ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
	BEGIN_PREDICTION_DATA( CWeaponSOBaseSniper )
	END_PREDICTION_DATA()
#endif

CWeaponSOBaseSniper::CWeaponSOBaseSniper()
{
	m_bIsCocking = false;
	m_bNeedsCocking = false;
	m_flEndCockTime = 0.0f;
}

bool CWeaponSOBaseSniper::Deploy( void )
{
	if ( BaseClass::Deploy() )
	{
		m_bIsCocking = false;
		m_flEndCockTime = 0.0f;

		return true;
	}

	return false;
}

bool CWeaponSOBaseSniper::Holster( CBaseCombatWeapon *pSwitchingTo )
{
	if ( m_bInReload )	// this inhibits holstering, so it is not handled inside an if statement call to the base class like the others
		return false;

	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponSOBaseSniper::PrimaryAttack( void )
{
	if( !IsBoltAction() || !m_bNeedsCocking )
	{
		int preShotAmmo = m_iClip1;

		BaseClass::PrimaryAttack();

		if( (m_iClip1 > 0) && IsBoltAction() ) // bolt action behaviour
			m_bNeedsCocking = true;

		WeaponSound( SPECIAL2 );
	}
}

// make snipers extremely accurate when scoped
float CWeaponSOBaseSniper::GetAccuracyModifier()
{
	if( m_bIsScoped )
		return 0.01f;

	return BaseClass::GetAccuracyModifier();
}

void CWeaponSOBaseSniper::ItemPostFrame( void )
{
	// bolt action sniper features
	if( IsBoltAction() )
	{
		// needs cocking, but we aren't cocking, so do it
		if( m_bNeedsCocking && !m_bIsCocking && (gpGlobals->curtime >= m_flNextPrimaryAttack) )
			CockBolt();
		// cocking has finished
		else if( m_bIsCocking && (gpGlobals->curtime >= m_flEndCockTime) )
			FinishCocking();
	}

	if( !IsCocking() || !IsBoltAction() )
		BaseClass::ItemPostFrame();
}

// start to cock the weapon
void CWeaponSOBaseSniper::CockBolt()
{
	m_bIsScoped = false;

	SendWeaponAnim( ACT_VM_PULLBACK );
	m_bIsCocking = true;

	float endTime = gpGlobals->curtime + SequenceDuration();
	
	m_flEndCockTime = endTime;
	m_flNextPrimaryAttack = endTime;
	SetWeaponIdleTime( endTime );
}

void CWeaponSOBaseSniper::FinishCocking()
{
	m_bIsScoped = false;
	m_bIsCocking = false;
	m_bNeedsCocking = false;
	m_flEndCockTime = 0.0f;
}

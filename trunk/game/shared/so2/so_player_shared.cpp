#include "cbase.h"

#ifdef CLIENT_DLL
	#include "c_so_player.h"
#else
	#include "so_player.h"
	#include "ai_basenpc.h"
#endif

#include "so_gamerules.h"
#include "takedamageinfo.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void SpawnBlood (Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

bool CSO_Player::Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon )
{
	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
    if( GetMoveType() == MOVETYPE_LADDER )
		return false;

	// Do not allow players to fire weapons while sprinting
    if( IsSprinting() )
		return false;

	return BaseClass::Weapon_CanSwitchTo( pWeapon );
}

//-----------------------------------------------------------------------------
// Consider the weapon's built-in accuracy, this character's proficiency with
// the weapon, and the status of the target. Use this information to determine
// how accurately to shoot at the target.
//-----------------------------------------------------------------------------
Vector CSO_Player::GetAttackSpread( CWeaponSOBase *pWeapon, CBaseEntity *pTarget )
{
	// Weapon accuracy system
	if ( pWeapon )
	{
		float weaponAccuracy = 1.0f; // by default, don't make any alterations

		if ( !(GetFlags() & FL_ONGROUND) )	// player is not on the ground; fuck with their accuracy
			weaponAccuracy *= 3.0f;	// this isn't done in CWeaponSOBase::GetAccuracyModifier() because some weapons override that function
									// after all, no weapon should be spared from a loss in accuracy because of jumping and whatnot!

		// CWeaponSOBase::GetAccuracyModifier() handles both player-specific and weapon-specific conditions, so factor that into our calculation now too
		return pWeapon->GetBulletSpread() * weaponAccuracy * pWeapon->GetAccuracyModifier();
	}
	
	return VECTOR_CONE_15DEGREES;
}

void CSO_Player::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	Vector vecOrigin = ptr->endpos - vecDir * 4;
	float flDistance = 0.0f;

	CBaseEntity *pAttacker = info.GetAttacker();
	if ( pAttacker )
	{
		// Do not allow players to hurt each other
		if ( !SOGameRules()->FPlayerCanTakeDamage( this, pAttacker ) )
			return;

		flDistance = (ptr->endpos - info.GetAttacker()->GetAbsOrigin()).Length();
	}

	if ( m_takedamage )
	{
		AddMultiDamage( info, this );

		int blood = BloodColor();
		if ( blood != DONT_BLEED )
		{
			SpawnBlood( vecOrigin, vecDir, blood, flDistance );// a little surface blood.
			TraceBleed( flDistance, vecDir, ptr, info.GetDamageType() );
		}
	}
}

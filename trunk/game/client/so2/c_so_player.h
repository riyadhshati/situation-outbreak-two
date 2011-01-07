//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef SO_PLAYER_H
#define SO_PLAYER_H
#pragma once

#include "c_hl2mp_player.h"
#include "so_player_shared.h"

// Add support for CS:S player animations
#include "so_playeranimstate.h"

//=============================================================================
// >> SO_Player
//=============================================================================
class C_SO_Player : public C_HL2MP_Player
{
public:
	DECLARE_CLASS( C_SO_Player, C_HL2MP_Player );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_SO_Player();
	~C_SO_Player( void );

	static C_SO_Player* GetLocalSOPlayer();

	// Add support for CS:S player animations
	const QAngle& GetRenderAngles();
	void UpdateClientSideAnimation();
	CStudioHdr *OnNewModel( void );
	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	int m_iThrowGrenadeCounter;	// used to trigger grenade throw animations.

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	CBaseCombatWeapon* GetHolsteredWeapon( void ) { return m_hHolsteredWeapon; }

public:
	// First-person ragdolls
	// http://developer.valvesoftware.com/wiki/First_Person_Ragdolls
	void CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );

	// See so_player_shared.cpp (these are shared functions)
	bool Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );
	Vector GetAttackSpread( CWeaponSOBase *pWeapon, CBaseEntity *pTarget = NULL );
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );
	void DoMuzzleFlash( void );

private:
	C_SO_Player( const C_SO_Player & );

	// Add support for CS:S player animations
	CSOPlayerAnimState *m_SOPlayerAnimState;

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	CHandle<CBaseCombatWeapon> m_hHolsteredWeapon;
};

inline C_SO_Player *ToSOPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<C_SO_Player*>( pEntity );
}

#endif //SO_PLAYER_H

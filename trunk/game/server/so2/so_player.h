#ifndef SO_PLAYER_H
#define SO_PLAYER_H
#pragma once

class CSO_Player;

#include "hl2mp_player.h"
#include "so_player_shared.h"

// Add support for CS:S player animations
#include "so_playeranimstate.h"

///////////////////////////////////
// DEFINE ALL PLAYER MODELS HERE //
///////////////////////////////////

static const char *PlayerModels[] = {
	"models/player/ct_gign.mdl",
	"models/player/ct_gsg9.mdl",
	"models/player/ct_sas.mdl",
	"models/player/ct_urban.mdl"
};

///////////////////////////////////

class CSO_Player : public CHL2MP_Player
{
public:
	DECLARE_CLASS( CSO_Player, CHL2MP_Player );

	CSO_Player();
	~CSO_Player( void );
	
	static CSO_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CSO_Player::s_PlayerEdict = ed;
		return (CSO_Player*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	// Add support for CS:S player animations
	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );
	void SetupBones( matrix3x4_t *pBoneToWorld, int boneMask );
	CNetworkVar( int, m_iThrowGrenadeCounter );	// used to trigger grenade throw animations.

public:
	void PickDefaultSpawnTeam( void );
	void Precache( void );
	void Spawn( void );
	void ChangeTeam( int iTeam );
	void GiveAllItems( void );
	void GiveDefaultItems( void );
	bool ValidatePlayerModel( const char *pModel );
	void SetPlayerModel( void );
	void PostThink( void );
	bool ClientCommand( const CCommand &args );
	bool HandleCommand_JoinTeam( int team );
	void Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity );
	int OnTakeDamage( const CTakeDamageInfo &inputInfo );
	void Event_Killed( const CTakeDamageInfo &info );
	bool StartObserverMode( int mode );

	// Rework respawning system
	void PlayerDeathThink( void );

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	CBaseCombatWeapon* GetHolsteredWeapon( void ) { return m_hHolsteredWeapon; }
	void SetHolsteredWeapon( CBaseCombatWeapon *pHolsteredWeapon ) { m_hHolsteredWeapon = pHolsteredWeapon; }

	float GetModelChangeDelay( void ) { return m_flModelChangeDelay; }

	// See so_player_shared.cpp (these are shared functions)
	bool Weapon_CanSwitchTo( CBaseCombatWeapon *pWeapon );
	Vector GetAttackSpread( CWeaponSOBase *pWeapon, CBaseEntity *pTarget = NULL );
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

private:
	// Health regeneration system
	float m_flHealthRegenDelay;

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	CNetworkHandle( CBaseCombatWeapon, m_hHolsteredWeapon );

	// Base each player's speed on their health
	float m_flSpeedCheckDelay;

	// Add support for CS:S player animations
	CSOPlayerAnimState *m_SOPlayerAnimState;

	float m_flModelChangeDelay;
};

inline CSO_Player *ToSOPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CSO_Player*>( pEntity );
}

#endif //SO_PLAYER_H

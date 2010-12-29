#ifndef SO_GAMERULES_H
#define SO_GAMERULES_H
#pragma once

#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "hl2mp_gamerules.h"
#include "gamevars_shared.h"

#ifndef CLIENT_DLL
	#include "so_player.h"
#endif

// Additional zombie team
enum
{
	TEAM_ZOMBIES = 4,
};

#ifdef CLIENT_DLL
	#define CSOGameRules C_SOGameRules
	#define CSOGameRulesProxy C_SOGameRulesProxy
#endif

class CSOGameRulesProxy : public CHL2MPGameRulesProxy
{
public:
	DECLARE_CLASS( CSOGameRulesProxy, CHL2MPGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CSOGameRules : public CHL2MPRules
{
public:
	DECLARE_CLASS( CSOGameRules, CHL2MPRules );

#ifdef CLIENT_DLL
	DECLARE_CLIENTCLASS_NOBASE(); // This makes datatables able to access our private vars.
#else
	DECLARE_SERVERCLASS_NOBASE(); // This makes datatables able to access our private vars.
#endif

	CSOGameRules();
	~CSOGameRules();

	void CheckRestartGame( void );
	void Think( void );
	void RestartGame( void );

	void Precache( void );
	bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );

	void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	void ClientSettingsChanged( CBasePlayer *pPlayer );

	const char *GetGameDescription( void ) { return "Situation Outbreak Two"; }

	// Rework respawning system
	int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );

	// Do not allow players to hurt each other
	bool FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );

#ifndef CLIENT_DLL
	// Fix various NPC-related issues and bugs in multiplayer
	// http://developer.valvesoftware.com/wiki/Fixing_AI_in_multiplayer#Patch
	// Using a similar system to the one mentioned in the link above, but it is not exactly the same
	void InitDefaultAIRelationships( void );

	bool FPlayerCanRespawn( CBasePlayer *pPlayer );
#endif

	// Clean up dead bodies and stuff every now and then
/*private:
#ifndef CLIENT_DLL
	CUtlVector<CBaseEntity*> g_EntsToCleanup;
	CUtlVector<CBaseEntity*> g_EntsBeingCleaned;
	CUtlVector<float> g_EntsBeingCleanedTime;
#endif*/

private:
#ifndef CLIENT_DLL
	// Sound announcements system
	// This function allows the server to play a particular sound to a single player or all players in the game
	void PlayAnnouncementSound( const char *soundName, CBasePlayer *pTargetPlayer );

	// Returns the amount of players who are on the server, regardless of whether or not they are alive (spectators don't count though)
	int GetNumPlayers();

	// Returns the amount of players who are alive on a particular team
	int GetNumAlivePlayers( int teamNumber );

	// Round system
	// Handles end of round conditions that can be checked periodically to save some resources
	void HandleRoundEndConditions( void );
	bool m_bRoundOver;
#endif
};

inline CSOGameRules* SOGameRules()
{
	return static_cast<CSOGameRules*>(g_pGameRules);
}

#endif //SO_GAMERULES_H

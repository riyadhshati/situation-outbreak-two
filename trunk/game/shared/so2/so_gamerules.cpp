#include "cbase.h"
#include "so_gamerules.h"
#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>
#include "ammodef.h"

#ifdef CLIENT_DLL
	#include "c_so_player.h"
#else

	#include "eventqueue.h"
	#include "player.h"
	#include "gamerules.h"
	#include "game.h"
	#include "items.h"
	#include "entitylist.h"
	#include "mapentities.h"
	#include "in_buttons.h"
	#include <ctype.h>
	#include "voice_gamemgr.h"
	#include "iscorer.h"
	#include "so_player.h"
	#include "weapon_hl2mpbasehlmpcombatweapon.h"
	#include "team.h"
	#include "voice_gamemgr.h"
	#include "hl2mp_gameinterface.h"
	#include "hl2mp_cvars.h"

#ifdef DEBUG	
	#include "hl2mp_bot_temp.h"
#endif

#endif

REGISTER_GAMERULES_CLASS( CSOGameRules );

BEGIN_NETWORK_TABLE_NOBASE( CSOGameRules, DT_SOGameRules )
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS( so_gamerules, CSOGameRulesProxy );
IMPLEMENT_NETWORKCLASS_ALIASED( SOGameRulesProxy, DT_SOGameRulesProxy )

#ifdef CLIENT_DLL
	void RecvProxy_SOGameRules( const RecvProp *pProp, void **pOut, void *pData, int objectID )
	{
		CSOGameRules *pRules = SOGameRules();
		Assert( pRules );
		*pOut = pRules;
	}

	BEGIN_RECV_TABLE( CSOGameRulesProxy, DT_SOGameRulesProxy )
		RecvPropDataTable( "so_gamerules_data", 0, 0, &REFERENCE_RECV_TABLE( DT_SOGameRules ), RecvProxy_SOGameRules )
	END_RECV_TABLE()
#else
	void* SendProxy_SOGameRules( const SendProp *pProp, const void *pStructBase, const void *pData, CSendProxyRecipients *pRecipients, int objectID )
	{
		CSOGameRules *pRules = SOGameRules();
		Assert( pRules );
		return pRules;
	}

	BEGIN_SEND_TABLE( CSOGameRulesProxy, DT_SOGameRulesProxy )
		SendPropDataTable( "so_gamerules_data", 0, &REFERENCE_SEND_TABLE( DT_SOGameRules ), SendProxy_SOGameRules )
	END_SEND_TABLE()
#endif

CSOGameRules::CSOGameRules()
{
#ifndef CLIENT_DLL
	// Fix various NPC-related issues and bugs in multiplayer
	// http://developer.valvesoftware.com/wiki/Fixing_AI_in_multiplayer#Patch
	// Using a similar system to the one mentioned in the link above, but it is not exactly the same
	InitDefaultAIRelationships();

	// Round system
	m_bRoundOver = false;
#endif
}
	
CSOGameRules::~CSOGameRules( void )
{
#ifndef CLIENT_DLL
	// Note, don't delete each team since they are in the gEntList and will 
	// automatically be deleted from there, instead.
	g_Teams.Purge();

	// Clean up dead bodies and stuff every now and then
	/*g_EntsToCleanup.Purge();
	g_EntsBeingCleaned.Purge();
	g_EntsBeingCleanedTime.Purge();*/
#endif
}

#ifndef CLIENT_DLL

// Sound announcements system
// This function allows the server to play a particular sound to a single player or all players in the game
void CSOGameRules::PlayAnnouncementSound( const char *soundName, CBasePlayer *pTargetPlayer = NULL )
{
	CRecipientFilter filter;

	if ( pTargetPlayer )	// target player defined, play sound only for specified player
		filter.AddRecipient( pTargetPlayer );
	else	// no target player defined, play sound for all players
		filter.AddAllPlayers();

	filter.MakeReliable();

	UserMessageBegin( filter, "SendAudio" );
	WRITE_STRING( soundName );
	MessageEnd();
}

// Returns the amount of players who are on the server, regardless of whether or not they are alive (spectators don't count though)
int CSOGameRules::GetNumPlayers()
{
	int iNumPlayers = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pClient = UTIL_PlayerByIndex( i );

		// Player must exist
		if ( !pClient || !pClient->edict() )
			continue;

		// Player must be connected
		if ( !(pClient->IsNetClient()) )
			continue;

		// Player must not be a spectator
		if( pClient->GetTeamNumber() == TEAM_SPECTATOR )
			continue;

		iNumPlayers++;
	}

	return iNumPlayers;
}

// Returns the amount of players who are alive on a particular team
int CSOGameRules::GetNumAlivePlayers( int teamNumber )
{
	int iNumPlayers = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pClient = UTIL_PlayerByIndex( i );

		// Player must exist
		if ( !pClient || !pClient->edict() )
			continue;

		// Player must be connected
		if ( !(pClient->IsNetClient()) )
			continue;
		
		// Player must be alive
		if( pClient->m_lifeState != LIFE_ALIVE )
			continue;

		// Player must be on the team specified in the parameter
		if( pClient->GetTeamNumber() != teamNumber )
			continue;

		iNumPlayers++;
	}

	return iNumPlayers;
}

// Round system
// Handles end of round conditions that can be checked periodically to save some resources
void CSOGameRules::HandleRoundEndConditions()
{
	// Make sure the round isn't over already
	if ( !m_bRoundOver )
	{
		// Make sure there is at least one player on the server who is playing to prevent unnecessary, annoying, and resource-hogging looping
		if ( GetNumPlayers() > 0 )
		{
			// No living players
			if ( GetNumAlivePlayers( TEAM_UNASSIGNED ) <= 0 )
			{
				PlayAnnouncementSound( "SO.RoundLost" );

				UTIL_ClientPrintAll( HUD_PRINTCENTER, "YOU FAILED!" );

				UTIL_ClientPrintAll( HUD_PRINTTALK, " " );
				UTIL_ClientPrintAll( HUD_PRINTTALK, "YOU FAILED!" );
				UTIL_ClientPrintAll( HUD_PRINTTALK, "Your team was wiped out." );
				UTIL_ClientPrintAll( HUD_PRINTTALK, "A new unit will be deployed shortly." );
				UTIL_ClientPrintAll( HUD_PRINTTALK, " " );

				mp_restartgame.SetValue( 7 );

				m_bRoundOver = true;
			}
		}
	}
}

void CSOGameRules::CheckRestartGame( void )
{
	// Restart the game if specified by the server
	int iRestartDelay = mp_restartgame.GetInt();

	if ( iRestartDelay > 0 )
	{
		if ( iRestartDelay > 60 )
			iRestartDelay = 60;

		m_flRestartGameTime = gpGlobals->curtime + iRestartDelay;
		m_bCompleteReset = true;
		mp_restartgame.SetValue( 0 );
	}
}

#endif

void CSOGameRules::Think( void )
{
#ifndef CLIENT_DLL
	
	CGameRules::Think();

	// Handle end of map level changes
	if ( g_fGameOver )
	{
		if ( m_flIntermissionEndTime < gpGlobals->curtime )
			ChangeLevel();

		return;
	}
	
	// Handle end of map conditions
	// (These probably shouldn't be checked periodically)
	if ( GetMapRemainingTime() < 0 )
	{
		GoToIntermission();
		return;
	}

	// Handle end of map conditions
	// (These probably shouldn't be checked periodically)
	float flFragLimit = fraglimit.GetFloat();
	if ( flFragLimit )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;

			if ( pPlayer->FragCount() >= flFragLimit )
			{
				GoToIntermission();
				return;
			}
		}
	}

	// Handle miscellaneous conditions that can be checked periodically
	if ( gpGlobals->curtime >= m_tmNextPeriodicThink )
	{
		// Round system
		HandleRoundEndConditions();

		//CheckAllPlayersReady();	// I don't see a need for this anymore
		CheckRestartGame();

		m_tmNextPeriodicThink = gpGlobals->curtime + 1.0;
	}

	// Handle game restart conditions
	if ( (m_flRestartGameTime > 0.0f) && (m_flRestartGameTime <= gpGlobals->curtime) )
		RestartGame();

	// I don't really know what this does...
	// Was in CSOGameRules::Think, so I'm keeping it around until we find a good reason not to
	ManageObjectRelocation();

	// Clean up dead bodies and stuff every now and then
	// (from here on down)

	// This stuff is now obsolete given its original purpose, but I've opted to just comment it instead of removing it entirely
	// Who knows, it might come in handy for something else
	// This solution is rather expensive though, so maybe not...

	// Find the types of entities we should be cleaning up
	/*CBaseEntity *pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassname( pEntity, "prop_ragdoll" )) != NULL )
	{
		if ( g_EntsToCleanup.Find( pEntity ) == -1 )
			g_EntsToCleanup.AddToTail( pEntity );
	}
	pEntity = NULL;
	while ( (pEntity = gEntList.FindEntityByClassname( pEntity, "raggib" )) != NULL )
	{
		if ( g_EntsToCleanup.Find( pEntity ) == -1 )
			g_EntsToCleanup.AddToTail( pEntity );
	}

	// Determine exactly which entities should be cleaned up
	for ( int i = 0; i < g_EntsToCleanup.Count(); i++ )
	{
		if ( g_EntsToCleanup[i] )
		{
			bool shouldBeginCleanup = true;

			if ( g_EntsBeingCleaned.Count() > 0 )
			{
				for ( int x = 0; x < g_EntsBeingCleaned.Count(); x++ )
				{
					if ( g_EntsBeingCleaned[x] && (g_EntsBeingCleaned[x] == g_EntsToCleanup[i]) )
						shouldBeginCleanup = false;	// this entity is currently being cleaned up, so avoid marking it for cleanup again
				}
			}

			if ( shouldBeginCleanup )
			{
				if ( g_EntsBeingCleaned.Find( g_EntsToCleanup[i] ) == -1 )
				{
					g_EntsBeingCleaned.AddToTail( g_EntsToCleanup[i] );
					g_EntsBeingCleanedTime.AddToTail( gpGlobals->curtime );
				}
			}
		}
	}

	// Actually cleanup entities that should be cleaned up
	if ( g_EntsBeingCleaned.Count() > 0 )
	{
		// We can easily adjust the timing of how entities are cleaned up here
		// The current values below seem to suffice though
		const int fadeDelayTime = 1;
		const int fadeTime = 3;

		for ( int y = 0; y < g_EntsBeingCleaned.Count(); y++ )
		{
			if ( g_EntsBeingCleaned[y] )
			{
				float pauseTime = g_EntsBeingCleanedTime[y] + fadeDelayTime;
				float endTime = pauseTime + fadeTime;

				if ( pauseTime <= gpGlobals->curtime )
					g_EntsBeingCleaned[y]->SetRenderColorA( 255 * ((endTime - gpGlobals->curtime) / fadeTime) );

				if ( endTime <= gpGlobals->curtime )
				{
					g_EntsBeingCleaned[y]->Remove();
					g_EntsBeingCleaned[y] = NULL;
					g_EntsBeingCleanedTime[y] = NULL;
				}
			}
			else
			{
				g_EntsBeingCleaned[y] = NULL;
				g_EntsBeingCleanedTime[y] = NULL;
			}
		}
	}*/
#endif
}

#ifndef CLIENT_DLL
void CSOGameRules::RestartGame()
{
	// Out-of-bounds check
	if ( mp_timelimit.GetInt() < 0 )
		mp_timelimit.SetValue( 0 );

	// Out-of-bounds check (?)
	m_flGameStartTime = gpGlobals->curtime;
	if ( !IsFinite( m_flGameStartTime.Get() ) )
	{
		Warning( "Trying to set a NaN game start time\n" );
		m_flGameStartTime.GetForModify() = 0.0f;
	}

	// Pre-cleanup stuff
	for (int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSO_Player *pPlayer = ToSOPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
		{
			if ( pPlayer->IsInAVehicle() )
				pPlayer->LeaveVehicle();

			QAngle angles = pPlayer->GetLocalAngles();

			angles.x = 0;
			angles.z = 0;

			pPlayer->SetLocalAngles( angles );

			CBaseCombatWeapon *pWeapon = (CBaseCombatWeapon*)pPlayer->GetActiveWeapon();
			if ( pWeapon )
			{
				pPlayer->Weapon_Detach( pWeapon );
				UTIL_Remove( pWeapon );
			}
		}

		pPlayer->RemoveAllItems( true );
		pPlayer->ClearActiveWeapon();
		pPlayer->Reset();
	}

	CleanUpMap();

	// Post-cleanup stuff
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CSO_Player *pPlayer = ToSOPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		if ( pPlayer->GetTeamNumber() != TEAM_SPECTATOR )
			pPlayer->Spawn();
	}

	m_flIntermissionEndTime = 0;
	m_flRestartGameTime = 0.0;
	m_bCompleteReset = false;

	// Round system
	m_bRoundOver = false;	// game has been reset, therefore a new round has begun

	IGameEvent *event = gameeventmanager->CreateEvent( "round_start" );
	if ( event )
	{
		event->SetInt( "fraglimit", 0 );
		event->SetInt( "priority", 6 ); // HLTV event priority, not transmitted

		event->SetString( "objective", "DEATHMATCH" );

		gameeventmanager->FireEvent( event );
	}
}
#endif

// TODO: Redo this when we have some spare time to include mod-specific event properties
// Right now it's identical to CSOGameRules's DeathNotice function in hl2mp_gamerules.cpp
void CSOGameRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
{
#ifndef CLIENT_DLL
	// Work out what killed the player, and send a message to all clients about it
	const char *killer_weapon_name = "world";		// by default, the player is killed by the world
	int killer_ID = 0;

	// Find the killer & the scorer
	CBaseEntity *pInflictor = info.GetInflictor();
	CBaseEntity *pKiller = info.GetAttacker();
	CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

	// Custom kill type?
	if ( info.GetDamageCustom() )
	{
		killer_weapon_name = GetDamageCustomString( info );
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
		}
	}
	else
	{
		// Is the killer a client?
		if ( pScorer )
		{
			killer_ID = pScorer->GetUserID();
			
			if ( pInflictor )
			{
				if ( pInflictor == pScorer )
				{
					// If the inflictor is the killer,  then it must be their current weapon doing the damage
					if ( pScorer->GetActiveWeapon() )
					{
						killer_weapon_name = pScorer->GetActiveWeapon()->GetClassname();
					}
				}
				else
				{
					killer_weapon_name = pInflictor->GetClassname();  // it's just that easy
				}
			}
		}
		else
		{
			killer_weapon_name = pInflictor->GetClassname();
		}

		// strip the NPC_* or weapon_* from the inflictor's classname
		if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
		{
			killer_weapon_name += 7;
		}
		else if ( strncmp( killer_weapon_name, "npc_", 4 ) == 0 )
		{
			killer_weapon_name += 4;
		}
		else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
		{
			killer_weapon_name += 5;
		}
		else if ( strstr( killer_weapon_name, "physics" ) )
		{
			killer_weapon_name = "physics";
		}

		if ( strcmp( killer_weapon_name, "prop_combine_ball" ) == 0 )
		{
			killer_weapon_name = "combine_ball";
		}
		else if ( strcmp( killer_weapon_name, "grenade_ar2" ) == 0 )
		{
			killer_weapon_name = "smg1_grenade";
		}
		else if ( strcmp( killer_weapon_name, "satchel" ) == 0 || strcmp( killer_weapon_name, "tripmine" ) == 0)
		{
			killer_weapon_name = "slam";
		}
	}

	IGameEvent *event = gameeventmanager->CreateEvent( "player_death" );
	if( event )
	{
		event->SetInt("userid", pVictim->GetUserID() );
		event->SetInt("attacker", killer_ID );
		event->SetString("weapon", killer_weapon_name );
		event->SetInt( "priority", 7 );
		gameeventmanager->FireEvent( event );
	}
#endif
}

void CSOGameRules::ClientSettingsChanged( CBasePlayer *pPlayer )
{
#ifndef CLIENT_DLL
	CSO_Player *pSOPlayer = ToSOPlayer( pPlayer );
	if ( !pSOPlayer )
		return;

	const char *pCurrentModel = modelinfo->GetModelName( pPlayer->GetModel() );
	const char *szModelName = engine->GetClientConVarValue( engine->IndexOfEdict( pPlayer->edict() ), "cl_playermodel" );

	// Check to see if our desired player model name is different than our current player model name
	if ( stricmp(szModelName, pCurrentModel) )
	{
		// It is, so try to make our current player model the same as the desired one using model names

		//Too soon, set the cvar back to what it was.
		//Note: this will make this function be called again
		//but since our models will match it'll just skip this whole dealio.
		if ( pSOPlayer->GetModelChangeDelay() >= gpGlobals->curtime )
		{
			char szReturnString[512];
			Q_snprintf( szReturnString, sizeof(szReturnString), "cl_playermodel %s\n", pCurrentModel );
			engine->ClientCommand( pSOPlayer->edict(), szReturnString );

			ClientPrint( pSOPlayer, HUD_PRINTTALK, "You cannot change your character so soon!" );

			Q_snprintf( szReturnString, sizeof(szReturnString), "Please wait %d more second(s) before trying to switch again.\n", (int)(pSOPlayer->GetNextModelChangeTime() - gpGlobals->curtime) );
			ClientPrint( pSOPlayer, HUD_PRINTTALK, szReturnString );
			return;
		}

		pSOPlayer->SetPlayerModel();
	}

	extern ConVar sv_report_client_settings;
	if ( sv_report_client_settings.GetInt() == 1 )
		UTIL_LogPrintf( "\"%s\" cl_cmdrate = \"%s\"\n", pSOPlayer->GetPlayerName(), engine->GetClientConVarValue( pSOPlayer->entindex(), "cl_cmdrate" ) );

	CTeamplayRules::ClientSettingsChanged( pSOPlayer );	// skip over CHL2MPRules::ClientSettingsChanged
#endif
}

int CSOGameRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
{
	// Do not allow players to hurt each other
	if ( pPlayer->IsPlayer() )
		return GR_TEAMMATE;

	return GR_NOTTEAMMATE;	// as of right now, no NPCs are friendly, but all players are (this will likely change in the future)
}

// Do not allow players to hurt each other
bool CSOGameRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
{
	// Base whether or not the player can take damage from this attacker on their relationship
	if ( PlayerRelationship( pAttacker, pPlayer ) == GR_TEAMMATE )
		 return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSOGameRules::Precache( void )
{
	BaseClass::Precache();
}

bool CSOGameRules::ClientCommand( CBaseEntity *pEdict, const CCommand &args )
{
	return BaseClass::ClientCommand( pEdict, args );
}

// Fix various NPC-related issues and bugs in multiplayer
// http://developer.valvesoftware.com/wiki/Fixing_AI_in_multiplayer#Patch
// Using a similar system to the one mentioned in the link above, but it is not exactly the same
#ifndef CLIENT_DLL

void CSOGameRules::InitDefaultAIRelationships( void )
{
	int i, j;

	//  Allocate memory for default relationships
	CBaseCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for ( i = 0; i < NUM_AI_CLASSES; i++ )
	{
		for ( j = 0; j < NUM_AI_CLASSES; j++ )
		{
			// By default all relationships are neutral of priority zero
			CBaseCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
		}
	}

	// ------------------------------------------------------------
	//	> CLASS_NONE
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_NONE,				D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_PLAYER,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_ZOMBIE,			D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_NONE,			CLASS_HEADCRAB,			D_NU, 0);

	// ------------------------------------------------------------
	//	> CLASS_PLAYER (used by all players)
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_NONE,				D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ZOMBIE,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HEADCRAB,			D_HT, 0);

	// ------------------------------------------------------------
	//	> CLASS_ZOMBIE (used by zombie NPCs)
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_NONE,				D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ZOMBIE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HEADCRAB,			D_LI, 0);

	// ------------------------------------------------------------
	//	> CLASS_HEADCRAB (used by headcrab NPCs)
	// ------------------------------------------------------------
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,		CLASS_NONE,				D_NU, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,		CLASS_PLAYER,			D_HT, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,		CLASS_ZOMBIE,			D_LI, 0);
	CBaseCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,		CLASS_HEADCRAB,			D_LI, 0);
}

#endif

// Redefine ammo definitions elsewhere

// shared ammo definition
// JAY: Trying to make a more physical bullet response
#define BULLET_MASS_GRAINS_TO_LB(grains)	(0.002285*(grains)/16.0f)
#define BULLET_MASS_GRAINS_TO_KG(grains)	lbs2kg(BULLET_MASS_GRAINS_TO_LB(grains))

// exaggerate all of the forces, but use real numbers to keep them consistent
#define BULLET_IMPULSE_EXAGGERATION			3.5
// convert a velocity in ft/sec and a mass in grains to an impulse in kg in/s
#define BULLET_IMPULSE(grains, ftpersec)	((ftpersec)*12*BULLET_MASS_GRAINS_TO_KG(grains)*BULLET_IMPULSE_EXAGGERATION)

CAmmoDef *GetAmmoDef()
{
	static CAmmoDef def;
	static bool bInitted = false;
	
	if ( !bInitted )
	{
		bInitted = true;

		// Additional ammo definitions should be added to this list accordingly
		def.AddAmmoType("pistol",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			130,		BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("heavypistol",		DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			35,			BULLET_IMPULSE(800, 5000),	0 );
		def.AddAmmoType("smg",				DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			250,		BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("rifle",			DMG_BULLET,					TRACER_LINE_AND_WHIZ,	0,			0,			120,		BULLET_IMPULSE(200, 1225),	0 );
		def.AddAmmoType("shotgun",			DMG_BULLET | DMG_BUCKSHOT,	TRACER_LINE,			0,			0,			40,			BULLET_IMPULSE(400, 1200),	0 );
	}

	return &def;
}

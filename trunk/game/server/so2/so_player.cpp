#include "cbase.h"
#include "so_player.h"

#include "team.h"
#include "in_buttons.h"
#include "ai_basenpc.h"
#include "bone_setup.h"
#include "so_gamerules.h"
#include "studio.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define MODEL_CHANGE_DELAY 10.0f

// Health regeneration system
ConVar so_health_regen( "so_health_regen", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Toggles player health regeneration functionality" );
ConVar so_health_regen_delay( "so_health_regen_delay", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Defines the amount of time (in seconds) before players are granted additional health" );
ConVar so_health_regen_amount( "so_health_regen_amount", "1", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Defines the amount of additional health granted to players upon regeneration" );

// Rework respawning system
ConVar so_respawn( "so_respawn", "0", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Toggles automatic respawning functionality" );
ConVar so_respawn_time( "so_respawn_time", "5", FCVAR_GAMEDLL | FCVAR_NOTIFY, "Defines the amount of time (in seconds) dead players must wait before they respawn" );

LINK_ENTITY_TO_CLASS( player, CSO_Player );

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

BEGIN_SEND_TABLE_NOBASE( CSO_Player, DT_SOLocalPlayerExclusive )
END_SEND_TABLE()

BEGIN_SEND_TABLE_NOBASE( CSO_Player, DT_SONonLocalPlayerExclusive )
END_SEND_TABLE()

IMPLEMENT_SERVERCLASS_ST( CSO_Player, DT_SO_Player )
END_SEND_TABLE()

BEGIN_DATADESC( CSO_Player )
END_DATADESC()

#pragma warning( disable : 4355 )

CSO_Player::CSO_Player()
{
	// Add support for CS:S player animations
	m_SOPlayerAnimState = CreateSOPlayerAnimState( this );
	UseClientSideAnimation();

	// Health regeneration system
	m_flHealthRegenDelay = 0.0f;

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	m_pHolsteredWeapon = NULL;

	// Base each player's speed on their health
	m_flSpeedCheckDelay = 0.0f;

	// Add support for CS:S player animations
	DoAnimationEvent( PLAYERANIMEVENT_SPAWN );
}

CSO_Player::~CSO_Player( void )
{
	// Add support for CS:S player animations
	m_SOPlayerAnimState->Release();
}

void CSO_Player::PickDefaultSpawnTeam( void )
{
	if ( GetTeamNumber() == 0 )
	{
		if ( SOGameRules() && !SOGameRules()->IsTeamplay() )
		{
			if ( !GetModelPtr() )
				ChangeTeam( TEAM_UNASSIGNED );
		}
	}
}

void CSO_Player::Precache( void )
{
	BaseClass::Precache();

	// Add rain splashes to func_precipitation
	// TODO: Perhaps find a better place to do this?
	PrecacheParticleSystem( "rainsplash" );

	// Precache player models
	for ( int i = 0; i < ARRAYSIZE(PlayerModels); i++ )
	   	 PrecacheModel( PlayerModels[i] );
}

void CSO_Player::Spawn( void )
{
	BaseClass::Spawn();
}

void CSO_Player::ChangeTeam( int iTeam )
{
	if ( iTeam != TEAM_SPECTATOR )
		iTeam = TEAM_UNASSIGNED;	// players are not allowed to change teams

	CBasePlayer::ChangeTeam( iTeam );	// skip over CHL2MP_Player::ChangeTeam

	SetPlayerModel();

	if ( iTeam == TEAM_SPECTATOR )
	{
		RemoveAllItems( true );
		State_Transition( STATE_OBSERVER_MODE );
	}
}

void CSO_Player::GiveAllItems( void )
{
	// Restock our default weapons and ammo first
	GiveDefaultItems();

	// Give us all the other weapons in the game in addition to our default weapons
	GiveNamedItem( "weapon_deagle" );
	GiveNamedItem( "weapon_p90" );
	GiveNamedItem( "weapon_aug" );
	GiveNamedItem( "weapon_m3" );
}

void CSO_Player::GiveDefaultItems( void )
{
	// Give us all the ammo we'll ever need when we spawn
	// (the actual amount of ammo we get depends on the maximum amount of ammo each weapon can hold)
	CBasePlayer::GiveAmmo( 9999, "pistol" );
	CBasePlayer::GiveAmmo( 9999, "heavypistol" );
	CBasePlayer::GiveAmmo( 9999, "smg" );
	CBasePlayer::GiveAmmo( 9999, "rifle" );
	CBasePlayer::GiveAmmo( 9999, "shotgun" );

	// Equip and switch to our default pistol (p228) automatically
	GiveNamedItem( "weapon_p228" );
	Weapon_Switch( Weapon_OwnsThisType( "weapon_p228" ) );
}

bool CSO_Player::ValidatePlayerModel( const char *pModel )
{
	for ( int i = 0; i < ARRAYSIZE(PlayerModels); i++ )
	{
		if ( !Q_stricmp(PlayerModels[i], pModel) )
			return true;
	}

	return false;
}

void CSO_Player::SetPlayerModel()
{
	const char *szDesiredModelName = engine->GetClientConVarValue( engine->IndexOfEdict( edict() ), "cl_playermodel" );
	while ( !ValidatePlayerModel(szDesiredModelName) || (Q_strlen(szDesiredModelName) == 0) || (modelinfo->GetModelIndex(szDesiredModelName) == -1) )
	{
		// The player needs their model changed, but an invalid one is specified for cl_playermodel
		// Randomly choose a valid player model for them instead and write the new model name to cl_playermodel to correct the problem

		const char *tempModelName = PlayerModels[RandomInt(0, ARRAYSIZE(PlayerModels) - 1)];

		char szReturnString[512];
		Q_snprintf( szReturnString, sizeof(szReturnString), "cl_playermodel %s\n", tempModelName );
		engine->ClientCommand( edict(), szReturnString );

		szDesiredModelName = tempModelName;
	}

	SetModel( szDesiredModelName );

	m_flModelChangeDelay = gpGlobals->curtime + MODEL_CHANGE_DELAY;
}

void CSO_Player::PostThink( void )
{
	CHL2_Player::PostThink();	// skip over CHL2MP_Player::PostThink

	if ( GetFlags() & FL_DUCKING )
		SetCollisionBounds( VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX );

	QAngle angles = GetLocalAngles();
	angles[PITCH] = 0;
	SetLocalAngles( angles );

	// Add support for CS:S player animations
	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();
    m_SOPlayerAnimState->Update( m_angEyeAngles[YAW], m_angEyeAngles[PITCH] );

	// Health regeneration system
	if ( gpGlobals->curtime >= m_flHealthRegenDelay )
	{
		// Don't regenerate the health of those who aren't alive or playing
		if ( GetTeamNumber() == TEAM_SPECTATOR || !IsAlive() )
			return;

		if ( so_health_regen.GetBool() )
		{
			int currentHealth = GetHealth();
			int newHealth = currentHealth;
			int maxHealth = GetMaxHealth();

			if ( currentHealth < maxHealth )	// we're wounded, so start regenerating health
				newHealth = currentHealth + so_health_regen_amount.GetInt();
			else if ( currentHealth > maxHealth )	// don't allow us to go over our maximum health
				newHealth = maxHealth;

			if ( newHealth != currentHealth )
				SetHealth( newHealth );	// actually set our new health value (assuming our health has changed)
		}

		m_flHealthRegenDelay = gpGlobals->curtime + so_health_regen_delay.GetFloat();
	}

	// Base each player's speed on their health
	// The original SetMaxSpeed function has been overrided to make this all work properly (see player.h)
	if ( gpGlobals->curtime >= m_flSpeedCheckDelay )
	{
		// There's no need for any speed alterations when we're not alive or playing
		if ( (GetTeamNumber() != TEAM_SPECTATOR) && IsAlive() )
		{
			if ( IsWalking() )
				SetMaxSpeed( SO_WALK_SPEED );
			else if ( IsSprinting() )
				SetMaxSpeed( SO_SPRINT_SPEED );
			else
				SetMaxSpeed( SO_NORM_SPEED );
		}

		m_flSpeedCheckDelay = gpGlobals->curtime + 0.5f;	// this should be often enough without overloading servers...hopefully...
	}
}

// Much of this function should resemble CHL2MP_Player::ClientCommand
bool CSO_Player::ClientCommand( const CCommand &args )
{
	if ( FStrEq( args[0], "spectate" ) )
	{
		if ( ShouldRunRateLimitedCommand( args ) )
		{
			// instantly join spectators
			HandleCommand_JoinTeam( TEAM_SPECTATOR );
		}

		return true;
	}
	else if ( FStrEq( args[0], "jointeam" ) ) 
	{
		if ( args.ArgC() < 2 )
			Warning( "Player sent bad jointeam syntax\n" );

		if ( ShouldRunRateLimitedCommand( args ) )
		{
			int iTeam = atoi( args[1] );

			// Do not allow players to willingly join whatever teams they want other than spectator
			if ( iTeam != TEAM_SPECTATOR )
			{
				Warning( "You cannot willingly switch teams like that! Nice try though. :P\n" );
				return true;
			}

			HandleCommand_JoinTeam( iTeam );
		}

		return true;
	}
	else if ( FStrEq( args[0], "joingame" ) )
	{
		return true;
	}

	return CHL2_Player::ClientCommand( args );	// skip over CHL2MP_Player::ClientCommand since most of it is here already, etc
}

// Much of this function should resemble CHL2MP_Player::HandleCommand_JoinTeam
bool CSO_Player::HandleCommand_JoinTeam( int team )
{
	if ( !GetGlobalTeam( team ) || team == 0 )
	{
		Warning( "HandleCommand_JoinTeam( %d ) - invalid team index.\n", team );
		return false;
	}

	if ( team == TEAM_SPECTATOR )
	{
		// Prevent this is the cvar is set
		if ( !mp_allowspectators.GetInt() )
		{
			ClientPrint( this, HUD_PRINTCENTER, "#Cannot_Be_Spectator" );
			return false;
		}

		if ( !IsDead() )
		{
			m_fNextSuicideTime = gpGlobals->curtime;	// allow the suicide to work
			CommitSuicide();
		}

		ChangeTeam( TEAM_SPECTATOR );

		return true;
	}
	else
	{
		StopObserverMode();
		State_Transition(STATE_ACTIVE);
	}

	// Switch their actual team...
	ChangeTeam( team );

	return true;
}

// Allow players to drop their active weapons
void CC_Player_Drop( void )
{
	CSO_Player *pPlayer = ToSOPlayer( UTIL_GetCommandClient() );
	if( !pPlayer )
		return;

	CWeaponSOBase *pWeapon = dynamic_cast<CWeaponSOBase*>( pPlayer->GetActiveWeapon() );
	if( pWeapon )
	{
		// TODO: This next bit is commented for now until we have some default weapons coded in to work with

		/*// Do not allow players to drop certain default weapons
		if( FClassnameIs( pWeapon, "weapon_zombie" ) ||
			FClassnameIs( pWeapon, "weapon_charple" ) ||
			FClassnameIs( pWeapon, "weapon_corpse" ) ||
			FClassnameIs( pWeapon, "weapon_stalker" ) ||
			FClassnameIs( pWeapon, "weapon_brassknuckles" ) )
		{
			return;
		}*/

		pPlayer->Weapon_Drop( pWeapon, NULL, NULL );
	}
}
static ConCommand drop( "drop", CC_Player_Drop, "Drops the player's active weapon (if possible)" );

// This function may need to be reworked in the future, so I've decided to copy and paste all BaseClass versions
// In fact, I've already made some changes...
void CSO_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget, const Vector *pVelocity )
{
	if ( pWeapon )	// should prevent some rare and strange crashes
	{
		bool bWasActiveWeapon = false;
		if ( pWeapon == GetActiveWeapon() )
			bWasActiveWeapon = true;

		if ( pWeapon )
		{
			if ( bWasActiveWeapon )
				pWeapon->SendWeaponAnim( ACT_VM_IDLE );
		}

		// If I'm an NPC, fill the weapon with ammo before I drop it.
		if ( GetFlags() & FL_NPC )
		{
			if ( pWeapon->UsesClipsForAmmo1() )
				pWeapon->m_iClip1 = pWeapon->GetDefaultClip1();
			if ( pWeapon->UsesClipsForAmmo2() )
				pWeapon->m_iClip2 = pWeapon->GetDefaultClip2();
		}

		if ( IsPlayer() )
		{
			Vector vThrowPos = Weapon_ShootPosition() - Vector(0,0,12);

			if( UTIL_PointContents(vThrowPos) & CONTENTS_SOLID )
				Msg("Weapon spawning in solid!\n");

			pWeapon->SetAbsOrigin( vThrowPos );

			QAngle gunAngles;
			VectorAngles( BodyDirection2D(), gunAngles );
			pWeapon->SetAbsAngles( gunAngles );
		}
		else
		{
			int iBIndex = -1;
			int iWeaponBoneIndex = -1;

			CStudioHdr *hdr = pWeapon->GetModelPtr();
			// If I have a hand, set the weapon position to my hand bone position.
			if ( hdr && hdr->numbones() > 0 )
			{
				// Assume bone zero is the root
				for ( iWeaponBoneIndex = 0; iWeaponBoneIndex < hdr->numbones(); ++iWeaponBoneIndex )
				{
					iBIndex = LookupBone( hdr->pBone( iWeaponBoneIndex )->pszName() );
					// Found one!
					if ( iBIndex != -1 )
						break;
				}

				if ( iBIndex == -1 )
					iBIndex = LookupBone( "ValveBiped.Weapon_bone" );
			}
			else
			{
				iBIndex = LookupBone( "ValveBiped.Weapon_bone" );
			}

			if ( iBIndex != -1)  
			{
				Vector origin;
				QAngle angles;
				matrix3x4_t transform;

				// Get the transform for the weapon bonetoworldspace in the NPC
				GetBoneTransform( iBIndex, transform );

				// find offset of root bone from origin in local space
				// Make sure we're detached from hierarchy before doing this!!!
				pWeapon->StopFollowingEntity();
				pWeapon->SetAbsOrigin( Vector( 0, 0, 0 ) );
				pWeapon->SetAbsAngles( QAngle( 0, 0, 0 ) );
				pWeapon->InvalidateBoneCache();
				matrix3x4_t rootLocal;
				pWeapon->GetBoneTransform( iWeaponBoneIndex, rootLocal );

				// invert it
				matrix3x4_t rootInvLocal;
				MatrixInvert( rootLocal, rootInvLocal );

				matrix3x4_t weaponMatrix;
				ConcatTransforms( transform, rootInvLocal, weaponMatrix );
				MatrixAngles( weaponMatrix, angles, origin );
			
				pWeapon->Teleport( &origin, &angles, NULL );
			}
			// Otherwise just set in front of me.
			else 
			{
				Vector vFacingDir = BodyDirection2D();
				vFacingDir = vFacingDir * 10.0; 
				pWeapon->SetAbsOrigin( Weapon_ShootPosition() + vFacingDir );
			}
		}

		Vector vecThrow;
		if (pvecTarget)
		{
			// I've been told to throw it somewhere specific.
			vecThrow = VecCheckToss( this, pWeapon->GetAbsOrigin(), *pvecTarget, 0.2, 1.0, false );
		}
		else
		{
			if ( pVelocity )
			{
				vecThrow = *pVelocity;
				float flLen = vecThrow.Length();
				if (flLen > 400)
				{
					VectorNormalize(vecThrow);
					vecThrow *= 400;
				}
			}
			else
			{
				// Nowhere in particular; just drop it.
				float throwForce = ( IsPlayer() ) ? 400.0f : random->RandomInt( 64, 128 );
				vecThrow = BodyDirection3D() * throwForce;
			}
		}

		pWeapon->Drop( vecThrow );
		Weapon_Detach( pWeapon );

		if ( HasSpawnFlags( SF_NPC_NO_WEAPON_DROP ) )
		{
			// Don't drop weapons when the super physgun is happening.
			UTIL_Remove( pWeapon );
		}

		if ( bWasActiveWeapon )
		{
			if (!SwitchToNextBestWeapon( NULL ))
			{
				CBaseViewModel *vm = GetViewModel();
				if ( vm )
					vm->AddEffects( EF_NODRAW );
			}
		}

		// Make weapons that are on the ground blink so that they are easier for players to see
		pWeapon->AddEffects( EF_ITEM_BLINK );
	}
}

int CSO_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	return BaseClass::OnTakeDamage( inputInfo );
}

void CSO_Player::Event_Killed( const CTakeDamageInfo &info )
{
	//update damage info with our accumulated physics force
	CTakeDamageInfo subinfo = info;
	subinfo.SetDamageForce( m_vecTotalBulletForce );

	// Note: since we're dead, it won't draw us on the client, but we don't set EF_NODRAW
	// because we still want to transmit to the clients in our PVS.
	CreateRagdollEntity();

	DetonateTripmines();

	CHL2_Player::Event_Killed( subinfo );

	if ( info.GetDamageType() & DMG_DISSOLVE )
	{
		if ( m_hRagdoll )
			m_hRagdoll->GetBaseAnimating()->Dissolve( NULL, gpGlobals->curtime, false, ENTITY_DISSOLVE_NORMAL );
	}

	CSO_Player *pAttackerPlayer = ToSOPlayer( info.GetAttacker() );
	if ( pAttackerPlayer )
	{
		if ( pAttackerPlayer != this )
			GetGlobalTeam( pAttackerPlayer->GetTeamNumber() )->AddScore( 1 );
	}

	FlashlightTurnOff();

	m_lifeState = LIFE_DEAD;

	RemoveEffects( EF_NODRAW );	// still draw player body
	StopZooming();

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	m_pHolsteredWeapon = NULL;	// weapon is no longer holstered (we're dead); reset variable
}

// Much of the contents of this function are identical to CBasePlayer::PlayerDeathThink
void CSO_Player::PlayerDeathThink( void )
{
	float flForward;

	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( GetFlags() & FL_ONGROUND )
	{
		flForward = GetAbsVelocity().Length() - 20;
		if ( flForward <= 0 )
		{
			SetAbsVelocity( vec3_origin );
		}
		else
		{
			Vector vecNewVelocity = GetAbsVelocity();
			VectorNormalize( vecNewVelocity );
			vecNewVelocity *= flForward;
			SetAbsVelocity( vecNewVelocity );
		}
	}

	// we drop the guns here because weapons that have an area effect and can kill their user
	// will sometimes crash coming back from CBasePlayer::Killed() if they kill their owner because the
	// player class sometimes is freed. It's safer to manipulate the weapons once we know
	// we aren't calling into any of their code anymore through the player pointer.
	if ( HasWeapons() )
		PackDeadPlayerItems();

	if ( GetModelIndex() && !IsSequenceFinished() && (m_lifeState == LIFE_DYING) )
	{
		StudioFrameAdvance();

		m_iRespawnFrames++;
		if ( m_iRespawnFrames < 60 )  // animations should be no longer than this
			return;
	}

	if ( m_lifeState == LIFE_DYING )
	{
		m_lifeState = LIFE_DEAD;
		m_flDeathAnimTime = gpGlobals->curtime;
	}
	
	StopAnimation();

	AddEffects( EF_NOINTERP );
	m_flPlaybackRate = 0.0;
	
	int fAnyButtonDown = m_nButtons & ~IN_SCORE;
	
	// Strip out the duck key from this check if it's toggled
	if ( (fAnyButtonDown & IN_DUCK) && GetToggledDuckState() )
		fAnyButtonDown &= ~IN_DUCK;

	// wait for all buttons released
	if ( m_lifeState == LIFE_DEAD )
	{
		if ( fAnyButtonDown )
			return;

		if ( g_pGameRules->FPlayerCanRespawn(this) )
			m_lifeState = LIFE_RESPAWNABLE;
		
		return;
	}

	// if the player has been dead for one second longer than allowed by forcerespawn, 
	// forcerespawn isn't on. Send the player off to an intermission camera until they 
	// choose to respawn.
	if ( (gpGlobals->curtime >= (m_flDeathTime + DEATH_ANIMATION_TIME)) && !IsObserver() )
	{
		StartObserverMode( OBS_MODE_ROAMING );	// start roaming around as an observer now that we're dead

		// Rework respawning system
		if ( !so_respawn.GetBool() || (GetTeamNumber() == TEAM_SPECTATOR) )	// we shouldn't need to think again under either of these conditions...at least for now
		{
			SetNextThink( TICK_NEVER_THINK );	// TODO: This doesn't seem to prevent us from repeating this function again and again
			return;	// we won't respawn until the round in-progress is over, so stop checking now that we're observing
		}
	}

	// Rework respawning system
	if ( (GetTeamNumber() != TEAM_SPECTATOR) && so_respawn.GetBool() && (gpGlobals->curtime >= (m_flDeathTime + DEATH_ANIMATION_TIME + so_respawn_time.GetInt())) )
	{
		m_nButtons = 0;
		m_iRespawnFrames = 0;

		extern void respawn( CBaseEntity *pEdict, bool fCopyCorpse );
		respawn( this, !IsObserver() );// don't copy a corpse if we're in deathcam.

		SetNextThink( TICK_NEVER_THINK );
	}
}

// Much of the contents of this function are identical to CBasePlayer::StartObserverMode
bool CSO_Player::StartObserverMode( int mode )
{
	VPhysicsDestroyObject();

	if ( !IsObserver() )
	{
		// set position to last view offset
		SetAbsOrigin( GetAbsOrigin() + GetViewOffset() );
		SetViewOffset( vec3_origin );
	}

	Assert( mode > OBS_MODE_NONE );
	
	m_afPhysicsFlags |= PFLAG_OBSERVER;

	// Holster weapon immediately, to allow it to cleanup
    if ( GetActiveWeapon() )
		GetActiveWeapon()->Holster();

	// clear out the suit message cache so we don't keep chattering
    SetSuitUpdate( NULL, FALSE, 0 );

	SetGroundEntity( (CBaseEntity *)NULL );
	
	RemoveFlag( FL_DUCKING );
	
    AddSolidFlags( FSOLID_NOT_SOLID );

	SetObserverMode( mode );

	if ( gpGlobals->eLoadType != MapLoad_Background )
		ShowViewPortPanel( "specgui" , ModeWantsSpectatorGUI(mode) );
	
	// Setup flags
    m_Local.m_iHideHUD = HIDEHUD_HEALTH;
	m_takedamage = DAMAGE_NO;		

	// Become invisible
	AddEffects( EF_NODRAW );		

	m_iHealth = 1;
	m_lifeState = LIFE_DEAD; // Can't be dead, otherwise movement doesn't work right.
	m_flDeathAnimTime = gpGlobals->curtime;

	// Fix roaming (free look) mode for observers and spectators
	// This flag prevents players from moving...what were you thinking, Valve!?
	//pl.deadflag = true;
	pl.deadflag = false;

	return true;
}

// Add support for CS:S player animations
// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //
class CTEPlayerAnimEvent : public CBaseTempEntity
{
public:
	DECLARE_CLASS( CTEPlayerAnimEvent, CBaseTempEntity );
	DECLARE_SERVERCLASS();

					CTEPlayerAnimEvent( const char *name ) : CBaseTempEntity( name )
					{
					}

	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_SERVERCLASS_ST_NOBASE( CTEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	SendPropEHandle( SENDINFO( m_hPlayer ) ),
	SendPropInt( SENDINFO( m_iEvent ), Q_log2( PLAYERANIMEVENT_COUNT ) + 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_nData ), 32 )
END_SEND_TABLE()

static CTEPlayerAnimEvent g_TEPlayerAnimEvent( "PlayerAnimEvent" );

void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData )
{
	CPVSFilter filter( (const Vector&)pPlayer->EyePosition() );

	//Tony; use prediction rules.
	filter.UsePredictionRules();
	
	g_TEPlayerAnimEvent.m_hPlayer = pPlayer;
	g_TEPlayerAnimEvent.m_iEvent = event;
	g_TEPlayerAnimEvent.m_nData = nData;
	g_TEPlayerAnimEvent.Create( filter, 0 );
}

// Add support for CS:S player animations
void CSO_Player::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	m_SOPlayerAnimState->DoAnimationEvent( event, nData );
	TE_PlayerAnimEvent( this, event, nData );	// Send to any clients who can see this guy.
}

// Add support for CS:S player animations
void CSO_Player::SetupBones( matrix3x4_t *pBoneToWorld, int boneMask )
{
	VPROF_BUDGET( "CSO_Player::SetupBones", VPROF_BUDGETGROUP_SERVER_ANIM );

	// Set the mdl cache semaphore.
	MDLCACHE_CRITICAL_SECTION();

	// Get the studio header.
	Assert( GetModelPtr() );
	CStudioHdr *pStudioHdr = GetModelPtr( );

	Vector pos[MAXSTUDIOBONES];
	Quaternion q[MAXSTUDIOBONES];

	// Adjust hit boxes based on IK driven offset.
	Vector adjOrigin = GetAbsOrigin() + Vector( 0, 0, m_flEstIkOffset );

	// FIXME: pass this into Studio_BuildMatrices to skip transforms
	CBoneBitList boneComputed;
	if ( m_pIk )
	{
		m_iIKCounter++;
		m_pIk->Init( pStudioHdr, GetAbsAngles(), adjOrigin, gpGlobals->curtime, m_iIKCounter, boneMask );
		GetSkeleton( pStudioHdr, pos, q, boneMask );

		m_pIk->UpdateTargets( pos, q, pBoneToWorld, boneComputed );
		CalculateIKLocks( gpGlobals->curtime );
		m_pIk->SolveDependencies( pos, q, pBoneToWorld, boneComputed );
	}
	else
	{
		GetSkeleton( pStudioHdr, pos, q, boneMask );
	}

	CBaseAnimating *pParent = dynamic_cast< CBaseAnimating* >( GetMoveParent() );
	if ( pParent )
	{
		// We're doing bone merging, so do special stuff here.
		CBoneCache *pParentCache = pParent->GetBoneCache();
		if ( pParentCache )
		{
			BuildMatricesWithBoneMerge( 
				pStudioHdr, 
				m_SOPlayerAnimState->GetRenderAngles(),
				adjOrigin, 
				pos, 
				q, 
				pBoneToWorld, 
				pParent, 
				pParentCache );

			return;
		}
	}

	Studio_BuildMatrices( 
		pStudioHdr, 
		m_SOPlayerAnimState->GetRenderAngles(),
		adjOrigin, 
		pos, 
		q, 
		-1,
		pBoneToWorld,
		boneMask );
}
// Add support for CS:S player animations
// This file should at least somewhat resemble sdk_playeranimstate.cpp because it is originally based on that file

#include "cbase.h"
#include "base_playeranimstate.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"

#include "so_playeranimstate.h"
#include "base_playeranimstate.h"
#include "datacache/imdlcache.h"

#ifdef CLIENT_DLL
	#include "c_so_player.h"
#else
	#include "so_player.h"
#endif

#include "weapon_fraggrenade.h"

#define SO_ANIM_RUN_SPEED 320.0f
#define SO_ANIM_WALK_SPEED 75.0f
#define SO_ANIM_CROUCHWALK_SPEED 110.0f

#define DEFAULT_AIM_IDLE_NAME "idle_upper_"
#define DEFAULT_AIM_CROUCH_IDLE_NAME "crouch_idle_upper_"
#define DEFAULT_AIM_CROUCH_WALK_NAME "crouch_walk_upper_"
#define DEFAULT_AIM_WALK_NAME "walk_upper_"
#define DEFAULT_AIM_RUN_NAME "run_upper_"
#define DEFAULT_FIRE_IDLE_NAME "idle_shoot_"
#define DEFAULT_FIRE_CROUCH_NAME "crouch_idle_shoot_"
#define DEFAULT_FIRE_CROUCH_WALK_NAME "crouch_walk_shoot_"
#define DEFAULT_FIRE_WALK_NAME "walk_shoot_"
#define DEFAULT_FIRE_RUN_NAME "run_shoot_"
#define DEFAULT_RELOAD_IDLE_NAME "idle_reload_"
#define DEFAULT_RELOAD_CROUCH_NAME "crouch_idle_reload_"
#define DEFAULT_RELOAD_CROUCH_WALK_NAME "crouch_walk_reload_"
#define DEFAULT_RELOAD_WALK_NAME "walk_reload_"
#define DEFAULT_RELOAD_RUN_NAME "run_reload_"

#define FIRESEQUENCE_LAYER (AIMSEQUENCE_LAYER + NUM_AIMSEQUENCE_LAYERS)
#define RELOADSEQUENCE_LAYER (FIRESEQUENCE_LAYER + 1)
#define GRENADESEQUENCE_LAYER (RELOADSEQUENCE_LAYER + 1)
#define NUM_LAYERS_WANTED (GRENADESEQUENCE_LAYER + 1)

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CSOPlayerAnimState* CreateSOPlayerAnimState( CSO_Player *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Setup the movement data.
	MultiPlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = SO_ANIM_RUN_SPEED;
	movementData.m_flWalkSpeed = SO_ANIM_WALK_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	// Create animation state for this player.
	CSOPlayerAnimState *pRet = new CSOPlayerAnimState( pPlayer, movementData );

	// Specific SO player initialization.
	pRet->InitSOAnimState( pPlayer );

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CSOPlayerAnimState::CSOPlayerAnimState()
{
	m_pSOPlayer = NULL;

	// Don't initialize SO specific variables here. Init them in InitSOAnimState()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CSOPlayerAnimState::CSOPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
: CMultiPlayerAnimState( pPlayer, movementData )
{
	m_pSOPlayer = NULL;

	// Don't initialize SO specific variables here. Init them in InitSOAnimState()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CSOPlayerAnimState::~CSOPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize Team Fortress specific animation state.
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CSOPlayerAnimState::InitSOAnimState( CSO_Player *pPlayer )
{
	m_pSOPlayer = pPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSOPlayerAnimState::ClearAnimationState( void )
{
	m_bThrowingGrenade = m_bPrimingGrenade = false;
	m_iLastThrowGrenadeCounter = GetOuterGrenadeThrowCounter();

	BaseClass::ClearAnimationState();

	ClearAnimationLayers();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CSOPlayerAnimState::TranslateActivity( Activity actDesired )
{
	Activity translateActivity = BaseClass::TranslateActivity( actDesired );

	if ( GetSOPlayer()->GetActiveWeapon() )
	{
		translateActivity = GetSOPlayer()->GetActiveWeapon()->ActivityOverride( translateActivity, false );
	}

	return translateActivity;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSOPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CMultiPlayerAnimState::Update" );

	// Get the SO player.
	CSO_Player *pSOPlayer = GetSOPlayer();
	if ( !pSOPlayer )
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pSOPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		// Pose parameter - what direction are the player's legs running in.
		ComputePoseParam_MoveYaw( pStudioHdr );

		// Pose parameter - Torso aiming (up/down).
		ComputePoseParam_AimPitch( pStudioHdr );

		// Pose parameter - Torso aiming (rotation).
		ComputePoseParam_AimYaw( pStudioHdr );
	}

#ifdef CLIENT_DLL 
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		m_pSOPlayer->SetPlaybackRate( 1.0f );
	}
#endif
}

extern ConVar mp_slammoveyaw;
float SnapYawTo( float flValue );
void CSOPlayerAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
	// Get the estimated movement yaw.
	EstimateYaw();

	// Get the view yaw.
	float flAngle = AngleNormalize( m_flEyeYaw );

	// Calc side to side turning - the view vs. movement yaw.
	float flYaw = flAngle - m_PoseParameterData.m_flEstimateYaw;
	flYaw = AngleNormalize( -flYaw );

	// Get the current speed the character is running.
	bool bIsMoving;
	float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );

	// Setup the 9-way blend parameters based on our speed and direction.
	Vector2D vecCurrentMoveYaw( 0.0f, 0.0f );
	if ( bIsMoving )
	{
		if ( mp_slammoveyaw.GetBool() )
		{
			flYaw = SnapYawTo( flYaw );
		}
		vecCurrentMoveYaw.x = cos( DEG2RAD( flYaw ) ) * flPlaybackRate;
		vecCurrentMoveYaw.y = -sin( DEG2RAD( flYaw ) ) * flPlaybackRate;
	}

	// Set the 9-way blend movement pose parameters.
	GetSOPlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x );

	// Noooooo Tony! Don't flip it!!!
	// Do not negate here (at least not for CS:S player animations)!!!
	//GetSOPlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, -vecCurrentMoveYaw.y ); //Tony; flip it
	GetSOPlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y ); //Tony; flip it

	m_DebugAnimData.m_vecMoveYaw = vecCurrentMoveYaw;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSOPlayerAnimState::ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr )
{
	// Get the view pitch.
	float flAimPitch = m_flEyePitch;

	// Set the aim pitch pose parameter and save.

	// Do not negate here (at least not for CS:S player animations)!!!
	//GetSOPlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimPitch, -flAimPitch );
	GetSOPlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimPitch, flAimPitch );

	m_DebugAnimData.m_flAimPitch = flAimPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CSOPlayerAnimState::ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr )
{
	// Get the movement velocity.
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Check to see if we are moving.
	bool bMoving = ( vecVelocity.Length() > 1.0f ) ? true : false;

	// If we are moving or are prone and undeployed.
	if ( bMoving || m_bForceAimYaw )
	{
		// The feet match the eye direction when moving - the move yaw takes care of the rest.
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	// Else if we are not moving.
	else
	{
		// Initialize the feet.
		if ( m_PoseParameterData.m_flLastAimTurnTime <= 0.0f )
		{
			m_flGoalFeetYaw	= m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		}
		// Make sure the feet yaw isn't too far out of sync with the eye yaw.
		// TODO: Do something better here!
		else
		{
			float flYawDelta = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

			if ( fabs( flYawDelta ) > 45.0f/*m_AnimConfig.m_flMaxBodyYawDegrees*/ )
			{
				float flSide = ( flYawDelta > 0.0f ) ? -1.0f : 1.0f;
				m_flGoalFeetYaw += ( 45.0f/*m_AnimConfig.m_flMaxBodyYawDegrees*/ * flSide );
			}
		}
	}

	// Fix up the feet yaw.
	m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );
	if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
	{
		if ( m_bForceAimYaw )
		{
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
		}
		else
		{
			ConvergeYawAngles( m_flGoalFeetYaw, /*DOD_BODYYAW_RATE*/720.0f, gpGlobals->frametime, m_flCurrentFeetYaw );
			m_flLastAimTurnTime = gpGlobals->curtime;
		}
	}

	// Rotate the body into position.
	m_angRender[YAW] = m_flCurrentFeetYaw;

	// Find the aim(torso) yaw base on the eye and feet yaws.
	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize( flAimYaw );

	// Set the aim yaw and save.

	// Do not negate here (at least not for CS:S player animations)!!!
	//GetSOPlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, -flAimYaw );
	GetSOPlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, flAimYaw );

	m_DebugAnimData.m_flAimYaw	= flAimYaw;

	// Turn off a force aim yaw - either we have already updated or we don't need to.
	m_bForceAimYaw = false;

#ifndef CLIENT_DLL
	QAngle angle = GetSOPlayer()->GetAbsAngles();
	angle[YAW] = m_flCurrentFeetYaw;

	GetSOPlayer()->SetAbsAngles( angle );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CSOPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	Activity iGestureActivity = ACT_INVALID;

	if ( (event == PLAYERANIMEVENT_ATTACK_PRIMARY) || (event == PLAYERANIMEVENT_ATTACK_SECONDARY) )	// may want to split these if we add a secondary attack to any weapons
	{
		m_iFireSequence = CalcFireLayerSequence( event );
		if ( m_iFireSequence > 0 )
		{
			m_bFiring = true;
			m_flFireCycle = 0;
		}

		iGestureActivity = ACT_VM_PRIMARYATTACK;
	}
	else if ( event == PLAYERANIMEVENT_JUMP )
	{
		// Play the jump animation.
		m_bJumping = true;
		m_bFirstJumpFrame = true;
		m_flJumpStartTime = gpGlobals->curtime;

		RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_HOP );
	}
	else if ( (event == PLAYERANIMEVENT_RELOAD) || (event == PLAYERANIMEVENT_RELOAD_LOOP) || (event == PLAYERANIMEVENT_RELOAD_END) )
	{	// PLAYERANIMEVENT_RELOAD_LOOP and PLAYERANIMEVENT_RELOAD_END are used exclusively by shotguns at the moment...I think
		m_iReloadSequence = CalcReloadLayerSequence( event );
		if ( m_iReloadSequence > 0 )
		{
			m_bReloading = true;
			m_flReloadCycle = 0;
		}

		if ( event == PLAYERANIMEVENT_RELOAD )
			iGestureActivity = ACT_VM_RELOAD;	// should not be needed for PLAYERANIMEVENT_RELOAD_LOOP or PLAYERANIMEVENT_RELOAD_END (at least not for shotguns)
	}
	else
	{
		switch ( event )
		{
		case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
			{
				if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );
				}
				iGestureActivity = ACT_VM_IDLE; //TODO?
				break;
			}
		case PLAYERANIMEVENT_ATTACK_PRE:
			{
				if ( m_pSOPlayer->GetFlags() & FL_DUCKING ) 
				{
					// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
					iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
				}
				else
				{
					// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
					iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
				}

				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, false );
				iGestureActivity = ACT_VM_IDLE; //TODO?

				break;
			}
		case PLAYERANIMEVENT_ATTACK_POST:
			{
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_POSTFIRE );
				iGestureActivity = ACT_VM_IDLE; //TODO?
				break;
			}
		case PLAYERANIMEVENT_RELOAD_LOOP:
			{
				// Weapon reload.
				if ( GetSOPlayer()->GetFlags() & FL_DUCKING )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_LOOP );
				}
				else
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_LOOP );
				}
				iGestureActivity = ACT_INVALID; //TODO: fix
				break;
			}
		case PLAYERANIMEVENT_RELOAD_END:
			{
				// Weapon reload.
				if ( GetSOPlayer()->GetFlags() & FL_DUCKING )
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_END );
				}
				else
				{
					RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_END );
				}
				iGestureActivity = ACT_INVALID; //TODO: fix
				break;
			}
		default:
			{
				BaseClass::DoAnimationEvent( event, nData );
				break;
			}
		}
	}

#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if ( iGestureActivity != ACT_INVALID && GetSOPlayer() != CSO_Player::GetLocalSOPlayer())
	{
		CBaseCombatWeapon *pWeapon = GetSOPlayer()->GetActiveWeapon();
		if ( pWeapon )
		{
			pWeapon->EnsureCorrectRenderingModel();
			pWeapon->SendWeaponAnim( iGestureActivity );
			// Force animation events!
			pWeapon->ResetEventsParity();		// reset event parity so the animation events will occur on the weapon. 
			pWeapon->DoAnimationEvents( pWeapon->GetModelPtr() );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Overriding CMultiplayerAnimState to add prone and sprinting checks as necessary.
// Input  :  - 
// Output : Activity
//-----------------------------------------------------------------------------
#ifdef CLIENT_DLL
extern ConVar anim_showmainactivity;
#endif

Activity CSOPlayerAnimState::CalcMainActivity()
{
	Activity idealActivity = ACT_IDLE;

	if ( HandleJumping( idealActivity ) ||
		 HandleDucking( idealActivity ) ||
		 HandleSwimming( idealActivity ) ||
		 HandleDying( idealActivity ) )
	{
		// intentionally blank
	}
	else
	{
		HandleMoving( idealActivity );
	}

	ShowDebugInfo();

	// Client specific.
#ifdef CLIENT_DLL

	if ( anim_showmainactivity.GetBool() )
	{
		DebugShowActivity( idealActivity );
	}

#endif

	return idealActivity;
}

int CSOPlayerAnimState::CalcAimLayerSequence( bool bForceIdle )
{
	// Figure out the weapon suffix.
	CWeaponSOBase *pWeapon = dynamic_cast<CWeaponSOBase*>( GetSOPlayer()->GetActiveWeapon() );
	if ( !pWeapon )
		return 0;

	const char *pSuffix = pWeapon->GetWeaponSuffix();
	if ( !pSuffix )
		return 0;

	if ( bForceIdle )
	{
		switch ( GetCurrentMainActivity() )
		{
		case ACT_CROUCHIDLE:
			return CalcSequenceIndex( "%s%s", DEFAULT_AIM_CROUCH_IDLE_NAME, pSuffix );

		default:
			return CalcSequenceIndex( "%s%s", DEFAULT_AIM_IDLE_NAME, pSuffix );
		}
	}
	else
	{
		switch ( GetCurrentMainActivity() )
		{
		case ACT_RUN:
			return CalcSequenceIndex( "%s%s", DEFAULT_AIM_RUN_NAME, pSuffix );

		case ACT_WALK:
		case ACT_RUNTOIDLE:
		case ACT_IDLETORUN:
			return CalcSequenceIndex( "%s%s", DEFAULT_AIM_WALK_NAME, pSuffix );

		case ACT_CROUCHIDLE:
			return CalcSequenceIndex( "%s%s", DEFAULT_AIM_CROUCH_IDLE_NAME, pSuffix );

		case ACT_RUN_CROUCH:
			return CalcSequenceIndex( "%s%s", DEFAULT_AIM_CROUCH_WALK_NAME, pSuffix );

		case ACT_IDLE:
		default:
			return CalcSequenceIndex( "%s%s", DEFAULT_AIM_IDLE_NAME, pSuffix );
		}
	}
}

int CSOPlayerAnimState::CalcFireLayerSequence( PlayerAnimEvent_t event )
{
	// Figure out the weapon suffix.
	CWeaponSOBase *pWeapon = dynamic_cast<CWeaponSOBase*>( GetSOPlayer()->GetActiveWeapon() );
	if ( !pWeapon )
		return 0;

	const char *pSuffix = pWeapon->GetWeaponSuffix();
	if ( !pSuffix )
		return 0;

	// Don't rely on their weapon here because the player has usually switched to their 
	// pistol or rifle by the time the PLAYERANIMEVENT_THROW_GRENADE message gets to the client.
	if ( event == PLAYERANIMEVENT_ATTACK_GRENADE )
		pSuffix = "Gren"; 

	switch ( GetCurrentMainActivity() )
	{
	case ACT_PLAYER_RUN_FIRE:
	case ACT_RUN:
		return CalcSequenceIndex( "%s%s", DEFAULT_FIRE_RUN_NAME, pSuffix );

	case ACT_PLAYER_WALK_FIRE:
	case ACT_WALK:
		return CalcSequenceIndex( "%s%s", DEFAULT_FIRE_WALK_NAME, pSuffix );

	case ACT_PLAYER_CROUCH_FIRE:
	case ACT_CROUCHIDLE:
		return CalcSequenceIndex( "%s%s", DEFAULT_FIRE_CROUCH_NAME, pSuffix );

	case ACT_PLAYER_CROUCH_WALK_FIRE:
	case ACT_RUN_CROUCH:
		return CalcSequenceIndex( "%s%s", DEFAULT_FIRE_CROUCH_WALK_NAME, pSuffix );

	default:
	case ACT_PLAYER_IDLE_FIRE:
		return CalcSequenceIndex( "%s%s", DEFAULT_FIRE_IDLE_NAME, pSuffix );
	}
}

int CSOPlayerAnimState::CalcReloadLayerSequence( PlayerAnimEvent_t event )
{
	// Figure out the weapon suffix.
	CWeaponSOBase *pWeapon = dynamic_cast<CWeaponSOBase*>( GetSOPlayer()->GetActiveWeapon() );
	if ( !pWeapon )
		return 0;

	const char *pSuffix = pWeapon->GetWeaponSuffix();
	if ( !pSuffix )
		return 0;

	// The CS:S shotgun reloading player animations are really stup-...I mean intuitive...and use a special animation set (grr)
	// We account for this using various event conditionals and such
	// Just make sure any shotguns you want using this solution call the appropriate player events at the right times (your setup might be different than the example below)
	// EX: StartReload() should call PLAYERANIMEVENT_RELOAD, Reload() should call PLAYERANIMEVENT_RELOAD_LOOP, FinishReload() should call PLAYERANIMEVENT_RELOAD_END, etc

	switch ( GetCurrentMainActivity() )
	{
	case ACT_PLAYER_RUN_FIRE:
	case ACT_RUN:
		{
			if ( event == PLAYERANIMEVENT_RELOAD_LOOP )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_RUN_NAME, pSuffix, "_loop" );
			}
			else if ( event == PLAYERANIMEVENT_RELOAD_END )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_RUN_NAME, pSuffix, "_end" );
			}
			else	// PLAYERANIMEVENT_RELOAD
			{
				int normReloadNum = CalcSequenceIndex( "%s%s", DEFAULT_RELOAD_RUN_NAME, pSuffix );
				if ( normReloadNum > 0 )
					return normReloadNum;
				else	// shotguns (they handle this event differently)
					return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_RUN_NAME, pSuffix, "_start" );
			}
		}

	case ACT_PLAYER_WALK_FIRE:
	case ACT_WALK:
		{
			if ( event == PLAYERANIMEVENT_RELOAD_LOOP )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_WALK_NAME, pSuffix, "_loop" );
			}
			else if ( event == PLAYERANIMEVENT_RELOAD_END )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_WALK_NAME, pSuffix, "_end" );
			}
			else	// PLAYERANIMEVENT_RELOAD
			{
				int normReloadNum = CalcSequenceIndex( "%s%s", DEFAULT_RELOAD_WALK_NAME, pSuffix );
				if ( normReloadNum > 0 )
					return normReloadNum;
				else	// shotguns (they handle this event differently)
					return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_WALK_NAME, pSuffix, "_start" );
			}
		}

	case ACT_PLAYER_CROUCH_FIRE:
	case ACT_CROUCHIDLE:
		{
			if ( event == PLAYERANIMEVENT_RELOAD_LOOP )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_CROUCH_NAME, pSuffix, "_loop" );
			}
			else if ( event == PLAYERANIMEVENT_RELOAD_END )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_CROUCH_NAME, pSuffix, "_end" );
			}
			else	// PLAYERANIMEVENT_RELOAD
			{
				int normReloadNum = CalcSequenceIndex( "%s%s", DEFAULT_RELOAD_CROUCH_NAME, pSuffix );
				if ( normReloadNum > 0 )
					return normReloadNum;
				else	// shotguns (they handle this event differently)
					return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_CROUCH_NAME, pSuffix, "_start" );
			}
		}

	case ACT_PLAYER_CROUCH_WALK_FIRE:
	case ACT_RUN_CROUCH:
		{
			if ( event == PLAYERANIMEVENT_RELOAD_LOOP )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_CROUCH_WALK_NAME, pSuffix, "_loop" );
			}
			else if ( event == PLAYERANIMEVENT_RELOAD_END )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_CROUCH_WALK_NAME, pSuffix, "_end" );
			}
			else	// PLAYERANIMEVENT_RELOAD
			{
				int normReloadNum = CalcSequenceIndex( "%s%s", DEFAULT_RELOAD_CROUCH_WALK_NAME, pSuffix );
				if ( normReloadNum > 0 )
					return normReloadNum;
				else	// shotguns (they handle this event differently)
					return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_CROUCH_WALK_NAME, pSuffix, "_start" );
			}
		}

	default:
	case ACT_PLAYER_IDLE_FIRE:
		{
			if ( event == PLAYERANIMEVENT_RELOAD_LOOP )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_IDLE_NAME, pSuffix, "_loop" );
			}
			else if ( event == PLAYERANIMEVENT_RELOAD_END )	// shotguns
			{
				return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_IDLE_NAME, pSuffix, "_end" );
			}
			else	// PLAYERANIMEVENT_RELOAD
			{
				int normReloadNum = CalcSequenceIndex( "%s%s", DEFAULT_RELOAD_IDLE_NAME, pSuffix );
				if ( normReloadNum > 0 )
					return normReloadNum;
				else	// shotguns (they handle this event differently)
					return CalcSequenceIndex( "%s%s%s", DEFAULT_RELOAD_IDLE_NAME, pSuffix, "_start" );
			}
		}
	}
}

int CSOPlayerAnimState::CalcSequenceIndex( const char *pBaseName, ... )
{
	char szFullName[512];
	va_list marker;
	va_start( marker, pBaseName );
	Q_vsnprintf( szFullName, sizeof( szFullName ), pBaseName, marker );
	va_end( marker );
	int iSequence = GetSOPlayer()->LookupSequence( szFullName );
	
	// Show warnings if we can't find anything here.
	if ( iSequence == -1 )
	{
		static CUtlDict<int,int> dict;
		if ( dict.Find( szFullName ) == -1 )
			dict.Insert( szFullName, 0 );

		iSequence = 0;
	}

	return iSequence;
}

bool CSOPlayerAnimState::ShouldBlendAimSequenceToIdle()
{
	Activity act = GetCurrentMainActivity();

	return (act == ACT_RUN || act == ACT_WALK || act == ACT_RUNTOIDLE || act == ACT_RUN_CROUCH);
}

void CSOPlayerAnimState::ComputeAimSequence()
{
	VPROF( "CBasePlayerAnimState::ComputeAimSequence" );

	// Synchronize the lower and upper body cycles.
	float flCycle = GetSOPlayer()->GetCycle();

	// Figure out the new cycle time.
	UpdateAimSequenceLayers( flCycle, AIMSEQUENCE_LAYER, true, &m_IdleSequenceTransitioner, 1 );
	
	if ( ShouldBlendAimSequenceToIdle() )
	{
		// What we do here is blend between the idle upper body animation (like where he's got the dual elites
		// held out in front of him but he's not moving) and his walk/run/crouchrun upper body animation,
		// weighting it based on how fast he's moving. That way, when he's moving slowly, his upper 
		// body doesn't jiggle all around.
		bool bIsMoving;
		float flPlaybackRate = CalcMovementPlaybackRate( &bIsMoving );
		if ( bIsMoving )
			UpdateAimSequenceLayers( flCycle, AIMSEQUENCE_LAYER+2, false, &m_SequenceTransitioner, flPlaybackRate );
	}

	OptimizeLayerWeights( AIMSEQUENCE_LAYER, NUM_AIMSEQUENCE_LAYERS );
}

void CSOPlayerAnimState::UpdateAimSequenceLayers(
	float flCycle,
	int iFirstLayer,
	bool bForceIdle,
	CSequenceTransitioner *pTransitioner,
	float flWeightScale
	)
{
	float flAimSequenceWeight = 1;
	int iAimSequence = CalcAimLayerSequence( bForceIdle );
	if ( iAimSequence == -1 )
		iAimSequence = 0;

	// Feed the current state of the animation parameters to the sequence transitioner.
	// It will hand back either 1 or 2 animations in the queue to set, depending on whether
	// it's transitioning or not. We just dump those into the animation layers.
	pTransitioner->CheckForSequenceChange(
		GetSOPlayer()->GetModelPtr(),
		iAimSequence,
		false,	// don't force transitions on the same anim
		true	// yes, interpolate when transitioning
		);

	pTransitioner->UpdateCurrent(
		GetSOPlayer()->GetModelPtr(),
		iAimSequence,
		flCycle,
		GetSOPlayer()->GetPlaybackRate(),
		gpGlobals->curtime
		);

	CAnimationLayer *pDest0 = GetSOPlayer()->GetAnimOverlay( iFirstLayer );
	CAnimationLayer *pDest1 = GetSOPlayer()->GetAnimOverlay( iFirstLayer+1 );

	if ( pTransitioner->m_animationQueue.Count() == 1 )
	{
		// If only 1 animation, then blend it in fully.
		CAnimationLayer *pSource0 = &pTransitioner->m_animationQueue[0];
		*pDest0 = *pSource0;
		
		pDest0->m_flWeight = 1;
		pDest1->m_flWeight = 0;
		pDest0->m_nOrder = iFirstLayer;

#ifndef CLIENT_DLL
		pDest0->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	}
	else if ( pTransitioner->m_animationQueue.Count() >= 2 )
	{
		// The first one should be fading out. Fade in the new one inversely.
		CAnimationLayer *pSource0 = &pTransitioner->m_animationQueue[0];
		CAnimationLayer *pSource1 = &pTransitioner->m_animationQueue[1];

		*pDest0 = *pSource0;
		*pDest1 = *pSource1;
		Assert( pDest0->m_flWeight >= 0.0f && pDest0->m_flWeight <= 1.0f );
		pDest1->m_flWeight = 1 - pDest0->m_flWeight;	// This layer just mirrors the other layer's weight (one fades in while the other fades out).

		pDest0->m_nOrder = iFirstLayer;
		pDest1->m_nOrder = iFirstLayer+1;

#ifndef CLIENT_DLL
		pDest0->m_fFlags |= ANIM_LAYER_ACTIVE;
		pDest1->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	}
	
	pDest0->m_flWeight *= flWeightScale * flAimSequenceWeight;
	pDest0->m_flWeight = clamp( pDest0->m_flWeight, 0.0f, 1.0f );

	pDest1->m_flWeight *= flWeightScale * flAimSequenceWeight;
	pDest1->m_flWeight = clamp( pDest1->m_flWeight, 0.0f, 1.0f );

	pDest0->m_flCycle = pDest1->m_flCycle = flCycle;
}

void CSOPlayerAnimState::OptimizeLayerWeights( int iFirstLayer, int nLayers )
{
	int i;

	// Find the total weight of the blended layers, not including the idle layer (iFirstLayer)
	float totalWeight = 0.0f;
	for ( i=1; i < nLayers; i++ )
	{
		CAnimationLayer *pLayer = GetSOPlayer()->GetAnimOverlay( iFirstLayer+i );
		if ( pLayer->IsActive() && pLayer->m_flWeight > 0.0f )
		{
			totalWeight += pLayer->m_flWeight;
		}
	}

	// Set the idle layer's weight to be 1 minus the sum of other layer weights
	CAnimationLayer *pLayer = GetSOPlayer()->GetAnimOverlay( iFirstLayer );
	if ( pLayer->IsActive() && pLayer->m_flWeight > 0.0f )
	{
		pLayer->m_flWeight = 1.0f - totalWeight;
		pLayer->m_flWeight = max(pLayer->m_flWeight, 0.0f);
	}

	// This part is just an optimization. Since we have the walk/run animations weighted on top of 
	// the idle animations, all this does is disable the idle animations if the walk/runs are at
	// full weighting, which is whenever a guy is at full speed.
	//
	// So it saves us blending a couple animation layers whenever a guy is walking or running full speed.
	int iLastOne = -1;
	for ( i=0; i < nLayers; i++ )
	{
		CAnimationLayer *pLayer = GetSOPlayer()->GetAnimOverlay( iFirstLayer+i );
		if ( pLayer->IsActive() && pLayer->m_flWeight > 0.99 )
			iLastOne = i;
	}

	if ( iLastOne != -1 )
	{
		for ( int i=iLastOne-1; i >= 0; i-- )
		{
			CAnimationLayer *pLayer = GetSOPlayer()->GetAnimOverlay( iFirstLayer+i );
#ifdef CLIENT_DLL 
			pLayer->m_nOrder = CBaseAnimatingOverlay::MAX_OVERLAYS;
#else
			pLayer->m_nOrder.Set( CBaseAnimatingOverlay::MAX_OVERLAYS );
			pLayer->m_fFlags = 0;
#endif
		}
	}
}

void CSOPlayerAnimState::ComputeMainSequence()
{
	VPROF( "CBasePlayerAnimState::ComputeMainSequence" );

	CBaseAnimatingOverlay *pPlayer = GetBasePlayer();

	// Have our class or the mod-specific class determine what the current activity is.
	Activity idealActivity = CalcMainActivity();

#ifdef CLIENT_DLL
	Activity oldActivity = m_eCurrentMainSequenceActivity;
#endif
	
	// Store our current activity so the aim and fire layers know what to do.
	m_eCurrentMainSequenceActivity = idealActivity;

	// Hook to force playback of a specific requested full-body sequence
	if ( m_nSpecificMainSequence >= 0 )
	{
		if ( pPlayer->GetSequence() != m_nSpecificMainSequence )
		{
			pPlayer->ResetSequence( m_nSpecificMainSequence );
			ResetGroundSpeed();
			return;
		}
		 
		if ( !pPlayer->IsSequenceFinished() )
			return;

		m_nSpecificMainSequence = -1;
		RestartMainSequence();
		ResetGroundSpeed();
	}

	// Export to our outer class..
	int animDesired = SelectWeightedSequence( TranslateActivity( idealActivity ) );
	if ( pPlayer->GetSequenceActivity( pPlayer->GetSequence() ) == pPlayer->GetSequenceActivity( animDesired ) )
		return;

	if ( animDesired < 0 )
	{
		 animDesired = 0;
	}

	pPlayer->ResetSequence( animDesired );

#ifdef CLIENT_DLL
	// If we went from idle to walk, reset the interpolation history.
	// Kind of hacky putting this here.. it might belong outside the base class.
	if ( (oldActivity == ACT_CROUCHIDLE || oldActivity == ACT_IDLE || oldActivity == ACT_MP_DEPLOYED_IDLE || oldActivity == ACT_MP_CROUCH_DEPLOYED_IDLE ) && 
		 (idealActivity == ACT_WALK || idealActivity == ACT_RUN_CROUCH ) )
	{
		ResetGroundSpeed();
	}
#endif
}

bool CSOPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
	{
		if ( GetOuterXYSpeed() > MOVING_MINIMUM_SPEED )
		{
			idealActivity = ACT_RUN_CROUCH;
		}
		else
		{
			idealActivity = ACT_CROUCHIDLE;
		}
		
		return true;
	}
	
	return false;
}

bool CSOPlayerAnimState::HandleMoving( Activity &idealActivity )
{
	// In TF we run all the time now.
	float flSpeed = GetOuterXYSpeed();

	if ( flSpeed > MOVING_MINIMUM_SPEED )
	{
		// Always assume a run.
		idealActivity = ACT_RUN;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CSOPlayerAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
	VPROF( "CBasePlayerAnimState::ComputeSequences" );

	// Lower body (walk/run/idle).
	ComputeMainSequence();

	ComputeAimSequence();	// Upper body, based on weapon type.

	ComputeFireSequence( pStudioHdr );
	ComputeReloadSequence( pStudioHdr );
	ComputeGrenadeSequence( pStudioHdr );

	// The groundspeed interpolator uses the main sequence info.
	UpdateInterpolators();		
	ComputeGestureSequence( pStudioHdr );
}

void CSOPlayerAnimState::ClearAnimationLayers()
{
	if ( !GetSOPlayer() )
		return;

	GetSOPlayer()->SetNumAnimOverlays( NUM_LAYERS_WANTED );
	for ( int i=0; i < GetSOPlayer()->GetNumAnimOverlays(); i++ )
	{
		GetSOPlayer()->GetAnimOverlay( i )->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
	}
}

#ifdef CLIENT_DLL
void CSOPlayerAnimState::UpdateLayerSequenceGeneric( CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd )
{
	if ( !bEnabled )
		return;

	// Increment the fire sequence's cycle.
	flCurCycle += GetSOPlayer()->GetSequenceCycleRate( pStudioHdr, iSequence ) * gpGlobals->frametime;
	if ( flCurCycle > 1 )
	{
		if ( bWaitAtEnd )
		{
			flCurCycle = 1;
		}
		else
		{
			// Not firing anymore.
			bEnabled = false;
			iSequence = 0;
			return;
		}
	}

	// Now dump the state into its animation layer.
	C_AnimationLayer *pLayer = GetSOPlayer()->GetAnimOverlay( iLayer );

	pLayer->m_flCycle = flCurCycle;
	pLayer->m_nSequence = iSequence;

	pLayer->m_flPlaybackRate = 1.0;
	pLayer->m_flWeight = 1.0f;
	pLayer->m_nOrder = iLayer;
}
#endif

const float g_flThrowGrenadeFraction = 0.25;
bool CSOPlayerAnimState::IsThrowingGrenade()
{
	if ( m_bThrowingGrenade )
	{
		// An animation event would be more appropriate here.
		return m_flGrenadeCycle < g_flThrowGrenadeFraction;
	}
	else
	{
		bool bThrowPending = (m_iLastThrowGrenadeCounter != GetOuterGrenadeThrowCounter());
		return bThrowPending || IsOuterGrenadePrimed();
	}
}

bool CSOPlayerAnimState::IsOuterGrenadePrimed()
{
	CBaseCombatCharacter *pChar = GetSOPlayer()->MyCombatCharacterPointer();
	if ( pChar )
	{
		CWeaponFragGrenade *pGren = dynamic_cast<CWeaponFragGrenade*>( pChar->GetActiveWeapon() );
		return pGren && pGren->IsPrimed();
	}
	else
	{
		return NULL;
	}
}

void CSOPlayerAnimState::ComputeGrenadeSequence( CStudioHdr *pStudioHdr )
{
#ifdef CLIENT_DLL
	if ( m_bThrowingGrenade )
	{
		UpdateLayerSequenceGeneric( pStudioHdr, GRENADESEQUENCE_LAYER, m_bThrowingGrenade, m_flGrenadeCycle, m_iGrenadeSequence, false );
	}
	else
	{
		// Priming the grenade isn't an event.. we just watch the player for it.
		// Also play the prime animation first if he wants to throw the grenade.
		bool bThrowPending = (m_iLastThrowGrenadeCounter != GetOuterGrenadeThrowCounter());
		if ( IsOuterGrenadePrimed() || bThrowPending )
		{
			if ( !m_bPrimingGrenade )
			{
				// If this guy just popped into our PVS, and he's got his grenade primed, then
				// let's assume that it's all the way primed rather than playing the prime
				// animation from the start.
				if ( TimeSinceLastAnimationStateClear() < 0.4f )
					m_flGrenadeCycle = 1;
				else
					m_flGrenadeCycle = 0;
					
				m_iGrenadeSequence = CalcGrenadePrimeSequence();
			}

			m_bPrimingGrenade = true;
			UpdateLayerSequenceGeneric( pStudioHdr, GRENADESEQUENCE_LAYER, m_bPrimingGrenade, m_flGrenadeCycle, m_iGrenadeSequence, true );
			
			// If we're waiting to throw and we're done playing the prime animation...
			if ( bThrowPending && m_flGrenadeCycle == 1 )
			{
				m_iLastThrowGrenadeCounter = GetOuterGrenadeThrowCounter();

				// Now play the throw animation.
				m_iGrenadeSequence = CalcGrenadeThrowSequence();
				if ( m_iGrenadeSequence != -1 )
				{
					// Configure to start playing 
					m_bThrowingGrenade = true;
					m_bPrimingGrenade = false;
					m_flGrenadeCycle = 0;
				}
			}
		}
		else
		{
			m_bPrimingGrenade = false;
		}
	}
#endif
}

int CSOPlayerAnimState::CalcGrenadePrimeSequence()
{
	return CalcSequenceIndex( "idle_shoot_gren1" );
}

int CSOPlayerAnimState::CalcGrenadeThrowSequence()
{
	return CalcSequenceIndex( "idle_shoot_gren2" );
}

int CSOPlayerAnimState::GetOuterGrenadeThrowCounter()
{
	// Get the SO player.
	CSO_Player *pPlayer = GetSOPlayer();
	if ( pPlayer )
		return pPlayer->m_iThrowGrenadeCounter;
	else
		return 0;
}

float CSOPlayerAnimState::TimeSinceLastAnimationStateClear() const
{
	return gpGlobals->curtime - m_flLastAnimationStateClearTime;
}

void CSOPlayerAnimState::ComputeFireSequence( CStudioHdr *pStudioHdr )
{
#ifdef CLIENT_DLL
	UpdateLayerSequenceGeneric( pStudioHdr, FIRESEQUENCE_LAYER, m_bFiring, m_flFireCycle, m_iFireSequence, false );
#else
	// Server doesn't bother with different fire sequences.
#endif
}

void CSOPlayerAnimState::ComputeReloadSequence( CStudioHdr *pStudioHdr )
{
#ifdef CLIENT_DLL
	UpdateLayerSequenceGeneric( pStudioHdr, RELOADSEQUENCE_LAYER, m_bReloading, m_flReloadCycle, m_iReloadSequence, false );
#else
	// Server doesn't bother with different fire sequences.
#endif
}

bool CSOPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	if ( m_bJumping )
	{
		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		if ( gpGlobals->curtime - m_flJumpStartTime > 0.4f )
		{
			if ( GetSOPlayer()->GetFlags() & FL_ONGROUND )
			{
				m_bJumping = false;
				RestartMainSequence();	// Reset the animation.

				ResetGestureSlot( GESTURE_SLOT_ATTACK_AND_RELOAD );
			}
		}
	}

	// Are we still jumping? If so, keep playing the jump animation.
	return m_bJumping;
}

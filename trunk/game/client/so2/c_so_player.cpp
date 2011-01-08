//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "cbase.h"
#include "c_so_player.h"

// Add support for CS:S player animations
#include "c_basetempentity.h"
#include "prediction.h"

// Don't alias here
#if defined( CSO_Player )
#undef CSO_Player	
#endif

// Character customization system
ConVar cl_playermodel_headgear( "cl_playermodel_headgear", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Character customization: current headgear selection number" );
ConVar cl_playermodel_glasses( "cl_playermodel_glasses", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Character customization: current glasses selection number" );
ConVar cl_playermodel_commdevice( "cl_playermodel_commdevice", "0", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Character customization: current communications device selection number" );

LINK_ENTITY_TO_CLASS( player, C_SO_Player );

BEGIN_RECV_TABLE_NOBASE( C_SO_Player, DT_SOLocalPlayerExclusive )
END_RECV_TABLE()

BEGIN_RECV_TABLE_NOBASE( C_SO_Player, DT_SONonLocalPlayerExclusive )
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT( C_SO_Player, DT_SO_Player, CSO_Player )
	// Add support for CS:S player animations
	RecvPropInt( RECVINFO( m_iThrowGrenadeCounter ) ),

	// Do not allow players to fire weapons on ladders
	// http://articles.thewavelength.net/724/
	// Do not allow players to fire weapons while sprinting
	RecvPropEHandle( RECVINFO( m_hHolsteredWeapon ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_SO_Player )
END_PREDICTION_DATA()

C_SO_Player::C_SO_Player()
{
	// Add support for CS:S player animations
	m_SOPlayerAnimState = CreateSOPlayerAnimState( this );
}

C_SO_Player::~C_SO_Player()
{
	// Add support for CS:S player animations
	m_SOPlayerAnimState->Release();
}

C_SO_Player* C_SO_Player::GetLocalSOPlayer()
{
	return (C_SO_Player*)C_BasePlayer::GetLocalPlayer();
}

// Add support for CS:S player animations
const QAngle& C_SO_Player::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
		return m_SOPlayerAnimState->GetRenderAngles();
	}
}

// Add support for CS:S player animations
void C_SO_Player::UpdateClientSideAnimation()
{
	m_SOPlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );

	BaseClass::UpdateClientSideAnimation();
}

// Add support for CS:S player animations
CStudioHdr *C_SO_Player::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();

	InitializePoseParams();

	// Reset the players animation states, gestures
	if ( m_SOPlayerAnimState )
		m_SOPlayerAnimState->OnNewModel();

	return hdr;
}

// First-person ragdolls
// http://developer.valvesoftware.com/wiki/First_Person_Ragdolls
void C_SO_Player::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	// if we're dead, we want to deal with first or third person ragdolls.
	if ( m_lifeState != LIFE_ALIVE && !IsObserver() )
	{
		// First person ragdolls
		if ( m_hRagdoll.Get() )
		{
			// pointer to the ragdoll
			C_HL2MPRagdoll *pRagdoll = (C_HL2MPRagdoll*)m_hRagdoll.Get();

			// gets its origin and angles
			pRagdoll->GetAttachment( pRagdoll->LookupAttachment( "eyes" ), eyeOrigin, eyeAngles );
			Vector vForward; 
			AngleVectors( eyeAngles, &vForward );

			// DM: Don't use first person view when we are very close to something
			trace_t tr;
			UTIL_TraceLine( eyeOrigin, eyeOrigin + ( vForward * 10000 ), MASK_ALL, pRagdoll, COLLISION_GROUP_NONE, &tr );

			if ( (!(tr.fraction < 1) || (tr.endpos.DistTo(eyeOrigin) > 25)) )
				return;
		}

		eyeOrigin = vec3_origin;
		eyeAngles = vec3_angle;

		Vector origin = EyePosition();
		IRagdoll *pRagdoll = GetRepresentativeRagdoll();
		if ( pRagdoll )
		{
			origin = pRagdoll->GetRagdollOrigin();
			origin.z += VEC_DEAD_VIEWHEIGHT.z; // look over ragdoll, not through
		}

		BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );

		eyeOrigin = origin;
		Vector vForward; 
		AngleVectors( eyeAngles, &vForward );
		VectorNormalize( vForward );
		VectorMA( origin, -CHASE_CAM_DISTANCE, vForward, eyeOrigin );
		Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
		Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );
		trace_t trace; // clip against world

		// HACK don't recompute positions while doing RayTrace
		C_BaseEntity::EnableAbsRecomputations( false );
		UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );
		C_BaseEntity::EnableAbsRecomputations( true );
		
		if ( trace.fraction < 1.0 )
			eyeOrigin = trace.endpos;

		return;
	}

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

// Add support for CS:S player animations
// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //
class C_TEPlayerAnimEvent_SO : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent_SO, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		// Create the effect.
		C_SO_Player *pPlayer = dynamic_cast< C_SO_Player* >( m_hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
		}	
	}

public:
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent_SO, DT_TEPlayerAnimEvent_SO, CTEPlayerAnimEvent_SO );

BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent_SO, DT_TEPlayerAnimEvent_SO )
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()

// Add support for CS:S player animations
void C_SO_Player::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() )
	{
		if ( ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();

	// Add support for CS:S player animations
	if ( event == PLAYERANIMEVENT_ATTACK_GRENADE )
	{
		// Let the server handle this event. It will update m_iThrowGrenadeCounter and the client will
		// pick up the event in CCSPlayerAnimState.
	}
	else
	{
		m_SOPlayerAnimState->DoAnimationEvent( event, nData );
	}
}

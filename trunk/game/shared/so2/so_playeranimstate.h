// Add support for CS:S player animations
// This file should at least somewhat resemble sdk_playeranimstate.h because it is originally based on that file

#ifndef SO_PLAYERANIMSTATE_H
#define SO_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "convar.h"
#include "multiplayer_animstate.h"
#include "sequence_Transitioner.h"

#if defined( CLIENT_DLL )
class C_SO_Player;
#define CSO_Player C_SO_Player
#else
class CSO_Player;
#endif

// ------------------------------------------------------------------------------------------------ //
// CPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CSOPlayerAnimState : public CMultiPlayerAnimState
{
public:
	
	DECLARE_CLASS( CSOPlayerAnimState, CMultiPlayerAnimState );

	CSOPlayerAnimState();
	CSOPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	~CSOPlayerAnimState();

	void InitSOAnimState( CSO_Player *pPlayer );
	CSO_Player *GetSOPlayer( void ) { return m_pSOPlayer; }

	void ClearAnimationState();
	Activity TranslateActivity( Activity actDesired );
	void Update( float eyeYaw, float eyePitch );

	void	DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	void ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	void ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	void ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );

	virtual Activity CalcMainActivity();

	void ComputeMainSequence();
	bool HandleDucking( Activity &idealActivity );
	bool HandleMoving( Activity &idealActivity );
	bool HandleJumping( Activity &idealActivity );

	bool IsThrowingGrenade();

private:
	
	CSO_Player   *m_pSOPlayer;
	bool		m_bInAirWalk;

	float		m_flHoldDeployedPoseUntilTime;

	int CalcAimLayerSequence( bool bForceIdle );
	int CalcFireLayerSequence( PlayerAnimEvent_t event );
	int CalcReloadLayerSequence( PlayerAnimEvent_t event );
	int CalcSequenceIndex( const char *pBaseName, ... );

	bool ShouldBlendAimSequenceToIdle( void );
	void ComputeAimSequence( void );
	void UpdateAimSequenceLayers( float flCycle, int iFirstLayer, bool bForceIdle, CSequenceTransitioner *pTransitioner, float flWeightScale );
	void OptimizeLayerWeights( int iFirstLayer, int nLayers );
	void ComputeSequences( CStudioHdr *pStudioHdr );

	bool IsOuterGrenadePrimed();
	void ComputeGrenadeSequence( CStudioHdr *pStudioHdr );
	int CalcGrenadePrimeSequence();
	int CalcGrenadeThrowSequence();
	int GetOuterGrenadeThrowCounter();
	float TimeSinceLastAnimationStateClear() const;

	void ComputeFireSequence( CStudioHdr *pStudioHdr );
	void ComputeReloadSequence( CStudioHdr *pStudioHdr );

#ifdef CLIENT_DLL
	void UpdateLayerSequenceGeneric( CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd );
#endif

	void ClearAnimationLayers();

	// This gives us smooth transitions between aim anim sequences on the client.
	CSequenceTransitioner m_IdleSequenceTransitioner;
	CSequenceTransitioner m_SequenceTransitioner;
	
	// This is set to true if ANY animation is being played in the fire layer.
	bool m_bFiring;						// If this is on, then it'll continue the fire animation in the fire layer
										// until it completes.
	int m_iFireSequence;				// (For any sequences in the fire layer, including grenade throw).
	float m_flFireCycle;

	// Aim sequence plays reload while this is on.
	bool m_bReloading;
	float m_flReloadCycle;
	int m_iReloadSequence;

	// These control grenade animations.
	bool m_bThrowingGrenade;
	bool m_bPrimingGrenade;
	float m_flGrenadeCycle;
	int m_iGrenadeSequence;
	int m_iLastThrowGrenadeCounter;	// used to detect when the guy threw the grenade.
};

CSOPlayerAnimState *CreateSOPlayerAnimState( CSO_Player *pPlayer );

#endif // SO_PLAYERANIMSTATE_H

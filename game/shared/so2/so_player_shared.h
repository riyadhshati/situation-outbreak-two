#ifndef SO_PLAYER_SHARED_H
#define SO_PLAYER_SHARED_H
#pragma once

#include "hl2mp_player_shared.h"
#include "weapon_sobase.h"

#if defined( CLIENT_DLL )
#define CSO_Player C_SO_Player
#endif

// Base each player's speed on their health
// This is the one place where player speeds are defined now
// Doesn't that make everything easier!? =D
#define	SO_WALK_SPEED 150
#define	SO_NORM_SPEED 200
#define	SO_SPRINT_SPEED 250

// Character customization system
// I know things are out of order in this enum, but I like this order more than the one in the player model qcs
// Just changing the order here to match the player model qc might cause some confusion, too
enum
{
								// name of bodygroup in player model qc

	BODYGROUP_HEADGEAR = 2,		// "headgear"
	BODYGROUP_GLASSES = 3,		// "glasses"
	BODYGROUP_COMMDEVICE = 4,	// "commdev"
};

// Character customization system
// As we add more customization options, these will need to be changed
static const int BODYGROUP_HEADGEAR_MAX = 3;
static const int BODYGROUP_GLASSES_MAX = 2;
static const int BODYGROUP_COMMDEVICE_MAX = 3;

#endif //SO_PLAYER_SHARED_H

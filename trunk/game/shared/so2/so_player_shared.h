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

#endif //SO_PLAYER_SHARED_H

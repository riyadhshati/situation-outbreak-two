// Client-side support for class identification

#include "cbase.h"
#include "c_ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_NPC_Creeper : public C_AI_BaseNPC
{
public:
	DECLARE_CLASS( C_NPC_Creeper, C_AI_BaseNPC );
	DECLARE_CLIENTCLASS();

	C_NPC_Creeper();
	virtual ~C_NPC_Creeper();

private:
	C_NPC_Creeper( const C_NPC_Creeper & ); // not defined, not accessible
};

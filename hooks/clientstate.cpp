#include "includes.h"

bool Hooks::TempEntities( void *msg ) {
	if( !g_cl.m_processing ) {
		return g_hooks.m_client_state.GetOldMethod< TempEntities_t >( CClientState::TEMPENTITIES )( this, msg );
	}

	const bool ret = g_hooks.m_client_state.GetOldMethod< TempEntities_t >( CClientState::TEMPENTITIES )( this, msg );

	if( !g_csgo.m_cl ) {
		return ret;
	}

	for( CEventInfo *ei{ g_csgo.m_cl->m_events }; ei != nullptr; ei = ei->m_next ) {
		ei->m_fire_delay = 0.0f;
	}

	return ret;
}

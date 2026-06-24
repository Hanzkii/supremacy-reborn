#include "includes.h"

Resolver g_resolver{};;

LagRecord* Resolver::FindIdealRecord( AimPlayer* data ) {
    LagRecord *first_valid, *current;

	if( data->m_records.empty( ) )
		return nullptr;

    first_valid = nullptr;

    // iterate records.
	for( const auto &it : data->m_records ) {
		if( it->dormant( ) || it->immune( ) || !it->valid( ) )
			continue;

        // get current record.
        current = it.get( );

        // first record that was valid, store it for later.
        if( !first_valid )
            first_valid = current;

        // try to find a record with a shot, lby update, walking or no anti-aim.
		if( it->m_shot || it->m_mode == Modes::RESOLVE_BODY || it->m_mode == Modes::RESOLVE_WALK || it->m_mode == Modes::RESOLVE_NONE )
            return current;
	}

	// none found above, return the first valid record if possible.
	return ( first_valid ) ? first_valid : nullptr;
}

LagRecord* Resolver::FindLastRecord( AimPlayer* data ) {
    LagRecord* current;

	if( data->m_records.empty( ) )
		return nullptr;

	// iterate records in reverse.
	for( auto it = data->m_records.crbegin( ); it != data->m_records.crend( ); ++it ) {
		current = it->get( );

		// if this record is valid.
		// we are done since we iterated in reverse.
		if( current->valid( ) && !current->immune( ) && !current->dormant( ) )
			return current;
	}

	return nullptr;
}

void Resolver::OnBodyUpdate( Player* player, float value ) {
	AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

	// set data.
	data->m_old_body = data->m_body;
	data->m_body     = value;
}

float Resolver::GetAwayAngle( LagRecord* record ) {
	float  delta{ std::numeric_limits< float >::max( ) };
	vec3_t pos;
	ang_t  away;

	// other cheats predict you by their own latency.
	// they do this because, then they can put their away angle to exactly
	// where you are on the server at that moment in time.

	// the idea is that you would need to know where they 'saw' you when they created their user-command.
	// lets say you move on your client right now, this would take half of our latency to arrive at the server.
	// the delay between the server and the target client is compensated by themselves already, that is fortunate for us.

	// we have no historical origins.
	// no choice but to use the most recent one.
	//if( g_cl.m_net_pos.empty( ) ) {
		math::VectorAngles( g_cl.m_local->m_vecOrigin( ) - record->m_pred_origin, away );
		return away.y;
	//}

	// half of our rtt.
	// also known as the one-way delay.
	//float owd = ( g_cl.m_latency / 2.f );

	// since our origins are computed here on the client
	// we have to compensate for the delay between our client and the server
	// therefore the OWD should be subtracted from the target time.
	//float target = record->m_pred_time; //- owd;

	// iterate all.
	//for( const auto &net : g_cl.m_net_pos ) {
		// get the delta between this records time context
		// and the target time.
	//	float dt = std::abs( target - net.m_time );

		// the best origin.
	//	if( dt < delta ) {
	//		delta = dt;
	//		pos   = net.m_pos;
	//	}
	//}

	//math::VectorAngles( pos - record->m_pred_origin, away );
	//return away.y;
}

float Resolver::GetMaxDesync( LagRecord* record ) {
	// the animation state clamps the lower body to ~58-60 deg from the eye
	// yaw while standing; that window shrinks as the player runs.
	float speed = record->m_anim_velocity.length_2d( );

	// running scale ( 0 standing .. 1 at/above run speed ~260 ).
	float run = speed / 260.f;
	math::clamp( run, 0.f, 1.f );

	// 60 deg standing down to ~30 deg while moving.
	return 60.f - ( run * 30.f );
}

float Resolver::AnimationSide( AimPlayer* data, LagRecord* record ) {
	// delta-based signal: networked eye yaw vs the lower body yaw target.
	float lby_delta = math::NormalizedAngle( record->m_eye_angles.y - record->m_body );

	// animation-based signal: the foot yaw the animstate settled on.
	float anim_delta = 0.f;
	CCSGOPlayerAnimState* state = data->m_player->m_PlayerAnimState( );
	if( state )
		anim_delta = math::NormalizedAngle( record->m_eye_angles.y - state->m_goal_feet_yaw );

	// combine both signals; the sign indicates which way the body is turned.
	float sum = lby_delta + anim_delta;

	if( sum > 5.f )
		return 1.f;   // body turned to the right of the eye yaw.

	if( sum < -5.f )
		return -1.f;  // body turned to the left of the eye yaw.

	return 0.f;
}

void Resolver::ResolvePitch( AimPlayer* data, LagRecord* record ) {
	// the 3 pitches that fake-pitch anti-aims pin to: zero / fake-down / fake-up.
	static const float pitches[ 3 ] = { 0.f, 89.f, -89.f };

	bool nospread = g_menu.main.config.mode.get( ) == 1;

	// a faked pitch sits pinned at an extreme value.
	bool faked = std::abs( record->m_eye_angles.x ) > 85.f;

	// in matchmaking with a believable pitch, trust the networked value.
	if( !nospread && !faked )
		return;

	// bruteforce the 3 possible pitches, advanced by miss feedback.
	record->m_eye_angles.x = pitches[ ( ( data->m_pitch_index % 3 ) + 3 ) % 3 ];
}

bool Resolver::DetectRotation( AimPlayer* data, LagRecord* record, float& predicted ) {
	// raw networked yaw ( resolve hasn't overwritten it yet ).
	float raw = record->m_eye_angles.y;

	// per-tick change since the previous resolved record.
	float delta = math::NormalizedAngle( raw - data->m_last_eye_yaw );

	bool result = false;

	// spinning: a large, consistent same-direction change two ticks in a row.
	if( std::abs( delta ) > 22.f && std::abs( data->m_yaw_rate ) > 22.f
		&& ( delta * data->m_yaw_rate ) > 0.f ) {
		// predict where the spin continues to next tick.
		predicted = math::NormalizedAngle( raw + delta );
		result = true;
	}

	// remember the rate for the next tick.
	data->m_yaw_rate = delta;
	return result;
}

void Resolver::MatchShot( AimPlayer* data, LagRecord* record ) {
	// do not attempt to do this in nospread mode.
	if( g_menu.main.config.mode.get( ) == 1 )
		return;

	float shoot_time = -1.f;

	Weapon* weapon = data->m_player->GetActiveWeapon( );
	if( weapon ) {
		// with logging this time was always one tick behind.
		// so add one tick to the last shoot time.
		shoot_time = weapon->m_fLastShotTime( ) + g_csgo.m_globals->m_interval;
	}

	// this record has a shot on it.
	if( game::TIME_TO_TICKS( shoot_time ) == game::TIME_TO_TICKS( record->m_sim_time ) ) {
		if( record->m_lag <= 2 )
			record->m_shot = true;
		
		// more then 1 choke, cant hit pitch, apply prev pitch.
		else if( data->m_records.size( ) >= 2 ) {
			LagRecord* previous = data->m_records[ 1 ].get( );

			if( previous && !previous->dormant( ) )
				record->m_eye_angles.x = previous->m_eye_angles.x;
		}
	}
}

void Resolver::SetMode( LagRecord* record ) {
	// the resolver has 3 modes to chose from.
	// these modes will vary more under the hood depending on what data we have about the player
	// and what kind of hack vs. hack we are playing (mm/nospread).

	float speed = record->m_anim_velocity.length( );

	// if on ground, moving, and not fakewalking.
	if( ( record->m_flags & FL_ONGROUND ) && speed > 0.1f && !record->m_fake_walk )
		record->m_mode = Modes::RESOLVE_WALK;

	// if on ground, not moving or fakewalking.
	if( ( record->m_flags & FL_ONGROUND ) && ( speed <= 0.1f || record->m_fake_walk ) )
		record->m_mode = Modes::RESOLVE_STAND;

	// if not on ground.
	else if( !( record->m_flags & FL_ONGROUND ) )
		record->m_mode = Modes::RESOLVE_AIR;
}

void Resolver::ResolveAngles( Player* player, LagRecord* record ) {
	AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

	// mark this record if it contains a shot.
	MatchShot( data, record );

	// next up mark this record with a resolver mode that will be used.
	SetMode( record );

	// raw networked yaw before any resolver overwrites it ( rotation delta ).
	float raw_yaw = record->m_eye_angles.y;

	// resolve the pitch ( zero / fake-down / fake-up ) using miss feedback.
	// this replaces the old 'force pitch to 90' nospread behaviour and also
	// brute-forces faked pitches in matchmaking.
	ResolvePitch( data, record );

	// detect spinning / jitter anti-aim and predict its continuation.
	float predicted = 0.f;
	bool rotating = DetectRotation( data, record, predicted );

	// we arrived here we can do the acutal resolve.
	if( record->m_mode == Modes::RESOLVE_WALK ) 
		ResolveWalk( data, record );

	else if( record->m_mode == Modes::RESOLVE_STAND )
		ResolveStand( data, record );

	else if( record->m_mode == Modes::RESOLVE_AIR )
		ResolveAir( data, record );

	// a rotating player networks a moving real yaw; the brute resolvers
	// fight against that, so override them with the predicted continuation.
	if( rotating )
		record->m_eye_angles.y = predicted;

	// store the raw yaw for the next tick's rotation delta.
	data->m_last_eye_yaw = raw_yaw;

	// normalize the eye angles, doesn't really matter but its clean.
	math::NormalizeAngle( record->m_eye_angles.y );
}

void Resolver::ResolveWalk( AimPlayer* data, LagRecord* record ) {
	// apply lby to eyeangles.
	record->m_eye_angles.y = record->m_body;

	// delay body update.
	data->m_body_update = record->m_anim_time + 0.22f;

	// reset stand and body index.
	data->m_stand_index  = 0;
	data->m_stand_index2 = 0;
	data->m_body_index   = 0;

	// a walking player reveals real angles, so reset brute state.
	data->m_pitch_index  = 0;
	data->m_missed_shots = 0;

	// store the away angle for server-feedback learning.
	record->m_away = GetAwayAngle( record );

	// copy the last record that this player was walking
	// we need it later on because it gives us crucial data.
	std::memcpy( &data->m_walk_record, record, sizeof( LagRecord ) );
}

void Resolver::ResolveStand( AimPlayer* data, LagRecord* record ) {
	// for no-spread call a seperate resolver.
	if( g_menu.main.config.mode.get( ) == 1 ) {
		StandNS( data, record );
		return;
	}

	// get predicted away angle for the player and remember it for feedback.
	float away = GetAwayAngle( record );
	record->m_away = away;

	// max desync window for this record ( animation based ).
	float maxdesync = GetMaxDesync( record );

	// pointer for easy access.
	LagRecord* move = &data->m_walk_record;

	// we have a valid moving record.
	if( move->m_sim_time > 0.f ) {
		vec3_t delta = move->m_origin - record->m_origin;

		// check if moving record is close.
		if( delta.length( ) <= 128.f ) {
			// indicate that we are using the moving lby.
			data->m_moved = true;
		}
	}

	// a valid moving context was found
	if( data->m_moved ) {
		float delta = record->m_anim_time - move->m_anim_time;

		// it has not been time for this first update yet.
		if( delta < 0.22f ) {
			// set angles to current LBY.
			record->m_eye_angles.y = move->m_body;

			// set resolve mode.
			record->m_mode = Modes::RESOLVE_STOPPED_MOVING;

			// exit out of the resolver, thats it.
			return;
		}

		// LBY SHOULD HAVE UPDATED HERE.
		else if( record->m_anim_time >= data->m_body_update ) {
			// only shoot the LBY flick 3 times.
			// if we happen to miss then we most likely mispredicted.
			if( data->m_body_index <= 3 ) {
				// set angles to current LBY.
				record->m_eye_angles.y = record->m_body;

				// predict next body update.
				data->m_body_update = record->m_anim_time + 1.1f;

				// set the resolve mode.
				record->m_mode = Modes::RESOLVE_BODY;

				return;
			}

			// set to stand1 -> known last move.
			record->m_mode = Modes::RESOLVE_STAND1;
		}
	}

	// no moving context -> pure standing desync.
	if( record->m_mode != Modes::RESOLVE_STAND1 )
		record->m_mode = Modes::RESOLVE_STAND2;

	// base yaw we desync around: last-known LBY when we moved, else networked LBY.
	float base = ( record->m_mode == Modes::RESOLVE_STAND1 ) ? move->m_body : record->m_body;

	// server-based resolving: if a head hit taught us an offset and we have
	// not started missing, trust the learned angle first. the offset is stored
	// relative to the networked lby so it tracks the player.
	if( data->m_has_stand && data->m_missed_shots < 1 ) {
		record->m_eye_angles.y = record->m_body + data->m_prefer_stand;
		return;
	}

	// animation + delta based side seed.
	float seed = AnimationSide( data, record );
	int   side = ( seed >= 0.f ) ? 1 : -1;
	data->m_side = side;

	// shared brute index for stand1 ( known move ) and stand2 ( no move ).
	int idx = ( record->m_mode == Modes::RESOLVE_STAND1 )
		? data->m_stand_index : data->m_stand_index2;

	// desync / side bruteforce ordered by likelihood.
	switch( idx % 6 ) {
	case 0:
		// seeded side at max desync.
		record->m_eye_angles.y = base + side * maxdesync;
		break;

	case 1:
		// opposite side at max desync.
		record->m_eye_angles.y = base - side * maxdesync;
		break;

	case 2:
		// zero desync ( facing the lby ).
		record->m_eye_angles.y = base;
		break;

	case 3:
		// flipped lby ( switched after stopping ).
		record->m_eye_angles.y = base + 180.f;
		break;

	case 4:
		// away-based backwards.
		record->m_eye_angles.y = away + 180.f;
		break;

	case 5:
		// straight at us.
		record->m_eye_angles.y = away;
		break;

	default:
		break;
	}
}

void Resolver::StandNS( AimPlayer* data, LagRecord* record ) {
	// get away angles and remember them for server-feedback learning.
	float away = GetAwayAngle( record );
	record->m_away = away;

	switch( data->m_shots % 8 ) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = away + 90.f;
		break;
	case 2:
		record->m_eye_angles.y = away - 90.f;
		break;

	case 3:
		record->m_eye_angles.y = away + 45.f;
		break;
	case 4:
		record->m_eye_angles.y = away - 45.f;
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;
	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;

	case 7:
		record->m_eye_angles.y = away + 0.f;
		break;

	default:
		break;
	}

	// force LBY to not fuck any pose and do a true bruteforce.
	record->m_body = record->m_eye_angles.y;
}

void Resolver::ResolveAir( AimPlayer* data, LagRecord* record ) {
	// for no-spread call a seperate resolver.
	if( g_menu.main.config.mode.get( ) == 1 ) {
		AirNS( data, record );
		return;
	}

	// else run our matchmaking air resolver.

	// we have barely any speed. 
	// either we jumped in place or we just left the ground.
	// or someone is trying to fool our resolver.
	if( record->m_velocity.length_2d( ) < 60.f ) {
		// set this for completion.
		// so the shot parsing wont pick the hits / misses up.
		// and process them wrongly.
		record->m_mode = Modes::RESOLVE_STAND;

		// invoke our stand resolver.
		ResolveStand( data, record );

		// we are done.
		return;
	}

	// try to predict the direction of the player based on his velocity direction.
	// this should be a rough estimation of where he is looking.
	float velyaw = math::rad_to_deg( std::atan2( record->m_velocity.y, record->m_velocity.x ) );

	// store the reference angle for server-feedback learning.
	record->m_away = velyaw;

	// server-based resolving: trust a learned air offset before brute forcing.
	if( data->m_has_air && data->m_missed_shots < 1 ) {
		record->m_eye_angles.y = velyaw + data->m_prefer_air;
		return;
	}

	switch( data->m_shots % 3 ) {
	case 0:
		record->m_eye_angles.y = velyaw + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = velyaw - 90.f;
		break;

	case 2:
		record->m_eye_angles.y = velyaw + 90.f;
		break;
	}
}

void Resolver::AirNS( AimPlayer* data, LagRecord* record ) {
	// get away angles and remember them for server-feedback learning.
	float away = GetAwayAngle( record );
	record->m_away = away;

	switch( data->m_shots % 9 ) {
	case 0:
		record->m_eye_angles.y = away + 180.f;
		break;

	case 1:
		record->m_eye_angles.y = away + 150.f;
		break;
	case 2:
		record->m_eye_angles.y = away - 150.f;
		break;

	case 3:
		record->m_eye_angles.y = away + 165.f;
		break;
	case 4:
		record->m_eye_angles.y = away - 165.f;
		break;

	case 5:
		record->m_eye_angles.y = away + 135.f;
		break;
	case 6:
		record->m_eye_angles.y = away - 135.f;
		break;

	case 7:
		record->m_eye_angles.y = away + 90.f;
		break;
	case 8:
		record->m_eye_angles.y = away - 90.f;
		break;

	default:
		break;
	}
}

void Resolver::ResolvePoses( Player* player, LagRecord* record ) {
	AimPlayer* data = &g_aimbot.m_players[ player->index( ) - 1 ];

	// only do this bs when in air.
	if( record->m_mode == Modes::RESOLVE_AIR ) {
		// ang = pose min + pose val x ( pose range )

		// lean_yaw
		player->m_flPoseParameter( )[ 2 ]  = g_csgo.RandomInt( 0, 4 ) * 0.25f;   

		// body_yaw
		player->m_flPoseParameter( )[ 11 ] = g_csgo.RandomInt( 1, 3 ) * 0.25f;
	}
}
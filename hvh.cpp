#include "includes.h"

HVH g_hvh{ };;

void HVH::IdealPitch( ) {
	CCSGOPlayerAnimState *state = g_cl.m_local->m_PlayerAnimState( );
	if( !state )
		return;

	g_cl.m_cmd->m_view_angles.x = state->m_min_pitch;
}

void HVH::AntiAimPitch( ) {
	bool safe = g_menu.main.config.mode.get( ) == 0;
	
	switch( m_pitch ) {
	case 1:
		// down.
		g_cl.m_cmd->m_view_angles.x = safe ? 89.f : 720.f;
		break;

	case 2:
		// up.
		g_cl.m_cmd->m_view_angles.x = safe ? -89.f : -720.f;
		break;

	case 3:
		// random.
		g_cl.m_cmd->m_view_angles.x = g_csgo.RandomFloat( safe ? -89.f : -720.f, safe ? 89.f : 720.f );
		break;

	case 4:
		// ideal.
		IdealPitch( );
		break;

	case 5:
		// zero.
		g_cl.m_cmd->m_view_angles.x = 0.f;
		break;

	case 6: {
		// jitter ( alternate up / down each tick ).
		bool up = ( ( int )std::floor( g_csgo.m_globals->m_curtime / g_csgo.m_globals->m_interval ) & 1 ) != 0;
		float down_pitch = safe ? 89.f : 720.f;
		float up_pitch = safe ? -89.f : -720.f;
		g_cl.m_cmd->m_view_angles.x = up ? up_pitch : down_pitch;
		break;
	}

	default:
		break;
	}
}

void HVH::ResetAutoDirectionData( ) {
	m_damage_records.clear( );
	m_hit_history.clear( );
	m_has_smoothed_yaw = false;
	m_smoothed_yaw = 0.f;
	m_auto_dist = 0.f;
}

void HVH::RecordDamage( float damage, float yaw ) {
	// ignore invalid data.
	if( damage <= 0.f )
		return;

	float now = g_csgo.m_globals->m_curtime;

	// normalize yaw and store.
	float norm_yaw = math::NormalizedAngle( yaw );

	m_damage_records.push_back( { now, damage, norm_yaw } );
	m_hit_history.push_back( { now, norm_yaw } );

	// trim old data.
	constexpr float HISTORY_WINDOW{ 8.f };
	while( !m_damage_records.empty( ) && ( now - m_damage_records.front( ).m_time ) > HISTORY_WINDOW )
		m_damage_records.pop_front( );

	while( !m_hit_history.empty( ) && ( now - m_hit_history.front( ).m_time ) > HISTORY_WINDOW )
		m_hit_history.pop_front( );
}

void HVH::AutoDirection( ) {
	constexpr float HISTORY_WINDOW{ 8.f };
	constexpr float TREND_WINDOW{ 4.f };
	constexpr float MIN_CONFIDENCE{ 0.01f };

	const float now = g_csgo.m_globals->m_curtime;

	// keep records inside window.
	while( !m_damage_records.empty( ) && ( now - m_damage_records.front( ).m_time ) > HISTORY_WINDOW )
		m_damage_records.pop_front( );

	while( !m_hit_history.empty( ) && ( now - m_hit_history.front( ).m_time ) > HISTORY_WINDOW )
		m_hit_history.pop_front( );

	// nothing to base on, fallback.
	if( m_damage_records.empty( ) && m_hit_history.empty( ) ) {
		// honor timeout to avoid jitter.
		if( m_auto_last > 0.f && m_auto_time > 0.f && now < ( m_auto_last + m_auto_time ) )
			return;

		m_auto = math::NormalizedAngle( m_view - 180.f );
		m_auto_dist = 0.f;
		m_has_smoothed_yaw = false;
		return;
	}

	float sin_sum{ }, cos_sum{ }, weight_sum{ };

	// weighted circular mean using damage and recency.
	for( const auto &rec : m_damage_records ) {
		float age = now - rec.m_time;
		if( age > HISTORY_WINDOW )
			continue;

		float recency = std::max( 0.f, 1.f - ( age / HISTORY_WINDOW ) );
		float dmg_weight = std::max( 1.f, rec.m_damage );
		float weight = recency * dmg_weight;

		const float rad = math::deg_to_rad( rec.m_yaw );
		sin_sum += std::sin( rad ) * weight;
		cos_sum += std::cos( rad ) * weight;
		weight_sum += weight;
	}

	if( weight_sum <= MIN_CONFIDENCE ) {
		m_auto = math::NormalizedAngle( m_view - 180.f );
		m_auto_dist = 0.f;
		m_has_smoothed_yaw = false;
		return;
	}

	float weighted_yaw = math::rad_to_deg( std::atan2( sin_sum / weight_sum, cos_sum / weight_sum ) );
	math::NormalizeAngle( weighted_yaw );

	// evaluate directional trend based on history.
	float trend_adjust{ };
	float trend_weight{ };
	if( m_hit_history.size( ) >= 2 ) {
		for( size_t i{ 1u }; i < m_hit_history.size( ); ++i ) {
			const auto &prev = m_hit_history[ i - 1 ];
			const auto &cur = m_hit_history[ i ];

			float delta = math::NormalizedAngle( cur.m_yaw - prev.m_yaw );
			float age = now - cur.m_time;
			float weight = std::max( 0.f, 1.f - ( age / TREND_WINDOW ) );

			trend_adjust += delta * weight;
			trend_weight += weight;
		}
	}

	if( trend_weight > 0.001f ) {
		trend_adjust /= trend_weight;
		// clamp trend influence.
		math::NormalizeAngle( trend_adjust );
		trend_adjust = std::clamp( trend_adjust, -45.f, 45.f );
	}
	else trend_adjust = 0.f;

	auto blend_angle = [ ]( float from, float to, float t ) {
		float diff = math::NormalizedAngle( to - from );
		return math::NormalizedAngle( from + diff * t );
	};

	float candidate = math::NormalizedAngle( weighted_yaw + trend_adjust );

	// smooth noisy data but keep responsiveness proportional to confidence.
	float smooth_factor = std::clamp( weight_sum / 120.f, 0.15f, 0.65f );
	if( !m_has_smoothed_yaw ) {
		m_smoothed_yaw = candidate;
		m_has_smoothed_yaw = true;
	}
	else m_smoothed_yaw = blend_angle( m_smoothed_yaw, candidate, smooth_factor );

	// final recommended direction (facing threat).
	m_auto = m_smoothed_yaw;
	m_auto_dist = weight_sum;
	m_auto_last = now;
}

void HVH::GetAntiAimDirection( ) {
	// edge aa.
	if( g_menu.main.antiaim.edge.get( ) && g_cl.m_local->m_vecVelocity( ).length( ) < 320.f ) {

		ang_t ang;
		if( DoEdgeAntiAim( g_cl.m_local, ang ) ) {
			m_direction = ang.y;
			return;
		}
	}

	// lock while standing..
	bool lock = g_menu.main.antiaim.dir_lock.get( );

	// save view, depending if locked or not.
	if( ( lock && g_cl.m_speed > 0.1f ) || !lock )
		m_view = g_cl.m_cmd->m_view_angles.y;

	if( m_base_angle > 0 ) {
		// 'static'.
		if( m_base_angle == 1 )
			m_view = 0.f;

		// away options.
		else {
			float  best_fov{ std::numeric_limits< float >::max( ) };
			float  best_dist{ std::numeric_limits< float >::max( ) };
			float  fov, dist;
			Player *target, *best_target{ nullptr };

			for( int i{ 1 }; i <= g_csgo.m_globals->m_max_clients; ++i ) {
				target = g_csgo.m_entlist->GetClientEntity< Player * >( i );

				if( !g_aimbot.IsValidTarget( target ) )
					continue;

				if( target->dormant( ) )
					continue;

				// 'away crosshair'.
				if( m_base_angle == 2 ) {

					// check if a player was closer to our crosshair.
					fov = math::GetFOV( g_cl.m_view_angles, g_cl.m_shoot_pos, target->WorldSpaceCenter( ) );
					if( fov < best_fov ) {
						best_fov = fov;
						best_target = target;
					}
				}

				// 'away distance'.
				else if( m_base_angle == 3 ) {

					// check if a player was closer to us.
					dist = ( target->m_vecOrigin( ) - g_cl.m_local->m_vecOrigin( ) ).length_sqr( );
					if( dist < best_dist ) {
						best_dist = dist;
						best_target = target;
					}
				}
			}

			if( best_target ) {
				// todo - dex; calculate only the yaw needed for this (if we're not going to use the x component that is).
				ang_t angle;
				math::VectorAngles( best_target->m_vecOrigin( ) - g_cl.m_local->m_vecOrigin( ), angle );
				m_view = angle.y;
			}
		}
	}

	// switch direction modes.
	switch( m_dir ) {

		// auto.
	case 0:
		AutoDirection( );
		m_direction = m_auto;
		break;

		// backwards.
	case 1:
		m_direction = m_view + 180.f;
		break;

		// left.
	case 2:
		m_direction = m_view + 90.f;
		break;

		// right.
	case 3:
		m_direction = m_view - 90.f;
		break;

		// custom.
	case 4:
		m_direction = m_view + m_dir_custom;
		break;

		// safe head ( threat based, hide head on least-shot side ).
	case 5: {
		// resolve the threat direction first.
		AutoDirection( );
		float threat = m_auto;

		// accumulate recent damage split by which side of the threat axis
		// it arrived from, so we can steer the head away from the hot side.
		float left_dmg{ }, right_dmg{ };

		for( const auto &rec : m_damage_records ) {
			float side = math::NormalizedAngle( rec.m_yaw - threat );
			if( side >= 0.f )
				left_dmg += std::max( 1.f, rec.m_damage );
			else
				right_dmg += std::max( 1.f, rec.m_damage );
		}

		// face toward the threat, then bias the body so the head sits on the
		// side that has historically taken the least damage.
		float bias = ( left_dmg > right_dmg ) ? -35.f : 35.f;

		m_direction = threat + bias;
		break;
	}

	default:
		break;
	}

	// normalize the direction.
	math::NormalizeAngle( m_direction );
}

bool HVH::DoEdgeAntiAim( Player *player, ang_t &out ) {
	CGameTrace trace;
	static CTraceFilterSimple_game filter{ };

	if( player->m_MoveType( ) == MOVETYPE_LADDER )
		return false;

	// skip this player in our traces.
	filter.SetPassEntity( player );

	// get player bounds.
	vec3_t mins = player->m_vecMins( );
	vec3_t maxs = player->m_vecMaxs( );

	// make player bounds bigger.
	mins.x -= 20.f;
	mins.y -= 20.f;
	maxs.x += 20.f;
	maxs.y += 20.f;

	// get player origin.
	vec3_t start = player->GetAbsOrigin( );

	// offset the view.
	start.z += 56.f;

	g_csgo.m_engine_trace->TraceRay( Ray( start, start, mins, maxs ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );
	if( !trace.m_startsolid )
		return false;

	float  smallest = 1.f;
	vec3_t plane;

	// trace around us in a circle, in 20 steps (anti-degree conversion).
	// find the closest object.
	for( float step{ }; step <= math::pi_2; step += ( math::pi / 10.f ) ) {
		// extend endpoint x units.
		vec3_t end = start;

		// set end point based on range and step.
		end.x += std::cos( step ) * 32.f;
		end.y += std::sin( step ) * 32.f;

		g_csgo.m_engine_trace->TraceRay( Ray( start, end, { -1.f, -1.f, -8.f }, { 1.f, 1.f, 8.f } ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );

		// we found an object closer, then the previouly found object.
		if( trace.m_fraction < smallest ) {
			// save the normal of the object.
			plane = trace.m_plane.m_normal;
			smallest = trace.m_fraction;
		}
	}

	// no valid object was found.
	if( smallest == 1.f || plane.z >= 0.1f )
		return false;

	// invert the normal of this object
	// this will give us the direction/angle to this object.
	vec3_t inv = -plane;
	vec3_t dir = inv;
	dir.normalize( );

	// extend point into object by 24 units.
	vec3_t point = start;
	point.x += ( dir.x * 24.f );
	point.y += ( dir.y * 24.f );

	// check if we can stick our head into the wall.
	if( g_csgo.m_engine_trace->GetPointContents( point, CONTENTS_SOLID ) & CONTENTS_SOLID ) {
		// trace from 72 units till 56 units to see if we are standing behind something.
		g_csgo.m_engine_trace->TraceRay( Ray( point + vec3_t{ 0.f, 0.f, 16.f }, point ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );

		// we didnt start in a solid, so we started in air.
		// and we are not in the ground.
		if( trace.m_fraction < 1.f && !trace.m_startsolid && trace.m_plane.m_normal.z > 0.7f ) {
			// mean we are standing behind a solid object.
			// set our angle to the inversed normal of this object.
			out.y = math::rad_to_deg( std::atan2( inv.y, inv.x ) );
			return true;
		}
	}

	// if we arrived here that mean we could not stick our head into the wall.
	// we can still see if we can stick our head behind/asides the wall.

	// adjust bounds for traces.
	mins = { ( dir.x * -3.f ) - 1.f, ( dir.y * -3.f ) - 1.f, -1.f };
	maxs = { ( dir.x * 3.f ) + 1.f, ( dir.y * 3.f ) + 1.f, 1.f };

	// move this point 48 units to the left 
	// relative to our wall/base point.
	vec3_t left = start;
	left.x = point.x - ( inv.y * 48.f );
	left.y = point.y - ( inv.x * -48.f );

	g_csgo.m_engine_trace->TraceRay( Ray( left, point, mins, maxs ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );
	float l = trace.m_startsolid ? 0.f : trace.m_fraction;

	// move this point 48 units to the right 
	// relative to our wall/base point.
	vec3_t right = start;
	right.x = point.x + ( inv.y * 48.f );
	right.y = point.y + ( inv.x * -48.f );

	g_csgo.m_engine_trace->TraceRay( Ray( right, point, mins, maxs ), CONTENTS_SOLID, ( ITraceFilter * )&filter, &trace );
	float r = trace.m_startsolid ? 0.f : trace.m_fraction;

	// both are solid, no edge.
	if( l == 0.f && r == 0.f )
		return false;

	// set out to inversed normal.
	out.y = math::rad_to_deg( std::atan2( inv.y, inv.x ) );

	// left started solid.
	// set angle to the left.
	if( l == 0.f ) {
		out.y += 90.f;
		return true;
	}

	// right started solid.
	// set angle to the right.
	if( r == 0.f ) {
		out.y -= 90.f;
		return true;
	}

	return false;
}

void HVH::DoRealAntiAim( ) {
	// if we have a yaw antaim.
	if( m_yaw > 0 ) {

		// if we have a yaw active, which is true if we arrived here.
		// set the yaw to the direction before applying any other operations.
		g_cl.m_cmd->m_view_angles.y = m_direction;

		bool stand = g_menu.main.antiaim.body_fake_stand.get( ) > 0 && m_mode == AntiAimMode::STAND;
		bool air = g_menu.main.antiaim.body_fake_air.get( ) > 0 && m_mode == AntiAimMode::AIR;

		// one tick before the update.
		if( stand && !g_cl.m_lag && g_csgo.m_globals->m_curtime >= ( g_cl.m_body_pred - g_cl.m_anim_frame ) && g_csgo.m_globals->m_curtime < g_cl.m_body_pred ) {
			// z mode.
			if( g_menu.main.antiaim.body_fake_stand.get( ) == 4 )
				g_cl.m_cmd->m_view_angles.y -= 90.f;
		}

		// check if we will have a lby fake this tick.
		if( !g_cl.m_lag && g_csgo.m_globals->m_curtime >= g_cl.m_body_pred && ( stand || air ) ) {
			// there will be an lbyt update on this tick.
			if( stand ) {
				switch( g_menu.main.antiaim.body_fake_stand.get( ) ) {

					// left.
				case 1:
					g_cl.m_cmd->m_view_angles.y += 110.f;
					break;

					// right.
				case 2:
					g_cl.m_cmd->m_view_angles.y -= 110.f;
					break;

					// opposite.
				case 3:
					g_cl.m_cmd->m_view_angles.y += 180.f;
					break;

					// z.
				case 4:
					g_cl.m_cmd->m_view_angles.y += 90.f;
					break;
				}
			}

			else if( air ) {
				switch( g_menu.main.antiaim.body_fake_air.get( ) ) {

					// left.
				case 1:
					g_cl.m_cmd->m_view_angles.y += 90.f;
					break;

					// right.
				case 2:
					g_cl.m_cmd->m_view_angles.y -= 90.f;
					break;

					// opposite.
				case 3:
					g_cl.m_cmd->m_view_angles.y += 180.f;
					break;
				}
			}
		}

		// run normal aa code.
		else {
			switch( m_yaw ) {

				// direction.
			case 1:
				// do nothing, yaw already is direction.
				break;

				// jitter.
			case 2: {

				// get the range from the menu.
				float range = m_jitter_range / 2.f;

				// set angle.
				g_cl.m_cmd->m_view_angles.y += g_csgo.RandomFloat( -range, range );
				break;
			}

				  // rotate.
			case 3: {
				// set base angle.
				g_cl.m_cmd->m_view_angles.y = ( m_direction - m_rot_range / 2.f );

				// apply spin.
				g_cl.m_cmd->m_view_angles.y += std::fmod( g_csgo.m_globals->m_curtime * ( m_rot_speed * 20.f ), m_rot_range );

				break;
			}

				  // random.
			case 4:
				// check update time.
				if( g_csgo.m_globals->m_curtime >= m_next_random_update ) {

					// set new random angle.
					m_random_angle = g_csgo.RandomFloat( -180.f, 180.f );

					// set next update time
					m_next_random_update = g_csgo.m_globals->m_curtime + m_rand_update;
				}

				// apply angle.
				g_cl.m_cmd->m_view_angles.y = m_random_angle;
				break;

				  // spin.
			case 5:
				// full 360 spin, speed driven independently of any range.
				g_cl.m_cmd->m_view_angles.y = std::fmod( g_csgo.m_globals->m_curtime * ( m_rot_speed * 36.f ), 360.f ) - 180.f;
				break;

				  // sway ( smooth sine oscillation around direction ).
			case 6:
				g_cl.m_cmd->m_view_angles.y = m_direction + std::sin( g_csgo.m_globals->m_curtime * std::max( m_rot_speed, 0.1f ) ) * ( m_rot_range / 2.f );
				break;

				  // switch ( 2 way, alternate each tick ).
			case 7: {
				float half = m_jitter_range / 2.f;
				bool right = ( ( int )std::floor( g_csgo.m_globals->m_curtime / g_csgo.m_globals->m_interval ) & 1 ) != 0;
				g_cl.m_cmd->m_view_angles.y = m_direction + ( right ? half : -half );
				break;
			}

				  // distortion ( layered non-uniform desync noise around direction ).
			case 8: {
				// combine multiple incommensurate sine waves to produce an
				// irregular, hard to read oscillation rather than a clean sweep.
				float t = g_csgo.m_globals->m_curtime * std::max( m_distortion_speed, 0.1f );
				float amp = m_distortion_amount / 2.f;

				float noise = std::sin( t )
					+ 0.5f * std::sin( t * 2.37f + 1.3f )
					+ 0.25f * std::sin( t * 4.91f + 2.1f );

				// normalize to [ -1, 1 ] ( max magnitude of the three terms is 1.75 ).
				noise /= 1.75f;

				g_cl.m_cmd->m_view_angles.y = m_direction + noise * amp;
				break;
			}

				  // snap ( quantize to fixed angular steps, advance over time ).
			case 9: {
				// step size in degrees ( clamp to a sane minimum ).
				float step = std::max( ( float )m_snap_step, 1.f );

				// how many steps to advance through over time.
				int   slots = std::max( 1, ( int )std::round( 360.f / step ) );
				int   idx = ( int )std::floor( g_csgo.m_globals->m_curtime * std::max( m_snap_speed, 0.01f ) );

				// quantized offset around the direction.
				float offset = ( float )( idx % slots ) * step;

				g_cl.m_cmd->m_view_angles.y = m_direction + offset;
				break;
			}

				  // 3-way ( left / center / right, cycle each tick ).
			case 10: {
				float half = m_jitter_range / 2.f;
				int   phase = ( int )std::floor( g_csgo.m_globals->m_curtime / g_csgo.m_globals->m_interval ) % 3;
				float off = ( phase == 0 ) ? -half : ( phase == 1 ) ? 0.f : half;
				g_cl.m_cmd->m_view_angles.y = m_direction + off;
				break;
			}

				  // pingpong ( linear triangle sweep, sharper than sway ).
			case 11: {
				float amp = m_rot_range / 2.f;
				float phase = std::fmod( g_csgo.m_globals->m_curtime * std::max( m_rot_speed, 0.1f ), 2.f );
				float tri = ( phase < 1.f ) ? ( phase * 2.f - 1.f ) : ( 3.f - phase * 2.f );
				g_cl.m_cmd->m_view_angles.y = m_direction + tri * amp;
				break;
			}

				  // micro jitter ( tiny rapid noise, stays close to direction ).
			case 12: {
				float micro = std::max( 1.f, m_jitter_range * 0.15f );
				g_cl.m_cmd->m_view_angles.y = m_direction + g_csgo.RandomFloat( -micro, micro );
				break;
			}

				  // random switch ( pick a side each update window, then hold ).
			case 13: {
				if( g_csgo.m_globals->m_curtime >= m_next_random_update ) {
					float half = m_jitter_range / 2.f;
					m_random_angle = ( g_csgo.RandomFloat( -1.f, 1.f ) >= 0.f ) ? half : -half;
					m_next_random_update = g_csgo.m_globals->m_curtime + m_rand_update;
				}
				g_cl.m_cmd->m_view_angles.y = m_direction + m_random_angle;
				break;
			}

			default:
				break;
			}

			// apply a static yaw offset on top of the selected mode.
			if( m_yaw_offset != 0.f )
				g_cl.m_cmd->m_view_angles.y += m_yaw_offset;
		}
	}

	// normalize angle.
	math::NormalizeAngle( g_cl.m_cmd->m_view_angles.y );
}

void HVH::DoFakeAntiAim( ) {
	// do fake yaw operations.

	// enforce this otherwise low fps dies.
	// cuz the engine chokes or w/e
	// the fake became the real, think this fixed it.
	*g_cl.m_packet = true;

	// shift factor scales how aggressively the fake desync deviates from the
	// clean opposite of the real. 1.0 == default behaviour.
	float shift = m_shift_factor;
	if( shift <= 0.f )
		shift = 1.f;

	switch( g_menu.main.antiaim.fake_yaw.get( ) ) {

		// default.
	case 1:
		// set base to opposite of direction.
		g_cl.m_cmd->m_view_angles.y = m_direction + 180.f;

		// apply 45 degree jitter, scaled by shift factor.
		g_cl.m_cmd->m_view_angles.y += g_csgo.RandomFloat( -90.f, 90.f ) * shift;
		break;

		// relative.
	case 2:
		// set base to opposite of direction.
		g_cl.m_cmd->m_view_angles.y = m_direction + 180.f;

		// apply offset correction, scaled by shift factor.
		g_cl.m_cmd->m_view_angles.y += g_menu.main.antiaim.fake_relative.get( ) * shift;
		break;

		// relative jitter.
	case 3: {
		// get fake jitter range from menu, scaled by shift factor.
		float range = ( g_menu.main.antiaim.fake_jitter_range.get( ) / 2.f ) * shift;

		// set base to opposite of direction.
		g_cl.m_cmd->m_view_angles.y = m_direction + 180.f;

		// apply jitter.
		g_cl.m_cmd->m_view_angles.y += g_csgo.RandomFloat( -range, range );
		break;
	}

		  // rotate.
	case 4:
		g_cl.m_cmd->m_view_angles.y = m_direction + 90.f + std::fmod( g_csgo.m_globals->m_curtime * 360.f, 180.f );
		break;

		// random.
	case 5:
		g_cl.m_cmd->m_view_angles.y = g_csgo.RandomFloat( -180.f, 180.f );
		break;

		// local view.
	case 6:
		g_cl.m_cmd->m_view_angles.y = g_cl.m_view_angles.y;
		break;

		// opposite ( clean, no jitter ).
	case 7:
		g_cl.m_cmd->m_view_angles.y = m_direction + 180.f;
		break;

		// sway ( smooth oscillation behind the real ).
	case 8:
		g_cl.m_cmd->m_view_angles.y = m_direction + 180.f + std::sin( g_csgo.m_globals->m_curtime * 6.f ) * 60.f;
		break;

	default:
		break;
	}

	// normalize fake angle.
	math::NormalizeAngle( g_cl.m_cmd->m_view_angles.y );
}

void HVH::AntiAim( ) {
	bool attack, attack2;

	if( !g_menu.main.antiaim.enable.get( ) )
		return;

	attack = g_cl.m_cmd->m_buttons & IN_ATTACK;
	attack2 = g_cl.m_cmd->m_buttons & IN_ATTACK2;

	if( g_cl.m_weapon && g_cl.m_weapon_fire ) {
		bool knife = g_cl.m_weapon_type == WEAPONTYPE_KNIFE && g_cl.m_weapon_id != ZEUS;
		bool revolver = g_cl.m_weapon_id == REVOLVER;

		// if we are in attack and can fire, do not anti-aim.
		if( attack || ( attack2 && ( knife || revolver ) ) )
			return;
	}

	// disable conditions.
	if( g_csgo.m_gamerules->m_bFreezePeriod( ) || ( g_cl.m_flags & FL_FROZEN ) || g_cl.m_round_end || ( g_cl.m_cmd->m_buttons & IN_USE ) )
		return;

	// grenade throwing
	// CBaseCSGrenade::ItemPostFrame()
	// https://github.com/VSES/SourceEngine2007/blob/master/src_main/game/shared/cstrike/weapon_basecsgrenade.cpp#L209
	if( g_cl.m_weapon_type == WEAPONTYPE_GRENADE
		&& ( !g_cl.m_weapon->m_bPinPulled( ) || attack || attack2 )
		&& g_cl.m_weapon->m_fThrowTime( ) > 0.f && g_cl.m_weapon->m_fThrowTime( ) < g_csgo.m_globals->m_curtime )
		return;

	m_mode = AntiAimMode::STAND;

	if( ( g_cl.m_buttons & IN_JUMP ) || !( g_cl.m_flags & FL_ONGROUND ) )
		m_mode = AntiAimMode::AIR;

	else if( g_cl.m_speed > 0.1f )
		m_mode = AntiAimMode::WALK;

	// load settings.
	if( m_mode == AntiAimMode::STAND ) {
		m_pitch = g_menu.main.antiaim.pitch_stand.get( );
		m_yaw = g_menu.main.antiaim.yaw_stand.get( );
		m_jitter_range = g_menu.main.antiaim.jitter_range_stand.get( );
		m_rot_range = g_menu.main.antiaim.rot_range_stand.get( );
		m_rot_speed = g_menu.main.antiaim.rot_speed_stand.get( );
		m_rand_update = g_menu.main.antiaim.rand_update_stand.get( );
		m_dir = g_menu.main.antiaim.dir_stand.get( );
		m_dir_custom = g_menu.main.antiaim.dir_custom_stand.get( );
		m_yaw_offset = g_menu.main.antiaim.yaw_offset_stand.get( );
		m_base_angle = g_menu.main.antiaim.base_angle_stand.get( );
		m_auto_time = g_menu.main.antiaim.dir_time_stand.get( );
		m_distortion_amount = g_menu.main.antiaim.distortion_amount_stand.get( );
		m_distortion_speed = g_menu.main.antiaim.distortion_speed_stand.get( );
		m_snap_step = g_menu.main.antiaim.snap_step_stand.get( );
		m_snap_speed = g_menu.main.antiaim.snap_speed_stand.get( );
	}

	else if( m_mode == AntiAimMode::WALK ) {
		m_pitch = g_menu.main.antiaim.pitch_walk.get( );
		m_yaw = g_menu.main.antiaim.yaw_walk.get( );
		m_jitter_range = g_menu.main.antiaim.jitter_range_walk.get( );
		m_rot_range = g_menu.main.antiaim.rot_range_walk.get( );
		m_rot_speed = g_menu.main.antiaim.rot_speed_walk.get( );
		m_rand_update = g_menu.main.antiaim.rand_update_walk.get( );
		m_dir = g_menu.main.antiaim.dir_walk.get( );
		m_dir_custom = g_menu.main.antiaim.dir_custom_walk.get( );
		m_yaw_offset = g_menu.main.antiaim.yaw_offset_walk.get( );
		m_base_angle = g_menu.main.antiaim.base_angle_walk.get( );
		m_auto_time = g_menu.main.antiaim.dir_time_walk.get( );
		m_distortion_amount = g_menu.main.antiaim.distortion_amount_walk.get( );
		m_distortion_speed = g_menu.main.antiaim.distortion_speed_walk.get( );
		m_snap_step = g_menu.main.antiaim.snap_step_walk.get( );
		m_snap_speed = g_menu.main.antiaim.snap_speed_walk.get( );
	}

	else if( m_mode == AntiAimMode::AIR ) {
		m_pitch = g_menu.main.antiaim.pitch_air.get( );
		m_yaw = g_menu.main.antiaim.yaw_air.get( );
		m_jitter_range = g_menu.main.antiaim.jitter_range_air.get( );
		m_rot_range = g_menu.main.antiaim.rot_range_air.get( );
		m_rot_speed = g_menu.main.antiaim.rot_speed_air.get( );
		m_rand_update = g_menu.main.antiaim.rand_update_air.get( );
		m_dir = g_menu.main.antiaim.dir_air.get( );
		m_dir_custom = g_menu.main.antiaim.dir_custom_air.get( );
		m_yaw_offset = g_menu.main.antiaim.yaw_offset_air.get( );
		m_base_angle = g_menu.main.antiaim.base_angle_air.get( );
		m_auto_time = g_menu.main.antiaim.dir_time_air.get( );
		m_distortion_amount = g_menu.main.antiaim.distortion_amount_air.get( );
		m_distortion_speed = g_menu.main.antiaim.distortion_speed_air.get( );
		m_snap_step = g_menu.main.antiaim.snap_step_air.get( );
		m_snap_speed = g_menu.main.antiaim.snap_speed_air.get( );
	}

	// shift factor is global across stances.
	m_shift_factor = g_menu.main.antiaim.fake_shift_factor.get( ) / 100.f;

	// set pitch.
	AntiAimPitch( );

	// if we have any yaw.
	if( m_yaw > 0 ) {
		// set direction.
		GetAntiAimDirection( );
	}

	// we have no real, but we do have a fake.
	else if( g_menu.main.antiaim.fake_yaw.get( ) > 0 )
		m_direction = g_cl.m_cmd->m_view_angles.y;

	if( g_menu.main.antiaim.fake_yaw.get( ) ) {
		// do not allow 2 consecutive sendpacket true if faking angles.
		if( *g_cl.m_packet && g_cl.m_old_packet )
			*g_cl.m_packet = false;

		// run the real on sendpacket false.
		if( !*g_cl.m_packet || !*g_cl.m_final_packet )
			DoRealAntiAim( );

		// run the fake on sendpacket true.
		else DoFakeAntiAim( );
	}

	// no fake, just run real.
	else DoRealAntiAim( );
}

void HVH::SendPacket( ) {
	// if not the last packet this shit wont get sent anyway.
	// fix rest of hack by forcing to false.
	if( !*g_cl.m_final_packet )
		*g_cl.m_packet = false;

	// fake-lag enabled.
	if( g_menu.main.antiaim.lag_enable.get( ) && !g_csgo.m_gamerules->m_bFreezePeriod( ) && !( g_cl.m_flags & FL_FROZEN ) ) {
		// limit of lag.
		int limit = std::min( ( int )g_menu.main.antiaim.lag_limit.get( ), g_cl.m_max_lag );

		// indicates wether to lag or not.
		bool active{ };

		// get current origin.
		vec3_t cur = g_cl.m_local->m_vecOrigin( );

		// get prevoius origin.
		vec3_t prev = g_cl.m_net_pos.empty( ) ? g_cl.m_local->m_vecOrigin( ) : g_cl.m_net_pos.front( ).m_pos;

		// delta between the current origin and the last sent origin.
		float delta = ( cur - prev ).length_sqr( );

		auto activation = g_menu.main.antiaim.lag_active.GetActiveIndices( );
		for( auto it = activation.begin( ); it != activation.end( ); it++ ) {

			// move.
			if( *it == 0 && delta > 0.1f && g_cl.m_speed > 0.1f ) {
				active = true;
				break;
			}

			// air.
			else if( *it == 1 && ( ( g_cl.m_buttons & IN_JUMP ) || !( g_cl.m_flags & FL_ONGROUND ) ) ) {
				active = true;
				break;
			}

			// crouch.
			else if( *it == 2 && g_cl.m_local->m_bDucking( ) ) {
				active = true;
				break;
			}

			// stand ( while standing still ).
			else if( *it == 3 && g_cl.m_speed <= 0.1f && ( g_cl.m_flags & FL_ONGROUND ) ) {
				active = true;
				break;
			}
		}

		if( active ) {
			int mode = g_menu.main.antiaim.lag_mode.get( );

			// max.
			if( mode == 0 )
				*g_cl.m_packet = false;

			// break.
			else if( mode == 1 && delta <= 4096.f )
				*g_cl.m_packet = false;

			// random.
			else if( mode == 2 ) {
				// compute new factor.
				if( g_cl.m_lag >= m_random_lag )
					m_random_lag = g_csgo.RandomInt( 2, limit );

				// factor not met, keep choking.
				else *g_cl.m_packet = false;
			}

			// break step.
			else if( mode == 3 ) {
				// normal break.
				if( m_step_switch ) {
					if( delta <= 4096.f )
						*g_cl.m_packet = false;
				}

				// max.
				else *g_cl.m_packet = false;
			}

			// adaptive ( choke harder the faster we move ).
			else if( mode == 4 ) {
				if( g_cl.m_speed > 130.f || delta <= 4096.f )
					*g_cl.m_packet = false;
			}

			if( g_cl.m_lag >= limit )
				*g_cl.m_packet = true;
		}
	}

	if( !g_menu.main.antiaim.lag_land.get( ) ) {
		vec3_t                start = g_cl.m_local->m_vecOrigin( ), end = start, vel = g_cl.m_local->m_vecVelocity( );
		CTraceFilterWorldOnly filter;
		CGameTrace            trace;

		// gravity.
		vel.z -= ( g_csgo.sv_gravity->GetFloat( ) * g_csgo.m_globals->m_interval );

		// extrapolate.
		end += ( vel * g_csgo.m_globals->m_interval );

		// move down.
		end.z -= 2.f;

		g_csgo.m_engine_trace->TraceRay( Ray( start, end ), MASK_SOLID, &filter, &trace );

		// check if landed.
		if( trace.m_fraction != 1.f && trace.m_plane.m_normal.z > 0.7f && !( g_cl.m_flags & FL_ONGROUND ) )
			*g_cl.m_packet = true;
	}

	// force fake-lag to 14 when fakelagging.
	if( g_input.GetKeyState( g_menu.main.movement.fakewalk.get( ) ) ) {
		*g_cl.m_packet = false;
	}

	// do not lag while shooting.
	if( g_cl.m_old_shot )
		*g_cl.m_packet = true;

	// we somehow reached the maximum amount of lag.
	// we cannot lag anymore and we also cannot shoot anymore since we cant silent aim.
	if( g_cl.m_lag >= g_cl.m_max_lag ) {
		// set bSendPacket to true.
		*g_cl.m_packet = true;

		// disable firing, since we cannot choke the last packet.
		g_cl.m_weapon_fire = false;
	}
}

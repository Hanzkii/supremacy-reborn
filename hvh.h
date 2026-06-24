#pragma once

class AdaptiveAngle {
public:
	float m_yaw;
	float m_dist;

public:
	// ctor.
	__forceinline AdaptiveAngle( float yaw, float penalty = 0.f ) {
		// set yaw.
		m_yaw = math::NormalizedAngle( yaw );

		// init distance.
		m_dist = 0.f;

		// remove penalty.
		m_dist -= penalty;
	}
};

struct DamageRecord_t {
	float m_time;
	float m_damage;
	float m_yaw;
};

struct AngleRecord_t {
	float m_time;
	float m_yaw;
};

enum AntiAimMode : size_t {
	STAND = 0,
	WALK,
	AIR,
};

class HVH {
public:
	size_t m_mode;
	int    m_pitch;
	int    m_yaw;
	float  m_jitter_range;
	float  m_rot_range;
	float  m_rot_speed;
	float  m_rand_update;
	int    m_dir;
	float  m_dir_custom;
	float  m_yaw_offset;
	size_t m_base_angle;
	float  m_auto_time;

	// distortion / snap / shift extensions.
	float  m_distortion_amount;
	float  m_distortion_speed;
	int    m_snap_step;
	float  m_snap_speed;
	float  m_shift_factor;

	bool   m_step_switch;
	int    m_random_lag;
	float  m_next_random_update;
	float  m_random_angle;
	float  m_direction;
	float  m_auto;
	float  m_auto_dist;
	float  m_auto_last;
	float  m_view;

	std::deque< DamageRecord_t > m_damage_records;
	std::deque< AngleRecord_t >  m_hit_history;
	bool   m_has_smoothed_yaw;
	float  m_smoothed_yaw;

public:
	void IdealPitch( );
	void AntiAimPitch( );
	void AutoDirection( );
	void ResetAutoDirectionData( );
	void RecordDamage( float damage, float yaw );
	void GetAntiAimDirection( );
    bool DoEdgeAntiAim( Player *player, ang_t &out );
	void DoRealAntiAim( );
	void DoFakeAntiAim( );
	void AntiAim( );
	void SendPacket( );
};

extern HVH g_hvh;

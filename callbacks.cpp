#include "includes.h"

// execution callbacks..
void callbacks::SkinUpdate( ) {
	g_skins.m_update = true;
}

void callbacks::ForceFullUpdate( ) {
	//static DWORD tick{};
	//
	//if( tick != g_winapi.GetTickCount( ) ) {
	//	g_csgo.cl_fullupdate->m_callback( );
	//	tick = g_winapi.GetTickCount( );
	//

	g_csgo.m_cl->m_delta_tick = -1;
}

void callbacks::ToggleThirdPerson( ) {
	g_visuals.m_thirdperson = !g_visuals.m_thirdperson;
}

void callbacks::ToggleFakeLatency( ) {
	g_aimbot.m_fake_latency = !g_aimbot.m_fake_latency;
}

void callbacks::ToggleKillfeed( ) {
    KillFeed_t* feed = ( KillFeed_t* )g_csgo.m_hud->FindElement( HASH( "SFHudDeathNoticeAndBotStatus" ) );
    if( feed )
        g_csgo.ClearNotices( feed );
}

void callbacks::SaveHotkeys( ) {
	g_config.SaveHotkeys( );
}

void callbacks::ConfigLoad1( ) {
	g_config.load( &g_menu.main, XOR( "1.sup" ) );
	g_menu.main.config.config.select( 1 - 1 );

	g_notify.add( tfm::format( XOR( "loaded config 1\n" ) ) );
}

void callbacks::ConfigLoad2( ) {
	g_config.load( &g_menu.main, XOR( "2.sup" ) );
	g_menu.main.config.config.select( 2 - 1 );
	g_notify.add( tfm::format( XOR( "loaded config 2\n" ) ) );
}

void callbacks::ConfigLoad3( ) {
	g_config.load( &g_menu.main, XOR( "3.sup" ) );
	g_menu.main.config.config.select( 3 - 1 );
	g_notify.add( tfm::format( XOR( "loaded config 3\n" ) ) );
}

void callbacks::ConfigLoad4( ) {
	g_config.load( &g_menu.main, XOR( "4.sup" ) );
	g_menu.main.config.config.select( 4 - 1 );
	g_notify.add( tfm::format( XOR( "loaded config 4\n" ) ) );
}

void callbacks::ConfigLoad5( ) {
	g_config.load( &g_menu.main, XOR( "5.sup" ) );
	g_menu.main.config.config.select( 5 - 1 );
	g_notify.add( tfm::format( XOR( "loaded config 5\n" ) ) );
}

void callbacks::ConfigLoad6( ) {
	g_config.load( &g_menu.main, XOR( "6.sup" ) );
	g_menu.main.config.config.select( 6 - 1 );
	g_notify.add( tfm::format( XOR( "loaded config 6\n" ) ) );
}

void callbacks::ConfigLoad( ) {
	std::string config = g_menu.main.config.config.GetActiveItem( );
	std::string file   = tfm::format( XOR( "%s.sup" ), config.data( ) );

	g_config.load( &g_menu.main, file );
	g_notify.add( tfm::format( XOR( "loaded config %s\n" ), config.data( ) ) );
}

void callbacks::ConfigSave( ) {
	std::string config = g_menu.main.config.config.GetActiveItem( );
	std::string file   = tfm::format( XOR( "%s.sup" ), config.data( ) );

	g_config.save( &g_menu.main, file );
	g_notify.add( tfm::format( XOR( "saved config %s\n" ), config.data( ) ) );
}

bool callbacks::IsBaimHealth( ) {
	return g_menu.main.aimbot.baim2.get( 1 );
}

bool callbacks::IsFovOn( ) {
	return g_menu.main.aimbot.fov.get( );
}

bool callbacks::IsHitchanceOn( ) {
	return g_menu.main.aimbot.hitchance.get( );
}

bool callbacks::IsPenetrationOn( ) {
	return g_menu.main.aimbot.penetrate.get( );
}

bool callbacks::IsMultipointOn( ) {
	return !g_menu.main.aimbot.multipoint.GetActiveIndices( ).empty( );
}

bool callbacks::IsMultipointBodyOn( ) {
	return g_menu.main.aimbot.multipoint.get( 2 );
}

bool callbacks::IsAntiAimModeStand( ) {
	return g_menu.main.antiaim.mode.get( ) == 0;
}

bool callbacks::HasStandYaw( ) {
	return g_menu.main.antiaim.yaw_stand.get( ) > 0;
}

bool callbacks::IsStandYawJitter( ) {
	int y = g_menu.main.antiaim.yaw_stand.get( );
	return y == 2 || y == 7;
}

bool callbacks::IsStandYawRotate( ) {
	int y = g_menu.main.antiaim.yaw_stand.get( );
	return y == 3 || y == 6;
}

bool callbacks::ShowStandRotSpeed( ) {
	int y = g_menu.main.antiaim.yaw_stand.get( );
	return y == 3 || y == 5 || y == 6;
}

bool callbacks::IsStandYawRnadom( ) {
	return g_menu.main.antiaim.yaw_stand.get( ) == 4;
}

bool callbacks::IsStandDirAuto( ) {
	return g_menu.main.antiaim.dir_stand.get( ) == 0;
}

bool callbacks::IsStandDirCustom( ) {
	return g_menu.main.antiaim.dir_stand.get( ) == 4;
}

bool callbacks::IsAntiAimModeWalk( ) {
	return g_menu.main.antiaim.mode.get( ) == 1;
}

bool callbacks::WalkHasYaw( ) {
	return g_menu.main.antiaim.yaw_walk.get( ) > 0;
}

bool callbacks::IsWalkYawJitter( ) {
	int y = g_menu.main.antiaim.yaw_walk.get( );
	return y == 2 || y == 7;
}

bool callbacks::IsWalkYawRotate( ) {
	int y = g_menu.main.antiaim.yaw_walk.get( );
	return y == 3 || y == 6;
}

bool callbacks::ShowWalkRotSpeed( ) {
	int y = g_menu.main.antiaim.yaw_walk.get( );
	return y == 3 || y == 5 || y == 6;
}

bool callbacks::IsWalkYawRandom( ) {
	return g_menu.main.antiaim.yaw_walk.get( ) == 4;
}

bool callbacks::IsWalkDirAuto( ) {
	return g_menu.main.antiaim.dir_walk.get( ) == 0;
}

bool callbacks::IsWalkDirCustom( ) {
	return g_menu.main.antiaim.dir_walk.get( ) == 4;
}

bool callbacks::IsAntiAimModeAir( ) {
	return g_menu.main.antiaim.mode.get( ) == 2;
}

bool callbacks::AirHasYaw( ) {
	return g_menu.main.antiaim.yaw_air.get( ) > 0;
}

bool callbacks::IsAirYawJitter( ) {
	int y = g_menu.main.antiaim.yaw_air.get( );
	return y == 2 || y == 7;
}

bool callbacks::IsAirYawRotate( ) {
	int y = g_menu.main.antiaim.yaw_air.get( );
	return y == 3 || y == 6;
}

bool callbacks::ShowAirRotSpeed( ) {
	int y = g_menu.main.antiaim.yaw_air.get( );
	return y == 3 || y == 5 || y == 6;
}

bool callbacks::IsAirYawRandom( ) {
	return g_menu.main.antiaim.yaw_air.get( ) == 4;
}

bool callbacks::IsAirDirAuto( ) {
	return g_menu.main.antiaim.dir_air.get( ) == 0;
}

bool callbacks::IsAirDirCustom( ) {
	return g_menu.main.antiaim.dir_air.get( ) == 4;
}

static bool FakeYawUsesJitter( int y ) {
	// presets that are driven by the fake jitter range slider.
	return y == 3 || y == 4 || y == 8 || y == 9 || y == 11;
}

bool callbacks::IsFakeStandRelative( ) {
	return g_menu.main.antiaim.mode.get( ) == 0 && g_menu.main.antiaim.fake_yaw_stand.get( ) == 2;
}

bool callbacks::IsFakeStandJitter( ) {
	return g_menu.main.antiaim.mode.get( ) == 0 && FakeYawUsesJitter( g_menu.main.antiaim.fake_yaw_stand.get( ) );
}

bool callbacks::IsFakeWalkRelative( ) {
	return g_menu.main.antiaim.mode.get( ) == 1 && g_menu.main.antiaim.fake_yaw_walk.get( ) == 2;
}

bool callbacks::IsFakeWalkJitter( ) {
	return g_menu.main.antiaim.mode.get( ) == 1 && FakeYawUsesJitter( g_menu.main.antiaim.fake_yaw_walk.get( ) );
}

bool callbacks::IsFakeAirRelative( ) {
	return g_menu.main.antiaim.mode.get( ) == 2 && g_menu.main.antiaim.fake_yaw_air.get( ) == 2;
}

bool callbacks::IsFakeAirJitter( ) {
	return g_menu.main.antiaim.mode.get( ) == 2 && FakeYawUsesJitter( g_menu.main.antiaim.fake_yaw_air.get( ) );
}

bool callbacks::IsConfigMM( ) {
	return g_menu.main.config.mode.get( ) == 0;
}

bool callbacks::IsConfigNS( ) {
	return g_menu.main.config.mode.get( ) == 1;
}

bool callbacks::IsConfig1( ) {
	return g_menu.main.config.config.get( ) == 0;
}

bool callbacks::IsConfig2( ) {
	return g_menu.main.config.config.get( ) == 1;
}

bool callbacks::IsConfig3( ) {
	return g_menu.main.config.config.get( ) == 2;
}

bool callbacks::IsConfig4( ) {
	return g_menu.main.config.config.get( ) == 3;
}

bool callbacks::IsConfig5( ) {
	return g_menu.main.config.config.get( ) == 4;
}

bool callbacks::IsConfig6( ) {
	return g_menu.main.config.config.get( ) == 5;
}

// weaponcfgs callbacks.
// dropdown index ( minus the leading "current weapon" entry ) maps to these weapons.
static constexpr int skin_weapon_order[] = {
	Weapons_t::DEAGLE,
	Weapons_t::ELITE,
	Weapons_t::FIVESEVEN,
	Weapons_t::GLOCK,
	Weapons_t::AK47,
	Weapons_t::AUG,
	Weapons_t::AWP,
	Weapons_t::FAMAS,
	Weapons_t::G3SG1,
	Weapons_t::GALIL,
	Weapons_t::M249,
	Weapons_t::M4A4,
	Weapons_t::MAC10,
	Weapons_t::P90,
	Weapons_t::UMP45,
	Weapons_t::XM1014,
	Weapons_t::BIZON,
	Weapons_t::MAG7,
	Weapons_t::NEGEV,
	Weapons_t::SAWEDOFF,
	Weapons_t::TEC9,
	Weapons_t::P2000,
	Weapons_t::MP7,
	Weapons_t::MP9,
	Weapons_t::NOVA,
	Weapons_t::P250,
	Weapons_t::SCAR20,
	Weapons_t::SG553,
	Weapons_t::SSG08,
	Weapons_t::M4A1S,
	Weapons_t::USPS,
	Weapons_t::CZ75A,
	Weapons_t::REVOLVER,
	Weapons_t::KNIFE_BAYONET,
	Weapons_t::KNIFE_FLIP,
	Weapons_t::KNIFE_GUT,
	Weapons_t::KNIFE_KARAMBIT,
	Weapons_t::KNIFE_M9_BAYONET,
	Weapons_t::KNIFE_HUNTSMAN,
	Weapons_t::KNIFE_FALCHION,
	Weapons_t::KNIFE_BOWIE,
	Weapons_t::KNIFE_BUTTERFLY,
	Weapons_t::KNIFE_SHADOW_DAGGERS
};

// resolve whether the skin config for the given weapon should be shown,
// either because it is manually picked in the weapon dropdown or ( index 0 )
// because it is the weapon currently equipped in-game.
static bool IsSkinWeaponSelected( int weapon ) {
	size_t selection = g_menu.main.skins.weapon.get( );

	if( selection != 0 ) {
		size_t idx = selection - 1;
		return idx < ( sizeof( skin_weapon_order ) / sizeof( skin_weapon_order[ 0 ] ) ) && skin_weapon_order[ idx ] == weapon;
	}

	if( !g_csgo.m_engine->IsInGame( ) || !g_cl.m_processing )
		return false;

	return g_cl.m_weapon_id == weapon;
}

bool callbacks::DEAGLE( ) {
	return IsSkinWeaponSelected( Weapons_t::DEAGLE );
}

bool callbacks::ELITE( ) {
	return IsSkinWeaponSelected( Weapons_t::ELITE );
}

bool callbacks::FIVESEVEN( ) {
	return IsSkinWeaponSelected( Weapons_t::FIVESEVEN );
}

bool callbacks::GLOCK( ) {
	return IsSkinWeaponSelected( Weapons_t::GLOCK );
}

bool callbacks::AK47( ) {
	return IsSkinWeaponSelected( Weapons_t::AK47 );
}

bool callbacks::AUG( ) {
	return IsSkinWeaponSelected( Weapons_t::AUG );
}

bool callbacks::AWP( ) {
	return IsSkinWeaponSelected( Weapons_t::AWP );
}

bool callbacks::FAMAS( ) {
	return IsSkinWeaponSelected( Weapons_t::FAMAS );
}

bool callbacks::G3SG1( ) {
	return IsSkinWeaponSelected( Weapons_t::G3SG1 );
}

bool callbacks::GALIL( ) {
	return IsSkinWeaponSelected( Weapons_t::GALIL );
}

bool callbacks::M249( ) {
	return IsSkinWeaponSelected( Weapons_t::M249 );
}

bool callbacks::M4A4( ) {
	return IsSkinWeaponSelected( Weapons_t::M4A4 );
}

bool callbacks::MAC10( ) {
	return IsSkinWeaponSelected( Weapons_t::MAC10 );
}

bool callbacks::P90( ) {
	return IsSkinWeaponSelected( Weapons_t::P90 );
}

bool callbacks::UMP45( ) {
	return IsSkinWeaponSelected( Weapons_t::UMP45 );
}

bool callbacks::XM1014( ) {
	return IsSkinWeaponSelected( Weapons_t::XM1014 );
}

bool callbacks::BIZON( ) {
	return IsSkinWeaponSelected( Weapons_t::BIZON );
}

bool callbacks::MAG7( ) {
	return IsSkinWeaponSelected( Weapons_t::MAG7 );
}

bool callbacks::NEGEV( ) {
	return IsSkinWeaponSelected( Weapons_t::NEGEV );
}

bool callbacks::SAWEDOFF( ) {
	return IsSkinWeaponSelected( Weapons_t::SAWEDOFF );
}

bool callbacks::TEC9( ) {
	return IsSkinWeaponSelected( Weapons_t::TEC9 );
}

bool callbacks::P2000( ) {
	return IsSkinWeaponSelected( Weapons_t::P2000 );
}

bool callbacks::MP7( ) {
	return IsSkinWeaponSelected( Weapons_t::MP7 );
}

bool callbacks::MP9( ) {
	return IsSkinWeaponSelected( Weapons_t::MP9 );
}

bool callbacks::NOVA( ) {
	return IsSkinWeaponSelected( Weapons_t::NOVA );
}

bool callbacks::P250( ) {
	return IsSkinWeaponSelected( Weapons_t::P250 );
}

bool callbacks::SCAR20( ) {
	return IsSkinWeaponSelected( Weapons_t::SCAR20 );
}

bool callbacks::SG553( ) {
	return IsSkinWeaponSelected( Weapons_t::SG553 );
}

bool callbacks::SSG08( ) {
	return IsSkinWeaponSelected( Weapons_t::SSG08 );
}

bool callbacks::M4A1S( ) {
	return IsSkinWeaponSelected( Weapons_t::M4A1S );
}

bool callbacks::USPS( ) {
	return IsSkinWeaponSelected( Weapons_t::USPS );
}

bool callbacks::CZ75A( ) {
	return IsSkinWeaponSelected( Weapons_t::CZ75A );
}

bool callbacks::REVOLVER( ) {
	return IsSkinWeaponSelected( Weapons_t::REVOLVER );
}

bool callbacks::KNIFE_BAYONET( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_BAYONET );
}

bool callbacks::KNIFE_FLIP( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_FLIP );
}

bool callbacks::KNIFE_GUT( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_GUT );
}

bool callbacks::KNIFE_KARAMBIT( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_KARAMBIT );
}

bool callbacks::KNIFE_M9_BAYONET( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_M9_BAYONET );
}

bool callbacks::KNIFE_HUNTSMAN( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_HUNTSMAN );
}

bool callbacks::KNIFE_FALCHION( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_FALCHION );
}

bool callbacks::KNIFE_BOWIE( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_BOWIE );
}

bool callbacks::KNIFE_BUTTERFLY( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_BUTTERFLY );
}

bool callbacks::KNIFE_SHADOW_DAGGERS( ) {
	return IsSkinWeaponSelected( Weapons_t::KNIFE_SHADOW_DAGGERS );
}

bool callbacks::AUTO_STOP( ) {
	return !g_menu.main.movement.autostop_always_on.get();
}
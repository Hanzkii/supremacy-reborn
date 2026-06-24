#include "../includes.h"

namespace watermark {

    Config cfg;

    // Dedicated fonts for the watermark panel
    static render::Font s_title_font{};
    static render::Font s_stat_font{};
    static bool         s_ready = false;

    // --- Animation state (updated every frame, no heap allocs) ---
    static float s_fade_alpha  = 0.f;   // 0..1, drives the fade-in
    static float s_anim_offset = 0.f;   // 0..1, cycles continuously for gradient + glow
    static float s_glow_pulse  = 0.f;   // 0..1, sine-wave brightness for title glow

    // --- Cached stats (updated on a timer to avoid redundant formatting) ---
    static int         s_fps      = 0;
    static int         s_ping     = 0;
    static int         s_tick     = 0;
    static std::string s_time_str = {};

    // --- FPS tracking (per-frame counter + half-second refresh) ---
    static int                                   s_fps_frames  = 0;
    static std::chrono::steady_clock::time_point s_fps_last_tp = {};

    // --- General animation delta timer ---
    static std::chrono::steady_clock::time_point s_anim_last_tp = {};

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    // Linear interpolation between two Colors. t is clamped to [0,1].
    static Color lerp_color( Color a, Color b, float t ) {
        t = std::max( 0.f, std::min( 1.f, t ) );
        return Color{
            (int)a.r() + (int)( ( (int)b.r() - (int)a.r() ) * t ),
            (int)a.g() + (int)( ( (int)b.g() - (int)a.g() ) * t ),
            (int)a.b() + (int)( ( (int)b.b() - (int)a.b() ) * t ),
            (int)a.a() + (int)( ( (int)b.a() - (int)a.a() ) * t )
        };
    }

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    void initializeWatermark() {
        // Bold title font — "eternal.codes"
        s_title_font = render::Font( XOR( "Verdana" ), 13, FW_BOLD,   FONTFLAG_ANTIALIAS | FONTFLAG_DROPSHADOW );
        // Lighter stat font — fps / ping / tick / time
        s_stat_font  = render::Font( XOR( "Verdana" ), 12, FW_NORMAL, FONTFLAG_ANTIALIAS );

        // Reset state so the next draw starts the fade-in from scratch
        s_fade_alpha   = 0.f;
        s_anim_offset  = 0.f;
        s_glow_pulse   = 0.f;
        s_fps_frames   = 0;

        auto now         = std::chrono::steady_clock::now();
        s_fps_last_tp    = now;
        s_anim_last_tp   = now;

        s_ready = true;
    }

    void updateWatermarkData() {
        if ( !s_ready )
            return;

        auto now = std::chrono::steady_clock::now();

        // --- FPS ---
        ++s_fps_frames;
        float fps_elapsed = std::chrono::duration<float>( now - s_fps_last_tp ).count();
        if ( fps_elapsed >= 0.5f ) {
            s_fps        = (int)std::round( s_fps_frames / fps_elapsed );
            s_fps_frames = 0;
            s_fps_last_tp = now;
        }

        // Game-specific stats are only meaningful when connected to a server.
        if ( g_csgo.m_engine && g_csgo.m_engine->IsInGame() ) {
            // --- Ping (round-trip latency) ---
            s_ping = std::max( 0, (int)std::round( g_cl.m_latency * 1000.f ) );

            // --- Tickrate ---
            if ( g_csgo.m_globals && g_csgo.m_globals->m_interval > 0.f )
                s_tick = (int)std::round( 1.f / g_csgo.m_globals->m_interval );
        }

        // --- Current time string (HH:MM) ---
        {
            time_t t = std::time( nullptr );
            tm     tm_local{};
            localtime_s( &tm_local, &t );
            std::ostringstream oss;
            oss << std::put_time( &tm_local, "%H:%M" );
            s_time_str = oss.str();
        }

        // --- Animation delta time ---
        float dt = std::chrono::duration<float>( now - s_anim_last_tp ).count();
        s_anim_last_tp = now;

        // Fade-in: 0 → 1 in ~0.5 s
        if ( s_fade_alpha < 1.f )
            s_fade_alpha = std::min( 1.f, s_fade_alpha + dt * 2.f );

        // Gradient cycle: slow continuous 0..1 loop
        s_anim_offset += dt * cfg.anim_speed * 0.2f;
        if ( s_anim_offset >= 1.f )
            s_anim_offset -= 1.f;

        // Glow pulse: 0..1 sine oscillation
        s_glow_pulse = ( std::sin( s_anim_offset * math::pi_2 ) + 1.f ) * 0.5f;
    }

    void renderWatermark( int screen_width, int screen_height ) {
        if ( !s_ready || screen_width <= 0 || screen_height <= 0 )
            return;

        // Layout constants
        const int pad_x    = 10;
        const int pad_y    = 5;
        const int accent_h = 2;   // thin top accent line height

        const std::string sep = XOR( " | " );

        // --- Build stat strings (only if enabled) ---
        const std::string title    = XOR( "eternal.codes" );
        const std::string fps_str  = cfg.show_fps  ? tfm::format( XOR( "fps: %i" ),  s_fps )  : "";
        const std::string ping_str = cfg.show_ping ? tfm::format( XOR( "ping: %i" ), s_ping ) : "";
        const std::string tick_str = cfg.show_tick ? tfm::format( XOR( "tick: %i" ), s_tick ) : "";
        const std::string user_str = ( cfg.show_user && !g_cl.m_user.empty() ) ? g_cl.m_user : "";

        // --- Measure widths (no heap allocs after construction) ---
        const auto title_sz = s_title_font.size( title );
        const auto sep_sz   = s_stat_font.size( sep );
        const auto fps_sz   = cfg.show_fps  ? s_stat_font.size( fps_str )  : render::FontSize_t{};
        const auto ping_sz  = cfg.show_ping ? s_stat_font.size( ping_str ) : render::FontSize_t{};
        const auto tick_sz  = cfg.show_tick ? s_stat_font.size( tick_str ) : render::FontSize_t{};
        const auto time_sz  = cfg.show_time ? s_stat_font.size( s_time_str ) : render::FontSize_t{};
        const auto user_sz  = ( cfg.show_user && !user_str.empty() ) ? s_stat_font.size( user_str ) : render::FontSize_t{};

        // --- Total panel width ---
        int content_w = pad_x + title_sz.m_width;
        if ( cfg.show_fps  )                          content_w += sep_sz.m_width + fps_sz.m_width;
        if ( cfg.show_ping )                          content_w += sep_sz.m_width + ping_sz.m_width;
        if ( cfg.show_tick )                          content_w += sep_sz.m_width + tick_sz.m_width;
        if ( cfg.show_time )                          content_w += sep_sz.m_width + time_sz.m_width;
        if ( cfg.show_user && !user_str.empty() )     content_w += sep_sz.m_width + user_sz.m_width;
        content_w += pad_x;

        const int panel_h = accent_h + pad_y + title_sz.m_height + pad_y;
        const int panel_x = screen_width - content_w - cfg.pos_x_offset;
        const int panel_y = cfg.pos_y_offset;

        // --- Alpha levels (all gated by fade and opacity) ---
        const int alpha_bg  = (int)( s_fade_alpha * 175.f * cfg.opacity );
        const int alpha_brd = (int)( s_fade_alpha * 90.f );
        const int alpha_txt = (int)( s_fade_alpha * 255.f * cfg.opacity );
        const int alpha_dim = (int)( s_fade_alpha * 200.f * cfg.opacity );

        // === BACKGROUND PANEL ===
        // Semi-transparent dark rectangle
        render::rect_filled( panel_x, panel_y, content_w, panel_h,
                             { 8, 8, 14, alpha_bg } );
        // Subtle border outline
        render::rect( panel_x, panel_y, content_w, panel_h,
                      { 50, 50, 70, alpha_brd } );

        // === ANIMATED GRADIENT ACCENT LINE (top) ===
        // Two endpoint colors cycle around the configured accent palette.
        {
            float t     = s_anim_offset;
            float inv_t = std::fmod( t + 0.5f, 1.f );

            Color ac1 = lerp_color( cfg.accent_start, cfg.accent_end, t );
            Color ac2 = lerp_color( cfg.accent_end,   cfg.accent_start, inv_t );

            // Apply current fade alpha
            ac1.a() = (uint8_t)( s_fade_alpha * 255.f );
            ac2.a() = (uint8_t)( s_fade_alpha * 255.f );

            render::gradient( panel_x, panel_y, content_w, accent_h, ac1, ac2 );
        }

        // === TEXT CONTENT ===
        const int text_y = panel_y + accent_h + pad_y;
        int       text_x = panel_x + pad_x;

        // Soft glow halo behind title text (pulsates with s_glow_pulse)
        {
            const int glow_a = (int)( s_glow_pulse * 55.f * s_fade_alpha );
            render::rect_filled( text_x - 2, text_y - 1,
                                 title_sz.m_width + 4, title_sz.m_height + 2,
                                 { 130, 60, 220, glow_a } );
        }

        // Title — "eternal.codes" — bold, slightly blue-white
        s_title_font.string( text_x, text_y, { 215, 215, 255, alpha_txt }, title );
        text_x += title_sz.m_width;

        // Separator and stat field colors
        const Color sep_color  = { 80,  80,  110, alpha_dim };
        const Color stat_color = { 165, 165, 185, alpha_dim };

        // Helper: draw a separator then a stat string, advancing text_x
        auto draw_stat = [&]( bool show, const std::string& val,
                               const render::FontSize_t& sz ) {
            if ( !show ) return;
            s_stat_font.string( text_x, text_y, sep_color,  sep );
            text_x += sep_sz.m_width;
            s_stat_font.string( text_x, text_y, stat_color, val );
            text_x += sz.m_width;
        };

        draw_stat( cfg.show_fps,                      fps_str,    fps_sz   );
        draw_stat( cfg.show_ping,                     ping_str,   ping_sz  );
        draw_stat( cfg.show_tick,                     tick_str,   tick_sz  );
        draw_stat( cfg.show_time,                     s_time_str, time_sz  );
        draw_stat( cfg.show_user && !user_str.empty(), user_str,  user_sz  );

        // === DEBUG ROW (optional, below main panel) ===
        if ( cfg.debug_mode ) {
            const float frametime_ms = g_csgo.m_globals
                ? g_csgo.m_globals->m_frametime * 1000.f : 0.f;
            const std::string dbg = tfm::format(
                XOR( "build: beta | ft: %.2fms" ), frametime_ms );
            const auto dbg_sz = s_stat_font.size( dbg );
            const int  dbg_h  = dbg_sz.m_height + pad_y;
            const int  dbg_y  = panel_y + panel_h + 1;
            render::rect_filled( panel_x, dbg_y, content_w, dbg_h,
                                 { 8, 8, 14, alpha_bg } );
            s_stat_font.string( panel_x + pad_x,
                                dbg_y + pad_y / 2,
                                { 140, 140, 160, alpha_dim }, dbg );
        }
    }

} // namespace watermark

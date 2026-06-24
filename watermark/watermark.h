#pragma once

// eternal.codes watermark module
// Provides a modern competitive-style HUD overlay with animated gradient accent,
// fade-in, glow pulse, and configurable dynamic stats.

namespace watermark {

    // All configurable parameters for the watermark.
    struct Config {
        Color accent_start  = { 148, 0,   211, 255 };  // purple
        Color accent_end    = { 0,   200, 255, 255 };  // cyan
        float opacity       = 0.85f;
        float anim_speed    = 1.0f;
        bool  show_fps      = true;
        bool  show_ping     = true;
        bool  show_tick     = true;
        bool  show_time     = true;
        bool  show_user     = false;
        bool  debug_mode    = false;
        int   pos_x_offset  = 10;   // pixels from the right edge
        int   pos_y_offset  = 8;    // pixels from the top edge
    };

    extern Config cfg;

    // Initialize fonts and animation state.
    // Must be called after render::init() (i.e. after ISurface is available).
    void initializeWatermark();

    // Update all dynamic data (fps, ping, tick, time) and advance animations.
    // Call once per rendered frame, before renderWatermark().
    void updateWatermarkData();

    // Draw the watermark overlay into the current VGUI paint frame.
    void renderWatermark( int screen_width, int screen_height );

} // namespace watermark

#pragma once

namespace AdvancedMediaSource
{
    public enum class TransitionEffectType : int
    {
        TRANSITION_NONE = 0,
        TRANSITION_BLACK,
        TRANSITION_WHITE,
        TRANSITION_FADE_TO_BLACK,
        TRANSITION_FADE_TO_WHITE,
        TRANSITION_FADE_OVERLAP,
        TRANSITION_RIGHT_TO_LEFT,
        TRANSITION_TOP_TO_BOTTOM,
        TRANSITION_BOTTOM_TO_TOP,
        TRANSITION_ROOM_IN,
        TRANSITION_ROOM_OUT
    };

    struct TransitionEffect
    {
        TransitionEffectType m_effect;
        ULONGLONG m_length;

        TransitionEffect()
        {
            Reset();
        }

        void Reset()
        {
            m_effect = TransitionEffectType::TRANSITION_NONE;
            m_length = 0;
        }
    };

    struct TransitionParameter
    {
        float m_offset_x;
        float m_offset_y;
        float m_offset_z;
        float m_scaling_x;
        float m_scaling_y;
        float m_fading_ratio;       // means alpha value
        bool m_draw_background;
        float m_backgound_color[4]; // bgra8

        TransitionParameter()
        {
            m_offset_x = 0.f;
            m_offset_y = 0.f;
            m_offset_z = 0.f;
            m_scaling_x = 1.f;
            m_scaling_y = 1.f;
            m_fading_ratio = 1.f;

            m_draw_background = true;
            m_backgound_color[0] = 0.f; // r
            m_backgound_color[1] = 0.f; // g
            m_backgound_color[2] = 0.f; // b
            m_backgound_color[3] = 1.f; // a
        }
    };
}

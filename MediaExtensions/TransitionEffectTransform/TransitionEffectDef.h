#pragma once

#include <vector>

using namespace Windows::UI;

namespace TransitionEffectTransform
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

    public value struct TransitionEffect
    {
        TransitionEffectType m_effect = TransitionEffectType::TRANSITION_NONE;
        ULONGLONG m_length = 0;
    };

    public ref class TransitionEffectParameter sealed
    {
    public:
        TransitionEffectParameter(){};
        virtual ~TransitionEffectParameter(){};

        void SetStartEffect(TransitionEffectType effect, FLOAT length)
        {
            m_start_effect.m_effect = effect;
            m_start_effect.m_length = (ULONGLONG)(length * 10000000);
        }

        TransitionEffect GetStartEffect()
        {
            return m_start_effect;
        }

        void SetEndEffect(TransitionEffectType effect, FLOAT length)
        {
            m_end_effect.m_effect = effect;
            m_end_effect.m_length = (ULONGLONG)(length * 10000000);
        }

        TransitionEffect GetEndEffect()
        {
            return m_end_effect;
        }

        void SetVideoDuration(FLOAT duration)
        {
            m_video_duration = (ULONGLONG)(duration * 10000000);
        }

        ULONGLONG GetVideoDuration()
        {
            return m_video_duration;
        }

    private:
        TransitionEffect m_start_effect;
        TransitionEffect m_end_effect;
        ULONGLONG m_video_duration;
    };    
}
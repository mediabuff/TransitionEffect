#include "pch.h"
#include "TransitionManager.h"

using namespace AdvancedMediaSource;

TransitionManager::TransitionManager()
    : m_timeline_compiled(false)
    , mp_time_line_segment(NULL)
{

}

TransitionManager::~TransitionManager()
{
    
}

void TransitionManager::CleanTimeLine()
{
    m_video_duration_list.clear();
    m_transition_effect_list.clear();
    m_time_line_info_list.clear();

    m_timeline_compiled = false;
}

bool TransitionManager::AddVideoInfoInOrder(ULONGLONG video_duration)
{
    if (m_timeline_compiled)
        return false;

    m_video_duration_list.push_back(video_duration);

    TransitionEffect effect_none;
    effect_none.m_effect = TransitionEffectType::TRANSITION_NONE;
    effect_none.m_length = 0;
    m_transition_effect_list.push_back(effect_none);

    return true;
}

bool TransitionManager::AddTransitionBetweenVideos(TransitionEffect effect, int index_of_previous_video)
{
    if (m_timeline_compiled)
        return false;

    if (index_of_previous_video > (int)m_video_duration_list.size() - 1)
        return false;

    m_transition_effect_list[index_of_previous_video] = effect;
    return true;
}

void TransitionManager::CompileTimeLine()
{
    ULONGLONG curr_time = 0;

    int number_of_videos = (int)m_video_duration_list.size();
    m_time_line_info_list.clear();
    m_video_index_to_time_line_index.clear();
    for (int i = 0; i < number_of_videos; i++)
    {
        TransitionEffect start_effect;
        TransitionEffect end_effect;

        if (i > 0)
        {
            start_effect = m_transition_effect_list[i - 1];
            curr_time = CalculateVideoStartTime(curr_time, m_transition_effect_list[i - 1]);
        }

        if (i < number_of_videos - 1)
        {
            assert(i < (int)m_transition_effect_list.size());
            end_effect = m_transition_effect_list[i];
        }

        if (end_effect.m_effect == TransitionEffectType::TRANSITION_BLACK ||
            end_effect.m_effect == TransitionEffectType::TRANSITION_WHITE)
        {
            TransitionEffect empty_effect;

            AddVideoToTimeLine(i, curr_time, m_video_duration_list[i], empty_effect, empty_effect);
            curr_time += m_video_duration_list[i];

            AddVideoToTimeLine(EMPTY_VIDEO, curr_time, end_effect.m_length, end_effect, empty_effect);
            curr_time += end_effect.m_length;
        }
        else
        {
            AddVideoToTimeLine(i, curr_time, m_video_duration_list[i], start_effect, end_effect);
            curr_time += m_video_duration_list[i];
        }
    }

    m_timeline_compiled = true;
}

void TransitionManager::GetCurrentVideoIndexs(ULONGLONG curr_time, std::vector<int> &video_index_list, bool &is_end_of_stream)
{
    if (!m_timeline_compiled)
        return;

    is_end_of_stream = true;

    video_index_list.clear();
    for (int i = 0; i < (int)m_time_line_info_list.size(); i++)
    {
        is_end_of_stream = false;

        if (curr_time >= m_time_line_info_list[i].m_start_time &&
            curr_time < m_time_line_info_list[i].m_end_time)
        {
            if (m_time_line_info_list[i].m_video_index != EMPTY_VIDEO)
                video_index_list.push_back(m_time_line_info_list[i].m_video_index);
        }
    }

    if (!is_end_of_stream)
    {
        ULONGLONG final_end_time = m_time_line_info_list[m_time_line_info_list.size() - 1].m_end_time;
        is_end_of_stream = (curr_time >= final_end_time);
    }    
}

TransitionParameter TransitionManager::GetVideoTransitionParameter(ULONGLONG curr_time, int video_index)
{
    TransitionParameter parameter;

    if (!m_timeline_compiled)
        return parameter;

    if (EMPTY_VIDEO == video_index)
    {
        int time_line_index = 0;
        for (int i = 0; i < (int)m_time_line_info_list.size(); i++)
        {
            int curr_video_index = m_time_line_info_list[i].m_video_index;
            ULONGLONG curr_start_time = m_time_line_info_list[i].m_start_time;
            ULONGLONG curr_end_time = m_time_line_info_list[i].m_end_time;

            if (EMPTY_VIDEO == curr_video_index &&
                curr_time >= curr_start_time &&
                curr_time < curr_end_time)
            {
                time_line_index = i;
                break;
            }
        }
        TransitionEffect start_effect = m_time_line_info_list[time_line_index].m_start_effect;
        parameter = GenerateTransitionParameter(1.f, start_effect.m_effect, true);
    }
    else
    {
        assert(video_index < (int)m_video_index_to_time_line_index.size());

        int time_line_index = m_video_index_to_time_line_index[video_index];

        ULONGLONG start_time = m_time_line_info_list[time_line_index].m_start_time;
        TransitionEffect start_effect = m_time_line_info_list[time_line_index].m_start_effect;
        ULONGLONG end_time = m_time_line_info_list[time_line_index].m_end_time;
        TransitionEffect end_effect = m_time_line_info_list[time_line_index].m_end_effect;

        if (curr_time < start_time + start_effect.m_length)
        {
            float effect_ratio = 1.f - (float)(curr_time - start_time) / start_effect.m_length;
            parameter = GenerateTransitionParameter(effect_ratio, start_effect.m_effect, true);
        }
        else if (curr_time >= end_time - end_effect.m_length)
        {
            float effect_ratio = 1.f - (float)(end_time - curr_time) / end_effect.m_length;
            parameter = GenerateTransitionParameter(effect_ratio, end_effect.m_effect, false);
        }
    }

    return parameter;
}

void TransitionManager::AddVideoToTimeLine(int video_index, ULONGLONG start_time, ULONGLONG duration,
                                           TransitionEffect start_effect, TransitionEffect end_effect)
{
    TimeLineInfo info;

    info.m_video_index = video_index;
    info.m_start_time = start_time;
    info.m_duration = duration;
    info.m_end_time = start_time + duration;
    info.m_start_effect = start_effect;
    info.m_end_effect = end_effect;

    m_time_line_info_list.push_back(info);

    m_video_index_to_time_line_index.push_back((int)m_time_line_info_list.size() - 1);
}

ULONGLONG TransitionManager::CalculateVideoStartTime(ULONGLONG ori_start_time, TransitionEffect effect)
{
    switch (effect.m_effect)
    {
    case TransitionEffectType::TRANSITION_NONE:
    case TransitionEffectType::TRANSITION_BLACK:
    case TransitionEffectType::TRANSITION_WHITE:
    case TransitionEffectType::TRANSITION_FADE_TO_BLACK:
    case TransitionEffectType::TRANSITION_FADE_TO_WHITE:
        // no change
        break;

    case TransitionEffectType::TRANSITION_FADE_OVERLAP:
    case TransitionEffectType::TRANSITION_RIGHT_TO_LEFT:
    case TransitionEffectType::TRANSITION_TOP_TO_BOTTOM:
    case TransitionEffectType::TRANSITION_BOTTOM_TO_TOP:
    case TransitionEffectType::TRANSITION_ROOM_IN:
    case TransitionEffectType::TRANSITION_ROOM_OUT:
        ori_start_time -= effect.m_length;
        break;

    default:
        break;
    }

    return ori_start_time;
}

TransitionParameter TransitionManager::GenerateTransitionParameter(float transition_effect_ratio, TransitionEffectType effect_type, bool is_start)
{
    TransitionParameter parameter;

    switch (effect_type)
    {
    case TransitionEffectType::TRANSITION_NONE:
        // do nothing

    case TransitionEffectType::TRANSITION_BLACK:
        parameter.m_draw_background = true;
        parameter.m_backgound_color[0] = 0.f;
        parameter.m_backgound_color[1] = 0.f;
        parameter.m_backgound_color[2] = 0.f;
        parameter.m_backgound_color[3] = 1.f;
        break;

    case TransitionEffectType::TRANSITION_WHITE:
        parameter.m_draw_background = true;
        parameter.m_backgound_color[0] = 1.f;
        parameter.m_backgound_color[1] = 1.f;
        parameter.m_backgound_color[2] = 1.f;
        parameter.m_backgound_color[3] = 1.f;
        break;

    case TransitionEffectType::TRANSITION_FADE_TO_BLACK:
        parameter.m_draw_background = true;
        parameter.m_backgound_color[0] = 0.f;
        parameter.m_backgound_color[1] = 0.f;
        parameter.m_backgound_color[2] = 0.f;
        parameter.m_backgound_color[3] = 1.f;
        parameter.m_fading_ratio = 1.f - transition_effect_ratio;
        break;

    case TransitionEffectType::TRANSITION_FADE_TO_WHITE:
        parameter.m_draw_background = true;
        parameter.m_backgound_color[0] = 1.f;
        parameter.m_backgound_color[1] = 1.f;
        parameter.m_backgound_color[2] = 1.f;
        parameter.m_backgound_color[3] = 1.f;
        parameter.m_fading_ratio = 1.f - transition_effect_ratio;
        break;

    case TransitionEffectType::TRANSITION_FADE_OVERLAP:
        // set background alpha to 0
        if (is_start)
        {
            parameter.m_draw_background = false;
            parameter.m_fading_ratio = 1.f - transition_effect_ratio;
        }
        break;

    case TransitionEffectType::TRANSITION_RIGHT_TO_LEFT:
        if (is_start)
        {
            parameter.m_draw_background = false;
            // 1.f -> 0.f
            parameter.m_offset_x = transition_effect_ratio * 2.f;
        }
        break;

    case TransitionEffectType::TRANSITION_TOP_TO_BOTTOM:
        if (is_start)
        {
            parameter.m_draw_background = false;
            // -1.f -> 0.f
            parameter.m_offset_y = transition_effect_ratio * 2.f;
        }
        break;

    case TransitionEffectType::TRANSITION_BOTTOM_TO_TOP:
        if (is_start)
        {
            parameter.m_draw_background = false;
            // 1.f -> 0.f
            parameter.m_offset_y = -transition_effect_ratio * 2.f;
        }
        break;

    case TransitionEffectType::TRANSITION_ROOM_IN:
        if (is_start)
        {
            parameter.m_draw_background = false;
            // 0.f -> 1.f
            float scaling = 1.f + 2.f * transition_effect_ratio;
            parameter.m_scaling_x = scaling;
            parameter.m_scaling_y = scaling;
        }
        break;

    case TransitionEffectType::TRANSITION_ROOM_OUT:
        if (is_start)
        {
            parameter.m_draw_background = false;
            // 0.f -> 1.f
            float scaling = 1.f - transition_effect_ratio;
            parameter.m_scaling_x = scaling;
            parameter.m_scaling_y = scaling;
        }
        break;

    default:
        break;
    }

    return parameter;
}
#pragma once

#include "VideoTypes.h"

using namespace AdvancedMediaSource;

#define EMPTY_VIDEO -1

struct TimeLineInfo
{
    int m_video_index;
    ULONGLONG m_start_time;
    ULONGLONG m_end_time;
    ULONGLONG m_duration;
    TransitionEffect m_start_effect;
    TransitionEffect m_end_effect;

    TimeLineInfo()
    {
        m_video_index = EMPTY_VIDEO;
        m_start_time = 0;
        m_end_time = 0;
        m_duration = 0;
        m_start_effect.Reset();
        m_end_effect.Reset();
    }
};

class TransitionManager
{
public:
    TransitionManager();
    ~TransitionManager();

    void CleanTimeLine();
    bool AddVideoInfoInOrder(ULONGLONG video_duration);
    bool AddTransitionBetweenVideos(TransitionEffect effect, int index_of_previous_video);
    void CompileTimeLine();

    void GetCurrentVideoIndexs(ULONGLONG curr_time, std::vector<int> &video_index_list, bool &is_end_of_stream);
    TransitionParameter GetVideoTransitionParameter(ULONGLONG curr_time, int video_index);

private:
    bool m_timeline_compiled;
    std::vector<ULONGLONG> m_video_duration_list;
    std::vector<TransitionEffect> m_transition_effect_list;
    std::vector<TimeLineInfo> m_time_line_info_list;
    std::vector<int> m_video_index_to_time_line_index;
    ULONGLONG *mp_time_line_segment;

    ULONGLONG CalculateVideoStartTime(ULONGLONG ori_start_time, TransitionEffect effect);
    TransitionParameter GenerateTransitionParameter(float transition_effect_ratio, TransitionEffectType effect_type, bool is_start);


    void AddVideoToTimeLine(int video_index, ULONGLONG start_time, ULONGLONG duration,
                            TransitionEffect start_effect, TransitionEffect end_effect);
};
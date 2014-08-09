#pragma once

namespace AdvancedMediaSource
{
    public value struct SVideoData
    {
        UINT32 Width, Height, Stride;
        UINT32 Numerator, Denominator;
        LONGLONG SampleDuration;
        UINT32 SampleSize;
        MFTIME Duration;
        
        UINT32 ASampleRate, AChannelCount, ABitsPerSample; // Audio props
        bool HasAudio;
    };
}

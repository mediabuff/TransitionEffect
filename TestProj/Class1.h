#pragma once

#include "CritSec.h"
#include "pch.h"

namespace TestProj
{
	public ref class Class1 sealed
	{
	public:

		Class1();
		virtual ~Class1();

		bool SetVideo(int id, Platform::String^ url);
		int GetWidth() { return m_Width; }
		int GetHeight() { return m_Height; }
		int GetFrameRate() { return m_FrameRate; }
		bool IsInitialized() { return m_Initialized; }

		void GenerateSample(Windows::Media::Core::MediaStreamSourceSampleRequest ^ request);

	private:

		CritSec m_critSec;
		ComPtr<IMFSourceReader> m_Sources[2];
		ComPtr<IMFVideoSampleAllocator> m_pSampleAllocator;
		int m_Width, m_Height, m_Stride, m_FrameRate;
		ULONGLONG m_VideoTimestamp;
		bool m_Initialized;
	};
}

#include "pch.h"
#include "Class1.h"

using namespace TestProj;
using namespace Platform;
using namespace Windows::Media::Core;
using namespace Windows::Media::MediaProperties;

Class1::Class1()
{
	m_Width = 0;
	m_Height = 0;
	m_FrameRate = 0;
	m_VideoTimestamp = 0;
	m_Initialized = false;



	/*ComPtr<IMFMediaType> pType;

	if(MFCreateMediaType(&pType) != S_OK)
	{
	return;
	}

	pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);

	HRESULT hr;*/



	m_Initialized = true;
}

Class1::~Class1()
{
	if (m_pSampleAllocator != nullptr)
	{
		m_pSampleAllocator->UninitializeSampleAllocator();
	}

}

bool Class1::SetVideo(int id, Platform::String^ url)
{
	if (id < 0 || id > 1)
	{
		return false;
	}

	AutoLock lock(m_critSec);

	ComPtr<IMFSourceReader> pSourceReader;

	if (MFCreateSourceReaderFromURL(url->Data(), NULL, &pSourceReader) != S_OK)
	{
		return false;
	}

	for (DWORD streamId = 0; true; streamId++)
	{
		ComPtr<IMFMediaType> pNativeType;
		ComPtr<IMFMediaType> pType;
		ComPtr<IMFMediaType> pFinalType;

		HRESULT hr = pSourceReader->GetNativeMediaType(streamId, 0, &pNativeType);
		if (hr == MF_E_NO_MORE_TYPES)
		{
			continue;
		}
		else if (hr == MF_E_INVALIDSTREAMNUMBER)
		{
			break;
		}
		else if (FAILED(hr))
		{
			return false;
		}

		GUID majorType, subtype;

		// Find the major type.
		hr = pNativeType->GetGUID(MF_MT_MAJOR_TYPE, &majorType);
		if (FAILED(hr))
		{
			return false;
		}

		// Define the output type.
		hr = MFCreateMediaType(&pType);
		if (FAILED(hr))
		{
			return false;
		}

		hr = pType->SetGUID(MF_MT_MAJOR_TYPE, majorType);
		if (FAILED(hr))
		{
			return false;
		}

		// Select a subtype.
		if (majorType == MFMediaType_Video)
		{
			subtype = MFVideoFormat_NV12;


		}
		else if (majorType == MFMediaType_Audio)
		{
			subtype = MFAudioFormat_PCM;
		}
		else
		{
			// Unrecognized type. Skip.
			continue;
		}

		hr = pType->SetGUID(MF_MT_SUBTYPE, subtype);
		if (FAILED(hr))
		{
			return false;
		}

		// Set the uncompressed format.
		hr = pSourceReader->SetCurrentMediaType(streamId, NULL, pType.Get());
		if (FAILED(hr))
		{
			return false;
		}

		// test final type
		hr = pSourceReader->GetCurrentMediaType(streamId, &pFinalType);
		if (FAILED(hr))
		{
			return false;
		}

		DWORD eFlags = 0;

		if (pType->IsEqual(pFinalType.Get(), &eFlags) == E_INVALIDARG)
		{
			return false;
		}
		else
		{
			if (!(eFlags & (MF_MEDIATYPE_EQUAL_MAJOR_TYPES | MF_MEDIATYPE_EQUAL_FORMAT_TYPES)))
			{
				return false;
			}
		}

		//Temporary logic for test project ONLY
		if (majorType == MFMediaType_Video)
		{
			//Get max width and height
			UINT32 width = 0, height = 0;
			UINT32 Numerator = 0, Denominator = 0;

			if (MFGetAttributeSize(pFinalType.Get(), MF_MT_FRAME_SIZE, &width, &height) != S_OK)
			{
				return false;
			}

			if (MFGetAttributeRatio(pFinalType.Get(), MF_MT_FRAME_RATE, &Numerator, &Denominator) != S_OK)
			{
				return false;
			}

			m_Width = (int)width > m_Width ? width : m_Width;
			m_Height = (int)height > m_Height ? height : m_Height;
			m_Stride = m_Width; // for NV12
			m_FrameRate = (int)Numerator > m_FrameRate ? Numerator : m_FrameRate;

			if (m_pSampleAllocator == nullptr)
			{
				if (MFCreateVideoSampleAllocatorEx(IID_PPV_ARGS(&m_pSampleAllocator)) != S_OK)
				{
					return false;
				}

				if ((hr = m_pSampleAllocator->InitializeSampleAllocator(60, pFinalType.Get())) != S_OK)
				{
					return false;
				}
			}
		}
	}

	m_Sources[id] = pSourceReader;

	return true;
}

void Class1::GenerateSample(Windows::Media::Core::MediaStreamSourceSampleRequest ^ request)
{
	ComPtr<IMFMediaStreamSourceSampleRequest> pRequest;
	VideoEncodingProperties^ pEncodingProperties;
	HRESULT hr = (request != nullptr) ? S_OK : E_POINTER;
	UINT32 ui32Height = 0;
	UINT32 ui32Width = 0;
	ULONGLONG ulTimeSpan = 0;

	AutoLock lock(m_critSec);

	if (SUCCEEDED(hr))
	{
		hr = reinterpret_cast<IInspectable*>(request)->QueryInterface(IID_PPV_ARGS(&pRequest));
	}

	if (SUCCEEDED(hr))
	{
		VideoStreamDescriptor^ pVideoStreamDescriptor = dynamic_cast<VideoStreamDescriptor^>(request->StreamDescriptor);

		if (pVideoStreamDescriptor != nullptr)
		{
			pEncodingProperties = pVideoStreamDescriptor->EncodingProperties;
		}
		else
		{
			return;//throw Platform::Exception::CreateException(E_INVALIDARG, L"Media Request is not for an video sample.");
		}
	}

	if (SUCCEEDED(hr))
	{
		ui32Height = pEncodingProperties->Height;
		ui32Width = pEncodingProperties->Width;

		MediaRatio^ spRatio = pEncodingProperties->FrameRate;
		if (SUCCEEDED(hr))
		{
			UINT32 ui32Numerator = spRatio->Numerator;
			UINT32 ui32Denominator = spRatio->Denominator;
			if (ui32Numerator != 0)
			{
				ulTimeSpan = ((ULONGLONG)ui32Denominator) * 10000000 / ui32Numerator;
			}
			else
			{
				hr = E_INVALIDARG;
			}
		}
	}

	if (SUCCEEDED(hr))
	{
		ComPtr<IMFMediaBuffer> pBuffer;
		ComPtr<IMFSample> pSample;

		hr = m_pSampleAllocator->AllocateSample(&pSample);

		if (SUCCEEDED(hr))
		{
			hr = pSample->SetSampleDuration(ulTimeSpan);
		}

		if (SUCCEEDED(hr))
		{
			hr = pSample->SetSampleTime((LONGLONG)m_VideoTimestamp);
		}

		if (SUCCEEDED(hr))
		{
			hr = pSample->GetBufferByIndex(0, &pBuffer);
		}

		if (SUCCEEDED(hr))
		{
			//hr = GenerateCubeFrame(ui32Width, ui32Height, _iVideoFrameNumber, spBuffer.Get());

			BYTE* pBits = 0;
			BYTE* pBits0 = 0;
			BYTE* pBits1 = 0;
			DWORD ml = 0;

			if (pBuffer->Lock(&pBits, &ml, NULL) != S_OK)
			{
				return;
			}

			ComPtr<IMFSample> pSample0;
			ComPtr<IMFSample> pSample1;
			ComPtr<IMFMediaBuffer> pBuffer0;
			ComPtr<IMFMediaBuffer> pBuffer1;
			DWORD ActualStreamIndex;
			DWORD StreamFlags = 100;
			LONGLONG SampleTimestamp;

			if (m_Sources[0]->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &pSample0) != S_OK)
			{
				return;
			}

			if (m_Sources[1]->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &ActualStreamIndex, &StreamFlags, &SampleTimestamp, &pSample1) != S_OK)
			{
				return;
			}

			if (pSample0->GetBufferByIndex(0, &pBuffer0) != S_OK)
			{
				return;
			}

			if (pSample1->GetBufferByIndex(0, &pBuffer1) != S_OK)
			{
				return;
			}

			if (pBuffer0->Lock(&pBits0, NULL, NULL) != S_OK)
			{
				return;
			}

			if (pBuffer1->Lock(&pBits1, NULL, NULL) != S_OK)
			{
				return;
			}

			//CopyMemory(pBits, pBits0, 1280 * (720 + 720 / 2));

			static int offset = 0, ddd = 100000;
			offset += ulTimeSpan;

			if (offset >= ui32Width * ddd) { offset = ui32Width * ddd; }

			for (int h = 0; h < ui32Height + ui32Height / 2; h++)
			{
				CopyMemory(pBits + ui32Width * h, pBits0 + ui32Width * h + offset / ddd, ui32Width - offset / ddd);
				CopyMemory(pBits + ui32Width * h + ui32Width - offset / ddd, pBits1 + ui32Width * h, offset / ddd);
			}

			pBuffer->Unlock();
			pBuffer0->Unlock();
			pBuffer1->Unlock();
		}

		if (SUCCEEDED(hr))
		{
			hr = pRequest->SetSample(pSample.Get());
		}

		if (SUCCEEDED(hr))
		{
			m_VideoTimestamp += ulTimeSpan;
		}
	}
}

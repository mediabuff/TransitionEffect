#pragma once

#include "pch.h"

namespace AdvancedMediaSource
{
	struct SShaderFrameData
	{
		float scale[2];
		float offset[2];
		float fading;

		float pad[3];
	};

	static_assert((sizeof(SShaderFrameData) % 16) == 0, "If the bind flag is D3D11_BIND_CONSTANT_BUFFER, you must set the ByteWidth value in multiples of 16, and less than or equal to D3D11_REQ_CONSTANT_BUFFER_ELEMENT_COUNT.");
}

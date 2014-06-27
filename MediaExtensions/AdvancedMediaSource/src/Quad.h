#pragma once

#include "pch.h"

namespace AdvancedMediaSource
{
	struct SVertex
	{ 
		float x, y, z, w;
		float u, v;
	};

	struct SQuad
	{
		SVertex v[4];
	};
}

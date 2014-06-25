#pragma once

#include "pch.h"

namespace AdvancedMediaSource
{
	public enum class EIntroType : int
	{
		None,
		FadeIn
	};

	public enum class EOutroType : int
	{
		None,
		FadeOut,
		TransitionLeft
	};

	public enum class EVideoEffect : int
	{
		None,
		Grayscale,
		Bloom
	};

	struct SIntro
	{
		EIntroType Type;
		LONGLONG Duration;
	};

	struct SOutro
	{
		EOutroType Type;
		LONGLONG Duration;
	};
}

// Copyright (c) 2017 Emilian Cioca
#include "TextureEncoder.h"

int main()
{
	auto encoder = TextureEncoder();

	if (!gem::Encoder::RunEncoder(encoder))
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

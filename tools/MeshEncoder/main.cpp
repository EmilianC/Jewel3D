// Copyright (c) 2017 Emilian Cioca
#include "MeshEncoder.h"

int main()
{
	auto encoder = MeshEncoder();

	if (!gem::Encoder::RunEncoder(encoder))
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

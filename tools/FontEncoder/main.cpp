// Copyright (c) 2017 Emilian Cioca
#include "FontEncoder.h"

int main()
{
	auto encoder = FontEncoder();

	if (!gem::Encoder::RunEncoder(encoder))
	{
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

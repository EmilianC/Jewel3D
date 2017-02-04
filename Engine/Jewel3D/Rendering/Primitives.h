// Copyright (c) 2017 Emilian Cioca
#pragma once
#include "Jewel3D/Resource/Shader.h"
#include "Jewel3D/Resource/Texture.h"
#include "Jewel3D/Utilities/Singleton.h"

namespace Jwl
{
	class vec3;
	class vec4;

	//- Provides simple, intimidate, geometry rendering.
	//- These features are mostly intended for debugging, as they are not particularly efficient.
	static class Primitives : public Singleton<class Primitives>
	{
	public:
		bool Load();
		bool IsLoaded() const;
		void Unload();

		void DrawLine(const vec3& p1, const vec3& p2, const vec4& color);
		void DrawLine(const vec3& p1, const vec3& p2, const vec4& color1, const vec4& color2);
		void DrawLine(const vec3& p1, const vec3& p2, Texture& tex);

		void DrawTriangle(const vec3& p1, const vec3& p2, const vec3& p3, const vec4& color);
		void DrawTriangle(const vec3& p1, const vec3& p2, const vec3& p3, const vec4& color1, const vec4& color2, const vec4& color3);
		void DrawTriangle(const vec3& p1, const vec3& p2, const vec3& p3, Texture& tex);

		void DrawRectangle(const vec3& p1, const vec3& p2, const vec3& p3, const vec3& p4, const vec4& color);
		void DrawRectangle(const vec3& p1, const vec3& p2, const vec3& p3, const vec3& p4, const vec4& color1, const vec4& color2, const vec4& color3, const vec4& color4);
		void DrawRectangle(const vec3& p1, const vec3& p2, const vec3& p3, const vec3& p4, Texture& tex);
		void DrawUnitRectangle();

		void DrawGrid(const vec3& p1, const vec3& p2, const vec3& p3, const vec3& p4, const vec4& color, unsigned numDivisions);

		void DrawFullScreenQuad(Shader& program);
		void DrawFullScreenQuad(Texture& tex);

		void DrawSkyBox(Texture& tex);
		void DrawSkyBox(Texture& tex, Shader& program) const;

	private:
		bool isLoaded = false;

		Shader skyboxProgram;
		Shader lineProgram;
		Shader triangleProgram;
		Shader rectangleProgram;
		Shader texturedLineProgram;
		Shader texturedTriangleProgram;
		Shader texturedRectangleProgram;
		Shader texturedFullScreenQuadProgram;

		unsigned fullScreenVAO = 0;
		unsigned fullScreenVBO = 0;
		unsigned quadVAO = 0;
		unsigned quadVBO = 0;
		unsigned skyboxVAO = 0;
		unsigned skyboxVBO = 0;
		unsigned primitivesVAO = 0;
	} &Primitives = Singleton<class Primitives>::instanceRef;
}

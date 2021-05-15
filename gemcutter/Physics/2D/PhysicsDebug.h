#pragma once

#ifdef _DEBUG

#include "gemcutter/Math/Vector.h"
#include "gemcutter/Resource/Shader.h"
#include "gemcutter/Resource/VertexArray.h"

#include <box2d/b2_draw.h>

namespace gem
{
	// Queues debug items from box2D into arrays ready for drawing to the screen.
	class PhysicsDebug : public b2Draw
	{
	public:
		PhysicsDebug();
		~PhysicsDebug();

		PhysicsDebug(const PhysicsDebug&) = delete;
		PhysicsDebug(PhysicsDebug&&) = default;
		PhysicsDebug& operator=(const PhysicsDebug&) = delete;
		PhysicsDebug& operator=(PhysicsDebug&&) = default;

		bool Init();

		void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;
		void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) override;

		void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) override;
		void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) override;

		void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override;

		void DrawTransform(const b2Transform& xf) override;

		void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override;

		void Render() const;
		void Clear();

	private:
		bool CheckArraySpace(const VertexArray& array, int vertexCount);

		VertexBuffer::Ptr polygonData;
		VertexBuffer::Ptr polygonIndices;
		VertexArray::Ptr  polygonArray;

		VertexBuffer::Ptr lineData;
		VertexBuffer::Ptr lineIndices;
		VertexArray::Ptr  lineArray;

		Shader::Ptr debugShader;

		vec2* sphereVertices = nullptr;

		bool outputWarning = false;
	};
}
#endif

#include "PhysicsDebug.h"

#ifdef _DEBUG

#include "gemcutter/Math/Math.h"
#include "gemcutter/Math/Matrix.h"

#include <numeric>

namespace
{
	constexpr const char* DEBUG_SHADER_PROGRAM = R"(
		Attributes
		{
			vec2 a_vert  : 0;
			vec4 a_color : 1;
		}
		Vertex
		{
			out vec4 color;
			void main()
			{
				color = a_color;
				gl_Position = Gem_ViewProj * vec4(a_vert, 0.0, 1.0);
			}
		}
		Fragment
		{
			in vec4 color;
			out vec4 outColor;
			void main()
			{
				outColor = color;
			}
		}
	)";

	constexpr unsigned POS_SIZE     = sizeof(float) * 2;
	constexpr unsigned COLOR_SIZE   = sizeof(float) * 4;
	constexpr unsigned INDEX_SIZE   = sizeof(unsigned short);
	constexpr unsigned BUFFER_COUNT = 4096;
	constexpr unsigned BUFFER_SIZE  = BUFFER_COUNT * (POS_SIZE + COLOR_SIZE);
	constexpr unsigned COLOR_OFFSET = BUFFER_COUNT * POS_SIZE;
	constexpr unsigned CIRCLE_SEGMENTS = 17;
}

namespace gem
{
	PhysicsDebug::PhysicsDebug()
	{
		SetFlags(b2Draw::e_shapeBit | b2Draw::e_jointBit | b2Draw::e_centerOfMassBit);

		const float angle = (2.0f * M_PI) / (CIRCLE_SEGMENTS - 1);
		const float cosA = cos(angle);
		const float sinA = sin(angle);
		const mat2 rotation(
			cosA, -sinA,
			sinA, cosA
		);

		sphereVertices = static_cast<vec2*>(malloc(sizeof(vec2) * CIRCLE_SEGMENTS));
		sphereVertices[0] = vec2(0.0f, 1.0f);
		sphereVertices[CIRCLE_SEGMENTS - 1] = sphereVertices[0];
		for (unsigned i = 1; i < CIRCLE_SEGMENTS - 1; ++i)
		{
			sphereVertices[i] = rotation * sphereVertices[i - 1];
		}
	}

	PhysicsDebug::~PhysicsDebug()
	{
		free(sphereVertices);
	}

	bool PhysicsDebug::Init()
	{
		debugShader = Shader::MakeNew();
		if (!debugShader->LoadFromSource(DEBUG_SHADER_PROGRAM))
			return false;

		polygonArray = VertexArray::MakeNew();
		lineArray    = VertexArray::MakeNew();

		// Create the actual vertex storage.
		polygonData    = VertexBuffer::MakeNew(BUFFER_SIZE, VertexBufferUsage::Stream, VertexBufferType::Data);
		lineData       = VertexBuffer::MakeNew(BUFFER_SIZE, VertexBufferUsage::Stream, VertexBufferType::Data);
		polygonIndices = VertexBuffer::MakeNew(BUFFER_SIZE, VertexBufferUsage::Stream, VertexBufferType::Index);
		lineIndices    = VertexBuffer::MakeNew(BUFFER_SIZE, VertexBufferUsage::Stream, VertexBufferType::Index);

		// Create the two vertex attribute discriptions.
		VertexStream positionStream;
		positionStream.format = VertexFormat::Vec2;

		VertexStream colorStream;
		colorStream.format = VertexFormat::Vec4;
		colorStream.bindingUnit = 1;
		colorStream.startOffset = COLOR_OFFSET;

		// Apply the streams to each VAO.
		positionStream.buffer = polygonData;
		colorStream.buffer    = polygonData;
		polygonArray->AddStream(positionStream);
		polygonArray->AddStream(colorStream);
		polygonArray->SetIndexBuffer(polygonIndices);
		polygonArray->format = VertexArrayFormat::TriangleFan;

		positionStream.buffer = lineData;
		colorStream.buffer    = lineData;
		lineArray->AddStream(positionStream);
		lineArray->AddStream(colorStream);
		lineArray->SetIndexBuffer(lineIndices);
		lineArray->format = VertexArrayFormat::LineStrip;

		return true;
	}

	void PhysicsDebug::DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		if (!CheckArraySpace(*lineArray, vertexCount + 1))
			return;

		const unsigned currentCount = lineArray->GetVertexCount();

		{
			auto mapping = lineData->MapBuffer(VertexAccess::WriteOnly);
			mapping.SetData(POS_SIZE * currentCount, POS_SIZE * vertexCount, vertices);
			mapping.Fill(COLOR_OFFSET + COLOR_SIZE * currentCount, vertexCount, color);
		}

		{
			auto mapping = lineIndices->MapBuffer(VertexAccess::WriteOnly);
			mapping.FillIndexSequence(INDEX_SIZE * currentCount, vertexCount, static_cast<unsigned short>(currentCount));
			mapping.SetRestartIndex(INDEX_SIZE * (currentCount + vertexCount));
		}

		lineArray->SetVertexCount(currentCount + vertexCount + 1);
	}

	void PhysicsDebug::DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color)
	{
		if (!CheckArraySpace(*polygonArray, vertexCount + 1))
			return;

		DrawPolygon(vertices, vertexCount, color);

		const b2Color fillColor(color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, 0.5f);
		const unsigned currentCount = polygonArray->GetVertexCount();

		{
			auto mapping = polygonData->MapBuffer(VertexAccess::WriteOnly);
			mapping.SetData(POS_SIZE * currentCount, POS_SIZE * vertexCount, vertices);
			mapping.Fill(COLOR_OFFSET + COLOR_SIZE * currentCount, vertexCount, fillColor);
		}

		{
			auto mapping = polygonIndices->MapBuffer(VertexAccess::WriteOnly);
			mapping.FillIndexSequence(INDEX_SIZE * currentCount, vertexCount, static_cast<unsigned short>(currentCount));
			mapping.SetRestartIndex(INDEX_SIZE * (currentCount + vertexCount));
		}

		polygonArray->SetVertexCount(currentCount + vertexCount + 1);
	}

	void PhysicsDebug::DrawCircle(const b2Vec2& center, float radius, const b2Color& color)
	{
		b2Vec2 vertices[CIRCLE_SEGMENTS];
		for (unsigned i = 0; i < CIRCLE_SEGMENTS; ++i)
		{
			vertices[i].x = center.x + sphereVertices[i].x * radius;
			vertices[i].y = center.y + sphereVertices[i].y * radius;
		}

		DrawPolygon(vertices, CIRCLE_SEGMENTS, color);
	}

	void PhysicsDebug::DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color)
	{
		b2Vec2 vertices[CIRCLE_SEGMENTS];
		for (unsigned i = 0; i < CIRCLE_SEGMENTS; ++i)
		{
			vertices[i].x = center.x + sphereVertices[i].x * radius;
			vertices[i].y = center.y + sphereVertices[i].y * radius;
		}
		DrawSolidPolygon(vertices, CIRCLE_SEGMENTS, color);

		b2Vec2 scaledAxis;
		scaledAxis.x = center.x + axis.x * radius;
		scaledAxis.y = center.y + axis.y * radius;
		DrawSegment(center, scaledAxis, color);
	}

	void PhysicsDebug::DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color)
	{
		if (!CheckArraySpace(*lineArray, 2 + 1))
			return;

		const unsigned currentCount = lineArray->GetVertexCount();

		{
			auto mapping = lineData->MapBuffer(VertexAccess::WriteOnly);
			mapping.SetElement(POS_SIZE * currentCount, p1);
			mapping.SetElement(POS_SIZE * (currentCount + 1), p2);
			mapping.Fill(COLOR_OFFSET + COLOR_SIZE * currentCount, 2, color);
		}

		{
			auto mapping = lineIndices->MapBuffer(VertexAccess::WriteOnly);
			mapping.FillIndexSequence(INDEX_SIZE * currentCount, 2, static_cast<unsigned short>(currentCount));
			mapping.SetRestartIndex(INDEX_SIZE * (currentCount + 2));
		}
		lineArray->SetVertexCount(currentCount + 2 + 1);
	}

	void PhysicsDebug::DrawTransform(const b2Transform& xf)
	{
		float cosValue = xf.q.c;
		float sinValue = xf.q.s;
		mat2 rotation(
			cosValue, -sinValue,
			sinValue, cosValue
		);

		float axisScale = 0.66f;
		vec2 up    = rotation * vec2(0.0f, axisScale);
		vec2 right = rotation * vec2(axisScale, 0.0f);

		DrawSegment(xf.p, { up.x + xf.p.x, up.y + xf.p.y }, b2Color(0.418f, 0.664f, 0.195f));
		DrawSegment(xf.p, { right.x + xf.p.x, right.y + xf.p.y }, b2Color(0.746f, 0.191f, 0.098f));
	}

	void PhysicsDebug::DrawPoint(const b2Vec2& p, float size, const b2Color& color)
	{
		float halfSize = size * 0.5f;
		b2Vec2 vertices[4] = {
			{ p.x - halfSize, p.y - halfSize },
			{ p.x - halfSize, p.y + halfSize },
			{ p.x + halfSize, p.y + halfSize },
			{ p.x + halfSize, p.y - halfSize }
		};

		DrawSolidPolygon(vertices, 4, color);
	}

	void PhysicsDebug::Render() const
	{
		SetCullFunc(CullFunc::None);
		SetDepthFunc(DepthFunc::None);
		SetBlendFunc(BlendFunc::Linear);
		debugShader->Bind();

		polygonArray->Bind();
		polygonArray->Draw();

		lineArray->Bind();
		lineArray->Draw();
	}

	void PhysicsDebug::Clear()
	{
		polygonArray->SetVertexCount(0);
		lineArray->SetVertexCount(0);
	}

	bool PhysicsDebug::CheckArraySpace(const VertexArray& array, int vertexCount)
	{
		unsigned oldCount = array.GetVertexCount();

		if (oldCount + vertexCount > BUFFER_COUNT)
		{
			if (!outputWarning)
			{
				Warning("Physics debug buffers exceeded. Some debug visuals will not be rendered.");
				outputWarning = true;
			}

			return false;
		}

		return true;
	}
}
#endif

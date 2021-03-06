// Copyright (c) 2017 Emilian Cioca
#pragma once
#include "gemcutter/Application/Logging.h"
#include "gemcutter/Rendering/Rendering.h"
#include "gemcutter/Resource/Shareable.h"

#include <vector>

namespace gem
{
	class VertexBuffer;
	class VertexArray;

	// Enables direct manipulation of a VertexBuffer's data.
	// The lifetime of this object should be kept as short as possible.
	// This is more performant than VertexBuffer functions when multiple operations are needed.
	class BufferMapping
	{
	public:
		BufferMapping(VertexBuffer& buffer, VertexAccess accessMode);
		~BufferMapping();

		void ZeroData(unsigned start, unsigned size);
		void SetData(unsigned start, unsigned size, const void* source);
		char* GetPtr();

		template<typename T>
		void SetElement(unsigned offset, const T& element);
		template<typename T>
		void Fill(unsigned start, unsigned count, const T& element);

		// For use only with 'unsigned short' index buffers.
		void FillIndexSequence(unsigned start, unsigned count, unsigned short firstIndex);
		void SetRestartIndex(unsigned offset);

	private:
		VertexBuffer& buffer;
		char* data;
	};

	// A single OpenGL buffer object.
	// This owns and provides access to the raw data used for rendering.
	class VertexBuffer : public Shareable<VertexBuffer>
	{
		friend BufferMapping;
		friend VertexArray;
	public:
		VertexBuffer(unsigned size, VertexBufferUsage usage, VertexBufferType type);
		~VertexBuffer();

		void ClearData();
		void ClearData(unsigned start, unsigned size);
		void SetData(unsigned start, unsigned size, const void* source);

		BufferMapping MapBuffer(VertexAccess accessMode);

		unsigned GetSize() const;
		VertexBufferUsage GetBufferUsage() const;
		VertexBufferType GetBufferType() const;

		// Special index value used to restart Triangle/line fans/strips/loops.
		static const unsigned short RESTART_INDEX;

	private:
		void Bind() const;
		void UnBind() const;

		unsigned VBO = 0;
		unsigned size = 0;
		VertexBufferUsage usage;
		int target;
	};

	// A single vertex attribute to be streamed to a vertex shader.
	// Defines how to read the attribute from the given VertexBuffer.
	struct VertexStream
	{
		VertexBuffer::Ptr buffer;
		unsigned bindingUnit = 0;

		VertexFormat format = VertexFormat::Float;
		bool normalized = false;
		unsigned startOffset = 0;
		unsigned stride = 0;
		unsigned divisor = 0;
	};

	namespace detail
	{
		template<typename Type>
		class VertexRange;
	}

	// A renderable collection of VertexStreams.
	class VertexArray : public Shareable<VertexArray>
	{
	public:
		VertexArray();
		~VertexArray();

		void SetIndexBuffer(VertexBuffer::Ptr buffer);
		const VertexBuffer* GetIndexBuffer() const;
		VertexBuffer* GetIndexBuffer();

		void AddStream(VertexStream ptr);
		void RemoveStream(unsigned bindingUnit);
		bool HasStream(unsigned bindingUnit) const;
		const VertexStream& GetStream(unsigned bindingUnit) const;
		const VertexBuffer& GetBuffer(unsigned bindingUnit) const;
		VertexBuffer& GetBuffer(unsigned bindingUnit);

		// Returns an enumerable range over the specific elements pointed to by a VertexStream.
		// Elements will be casted to 'Type'. You must ensure that this cast is safe for the contents of the VertexBuffer.
		// The full buffer will be enumerated, not just 'VertexCount' number of elements.
		template<typename Type>
		detail::VertexRange<Type> GetStream(unsigned bindingUnit, VertexAccess access = VertexAccess::ReadWrite);

		void Bind() const;
		void UnBind() const;

		// Renders the array using any currently bound UniformBuffers and Textures.
		void Draw() const;
		void Draw(unsigned firstIndex) const;
		void Draw(unsigned firstIndex, VertexArrayFormat formatOverride) const;

		void SetVertexCount(unsigned count);
		unsigned GetVertexCount() const;
		const auto& GetStreams() const { return streams; }

		VertexArrayFormat format = VertexArrayFormat::Triangle;

	private:
		unsigned VAO = 0;
		unsigned vertexCount = 0;

		VertexBuffer::Ptr indexBuffer;
		std::vector<VertexStream> streams;
	};
}

#include "VertexArray.inl"

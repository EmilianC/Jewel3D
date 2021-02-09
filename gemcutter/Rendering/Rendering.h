// Copyright (c) 2017 Emilian Cioca
#pragma once
#include <string_view>

namespace gem
{
	struct vec4;

	// Binding locations for built-in uniform buffers.
	enum class UniformBufferSlot : unsigned
	{
		Camera = 10,
		Model = 11,
		Engine = 12,
		Time = 13,
		Particle = 14
	};

	enum class VertexFormat
	{
		Float,
		Double,
		Vec2,
		Vec3,
		Vec4,
		Mat2,
		Mat3,
		Mat4,
		Int,
		uInt,
		Short,
		uShort,
		Byte,
		uByte
	};

	enum class VertexArrayFormat
	{
		Point,
		Line,
		LineAdjacency,
		LineLoop,
		LineStrip,
		LineStripAdjacency,
		Triangle,
		TriangleAdjacency,
		TriangleFan,
		TriangleStrip,
		TriangleStripAdjacency
	};

	enum class VertexAccess
	{
		ReadOnly,
		WriteOnly,
		ReadWrite
	};

	enum class VertexBufferUsage
	{
		// The buffer is not expected to change and will be kept on the GPU only.
		Static,
		// The buffer is expected to be occasionally updated. It will be optimized for editing.
		Dynamic,
		// The buffer is expected to be updated before each render. It will be optimized for streaming to the GPU.
		Stream
	};

	enum class VertexBufferType
	{
		// The buffer contains vertex data or attributes.
		Data,
		// The buffer contains indices defining primitive ordering.
		Index
	};

	enum class TextureFormat
	{
		R_8,
		R_16,
		R_16F,
		R_32,
		R_32F,
		RGB_8,
		RGB_16,
		RGB_16F,
		RGB_32,
		RGB_32F,
		RGBA_8,
		RGBA_16,
		RGBA_16F,
		RGBA_32,
		RGBA_32F,
		DEPTH_24,
		sRGB_8,
		sRGBA_8
	};

	enum class TextureWrap
	{
		Clamp,
		ClampWithBorder,
		Repeat,
		RepeatMirrored,
		RepeatMirroredOnce
	};

	struct TextureWraps
	{
		TextureWraps() = default;
		TextureWraps(TextureWrap xy);
		TextureWraps(TextureWrap x, TextureWrap y);

		TextureWrap x = TextureWrap::Clamp;
		TextureWrap y = TextureWrap::Clamp;
	};

	enum class TextureFilter
	{
		Point,
		Linear,
		Bilinear,
		Trilinear
	};

	enum class CullFunc
	{
		// Both back and front faces are rendered.
		None,
		// Clockwise face is culled from rendering. This is the default behaviour.
		Clockwise,
		// Counter-clockwise face is culled from rendering.
		CounterClockwise
	};

	enum class BlendFunc
	{
		// Blending is disabled.
		None,
		// Color is interpolated with the pixels in the renderTarget based on alpha.
		Linear,
		// Color is added onto the pixels in the renderTarget.
		Additive,
		// Color is multiplied with the pixels in the renderTarget.
		Multiplicative
	};

	enum class DepthFunc
	{
		None,
		WriteOnly,
		TestOnly,
		Normal
	};

	enum class VSyncMode
	{
		// Frames are sent to display device as soon as they are ready.
		// Screen tearing can occur but input is most responsive.
		Off = 0,
		// Framerate is locked to multiples of the display's refresh rate to eliminate tearing.
		// Can introduce input lag or stuttering.
		On = 1,
		// Vsync is on when the framerate exceeds the display's refresh rate to eliminate tearing.
		// When the framerate drops, Vsync turns off to eliminate stuttering and input lag.
		Adaptive = -1
	};

	// Used internally to convert enum values to OpenGL values.
	int ResolveVertexFormat(VertexFormat);
	int ResolveVertexArrayFormat(VertexArrayFormat);
	int ResolveVertexAccess(VertexAccess);
	int ResolveVertexBufferUsage(VertexBufferUsage);
	int ResolveFilterMag(TextureFilter);
	int ResolveFilterMin(TextureFilter);
	bool ResolveMipMapping(TextureFilter);
	int ResolveWrap(TextureWrap);
	unsigned ResolveFormat(TextureFormat);
	unsigned ResolveDataFormat(TextureFormat);

	TextureFilter StringToTextureFilter(std::string_view);
	TextureWrap StringToTextureWrap(std::string_view);
	CullFunc StringToCullFunc(std::string_view);
	BlendFunc StringToBlendFunc(std::string_view);
	DepthFunc StringToDepthFunc(std::string_view);

	unsigned CountBytes(VertexFormat);
	unsigned CountMipLevels(unsigned width, unsigned height, TextureFilter);
	unsigned CountChannels(TextureFormat);

	void ClearBackBuffer();
	void ClearBackBufferDepth();
	void ClearBackBufferColor();

	void SetClearColor(const vec4& color);
	void SetClearColor(float r, float g, float b, float a);
	void SetWireframe(bool enabled);
	void SetCullFunc(CullFunc func);
	void SetBlendFunc(BlendFunc func);
	void SetDepthFunc(DepthFunc func);
	void SetViewport(unsigned x, unsigned y, unsigned width, unsigned height);
	void SetScissor(unsigned x, unsigned y, unsigned width, unsigned height);
	void SetScissor(bool enabled);
	bool SetVSync(VSyncMode mode);

	// Saves a screenshot of the currently bound RenderTarget or the backbuffer.
	// This is a very slow function and should only be used for debugging or when absolutely necessary.
	bool SaveScreenshot(std::string_view filePath, unsigned x, unsigned y, unsigned width, unsigned height);

	extern class GPUInfoSingleton GPUInfo;
	class GPUInfoSingleton
	{
		friend class ApplicationSingleton;
	public:
		unsigned GetMaxTextureSlots() const;
		unsigned GetMaxUniformBufferSlots() const;
		unsigned GetMaxRenderTargetTextures() const;
		unsigned GetMaxDrawBuffers() const;

	private:
		void ScanDevice();

		unsigned maxTextureSlots = 0;
		unsigned maxUniformBufferSlots = 0;
		unsigned maxRenderTargetTextures = 0;
		unsigned maxDrawBuffers = 0;
	};
}

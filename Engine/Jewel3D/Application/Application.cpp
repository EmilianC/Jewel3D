// Copyright (c) 2017 Emilian Cioca
#include "Jewel3D/Precompiled.h"
#include "Application.h"
#include "Logging.h"
#include "Timer.h"
#include "Jewel3D/Input/Input.h"
#include "Jewel3D/Math/Math.h"
#include "Jewel3D/Rendering/Light.h"
#include "Jewel3D/Rendering/ParticleEmitter.h"
#include "Jewel3D/Rendering/Rendering.h"
#include "Jewel3D/Resource/Font.h"
#include "Jewel3D/Resource/Model.h"
#include "Jewel3D/Resource/ParticleBuffer.h"
#include "Jewel3D/Resource/Shader.h"
#include "Jewel3D/Resource/Texture.h"
#include "Jewel3D/Sound/SoundSystem.h"

#include <GLEW/GL/glew.h>
#include <GLEW/GL/wglew.h>

// This is the HINSTANCE of the application.
extern "C" IMAGE_DOS_HEADER __ImageBase;

#define BITS_PER_PIXEL			32
#define DEPTH_BUFFER_BITS		24
#define STENCIL_BUFFER_BITS		8

#define STYLE_BORDERED			(WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_VISIBLE)
#define STYLE_BORDERED_FIXED	(WS_CAPTION | WS_SYSMENU | WS_POPUP	| WS_MINIMIZEBOX | WS_VISIBLE)
#define STYLE_BORDERLESS		(WS_POPUP | WS_VISIBLE)
#define STYLE_EXTENDED			(WS_EX_APPWINDOW)

namespace
{
	static LONG GetStyle(bool fullScreen, bool bordered, bool resizable)
	{
		if (bordered && !fullScreen)
		{
			if (resizable)
			{
				return STYLE_BORDERED;
			}
			else
			{
				return STYLE_BORDERED_FIXED;
			}
		}
		else
		{
			return STYLE_BORDERLESS;
		}
	}

	static RECT GetWindowSize(LONG style, unsigned clientWidth, unsigned clientHeight)
	{
		RECT windowRect;
		windowRect.left = 0;
		windowRect.right = clientWidth;
		windowRect.top = 0;
		windowRect.bottom = clientHeight;

		if (!AdjustWindowRectEx(&windowRect, style, FALSE, STYLE_EXTENDED))
		{
			Jwl::Warning("Console: Could not resolve the window's size.");
		}

		return windowRect;
	}

	static bool ApplyFullscreen(HWND window, bool state, unsigned clientWidth, unsigned clientHeight)
	{
		if (state)
		{
			DEVMODE dmScreenSettings;
			memset(&dmScreenSettings, 0, sizeof(dmScreenSettings));

			dmScreenSettings.dmSize = sizeof(DEVMODE);
			dmScreenSettings.dmPelsWidth = clientWidth;
			dmScreenSettings.dmPelsHeight = clientHeight;
			dmScreenSettings.dmBitsPerPel = BITS_PER_PIXEL;
			dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

			if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
			{
				Jwl::Warning("Console: Could not enter fullscreen mode.");
				return false;
			}

			if (SetWindowPos(window, 0, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW) == ERROR)
			{
				Jwl::Warning("Console: Could not enter fullscreen mode.");
				return false;
			}
		}
		else
		{
			if (ChangeDisplaySettings(NULL, 0) != DISP_CHANGE_SUCCESSFUL)
			{
				Jwl::Warning("Console: Could not successfully exit fullscreen mode.");
				return false;
			}

			if (SetWindowPos(window, 0, 20, 20, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_SHOWWINDOW) == ERROR)
			{
				Jwl::Warning("Console: Could not successfully exit fullscreen mode.");
				return false;
			}
		}

		return true;
	}

	static bool ApplyStyle(HWND window, LONG style, unsigned clientWidth, unsigned clientHeight)
	{
		RECT windowRect = GetWindowSize(style, clientWidth, clientHeight);

		if (SetWindowLongPtr(window, GWL_STYLE, style) == ERROR ||
			SetWindowPos(window, 0, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW) == ERROR)
		{
			Jwl::Warning("Console: Could not apply the window's border style.");
			return false;
		}

		return true;
	}

	static bool ApplyResolution(HWND window, LONG style, unsigned clientWidth, unsigned clientHeight)
	{
		RECT windowRect = GetWindowSize(style, clientWidth, clientHeight);
		if (SetWindowPos(window, 0, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW) == ERROR)
		{
			Jwl::Warning("Console: Could not apply the resolution.");
			return false;
		}

		return true;
	}

	#ifdef _DEBUG
	static void CALLBACK OpenGLDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* msg, const void* data)
	{
		// Unused argument.
		data = nullptr;

		// Ignored messages.
		if (
			// Usage warning : Generic vertex attribute array # uses a pointer with a small value (#). Is this intended to be used as an offset into a buffer object?
			id == 0x00020004 || 
			// Buffer info : Total VBO memory usage in the system : #
			id == 0x00020070 ||
			// Program / shader state info : GLSL shader # failed to compile.
			id == 0x00020090)
		{
			return;
		}

		char buffer[9] = { '\0' };
		sprintf(buffer, "%.8x", id);

		std::string message("OpenGL(0x");
		message += buffer;
		message += "): ";

		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
			message += "Error\n";
			break;
		case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
			message += "Deprecated behaviour\n";
			break;
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			message += "Undefined behaviour\n";
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			message += "Portability issue\n";
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			message += "Performance issue\n";
			break;
		case GL_DEBUG_TYPE_MARKER:
			message += "Stream annotation\n";
			break;
		case GL_DEBUG_TYPE_OTHER_ARB:
			message += "Other\n";
			break;
		}

		message += "Source: ";
		switch (source)
		{
		case GL_DEBUG_SOURCE_API:
			message += "API\n";
			break;
		case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
			message += "Window system\n";
			break;
		case GL_DEBUG_SOURCE_SHADER_COMPILER:
			message += "Shader compiler\n";
			break;
		case GL_DEBUG_SOURCE_THIRD_PARTY:
			message += "Third party\n";
			break;
		case GL_DEBUG_SOURCE_APPLICATION:
			message += "Application\n";
			break;
		case GL_DEBUG_SOURCE_OTHER:
			message += "Other\n";
			break;
		}

		message += "Severity: ";
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:
			message += "HIGH\n";
			break;
		case GL_DEBUG_SEVERITY_MEDIUM:
			message += "Medium\n";
			break;
		case GL_DEBUG_SEVERITY_LOW:
			message += "Low\n";
			break;
		case GL_DEBUG_SEVERITY_NOTIFICATION:
			message += "Notification\n";
			break;
		}

		message.append(msg, length);
		if (message.back() != '\n')
		{
			message.push_back('\n');
		}

		if (type == GL_DEBUG_TYPE_ERROR)
		{
			Jwl::Error(message);
		}
		else
		{
			Jwl::Log(message);
		}
	}
	#endif
}

namespace Jwl
{
	void Application::DrainEventQueue()
	{
		// Windows message loop.
		MSG msg;
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			// Filter input events from other system/windows events and send them to the Input class.
			if ((msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) || (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST))
			{
				// Send WM_CHAR message for capturing character input.
				TranslateMessage(&msg);

				if (!Input.Update(msg))
				{
					// If the msg was not handled by the input class, we forward it to the default WndProc.
					DefWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
				}
			}
			else
			{
				// This message is not input based, send it to our main WndProc.
				DispatchMessage(&msg);
			}
		}
	}

	bool Application::CreateGameWindow(const std::string& title, unsigned _glMajorVersion, unsigned _glMinorVersion)
	{
		ASSERT(hwnd == NULL, "A game window is already open.");
		ASSERT(_glMajorVersion > 3 || (_glMajorVersion == 3 && _glMinorVersion == 3), "OpenGL version must be 3.3 or greater.");

		apInstance = reinterpret_cast<HINSTANCE>(&__ImageBase);

		glMajorVersion = _glMajorVersion;
		glMinorVersion = _glMinorVersion;

		// Prepare our window parameters.
		WNDCLASSEX windowClass;
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = Application::WndProc;			// We set our static method as the event handler
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = 0;
		windowClass.hInstance = apInstance;
		windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);	// default icon
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);		// default arrow
		windowClass.hbrBackground = NULL;						// don't need background
		windowClass.lpszMenuName = NULL;						// no menu
		windowClass.lpszClassName = "Jewel3D";
		windowClass.hIconSm = LoadIcon(NULL, IDI_WINLOGO);		// windows logo small icon

		// Send our window parameters to windows.
		if (!RegisterClassEx(&windowClass))
		{
			Error("Console: Failed to register window.");
			return false;
		}

		LONG style = GetStyle(fullscreen, bordered, resizable);
		RECT windowRect = GetWindowSize(style, screenViewport.width, screenViewport.height);

		// Create window. this sends a WM_CREATE message that is handled before the function returns.
		// Hwnd is set when handling WM_CREATE in the WinProc.
		HWND window = CreateWindowEx(
			STYLE_EXTENDED,			// extended style
			"Jewel3D",				// class name
			title.c_str(),			// application name
			style,					// border style
			CW_USEDEFAULT, CW_USEDEFAULT,		// x,y position of window
			windowRect.right - windowRect.left,	// dimensions of the window
			windowRect.bottom - windowRect.top,
			NULL,					// handle to parent
			NULL,					// handle to menu
			apInstance,				// application instance
			NULL);					// extra data

		if (window == NULL)
		{
			Error("Console: Failed to create window.");
			return false;
		}

		SetFullscreen(fullscreen);

		ShowWindow(hwnd, SW_SHOW);
		SetForegroundWindow(hwnd);

		// Ensure that the deltaTime between frames has been computed.
		SetUpdatesPerSecond(updatesPerSecond);

		return true;
	}

	void Application::DestroyGameWindow()
	{
		ASSERT(hwnd != NULL, "A game window must be created before calling this function.");

		// Delete all resources that require the OpenGL context.
		UnloadAll<Font>();
		UnloadAll<Shader>();
		UnloadAll<Texture>();
		UnloadAll<Model>();
		UnloadAll<ParticleBuffer>();
	
		if (IsFullscreen())
		{
			SetFullscreen(false);
		}
		
		EnableCursor();

		// Release Device and Render Contexts.
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(renderContext);
		ReleaseDC(hwnd, deviceContext);
		DestroyWindow(hwnd);
		UnregisterClass("Jewel3D", apInstance);

		renderContext = NULL;
		deviceContext = NULL;
		hwnd = NULL;
	}

	void Application::GameLoop(std::function<void()> update, std::function<void()> draw)
	{
		ASSERT(update, "An update function must be provided.");
		ASSERT(draw, "A draw function must be provided.");

		if (hwnd == NULL)
		{
			Error("Application: Must be initialized and have a window created before a call to GameLoop().");
			return;
		}

		//- Timing control variables.
		constexpr unsigned MAX_CONCURRENT_UPDATES = 5;
		unsigned fpsCounter = 0;
		__int64 lastUpdate = Timer::GetCurrentTick();
		__int64 lastRender = lastUpdate;
		__int64 lastFpsCapture = lastRender;

		while (appIsRunning)
		{
			// Updates our input and Windows OS events.
			DrainEventQueue();

			// Record the FPS for the previous second of time.
			__int64 currentTime = Timer::GetCurrentTick();
			if (currentTime - lastFpsCapture >= Timer::GetTicksPerSecond())
			{
				// We don't want to update the timer variable with "+= 1.0" here. After a lag spike this 
				// would cause FPS to suddenly be recorded more often than once a second.
				lastFpsCapture = currentTime;

				fps = fpsCounter;
				fpsCounter = 0;
			}

			// Update to keep up with real time.
			unsigned updateCount = 0;
			while (currentTime - lastUpdate >= updateStep)
			{
				update();

				lastUpdate += updateStep;
				updateCount++;

				// Avoid spiral of death. This also allows us to keep rendering even in a worst-case scenario.
				if (updateCount >= MAX_CONCURRENT_UPDATES)
					break;
			}

			if (updateCount > 0)
			{
				// If the frame rate is uncapped or we are due for a new frame, render the latest game-state.
				if (FPSCap == 0 || (currentTime - lastRender) >= renderStep)
				{
					draw();
					SwapBuffers(deviceContext);

					lastRender += renderStep;
					fpsCounter++;
				}
			}
		}
	}

	void Application::UpdateEngine()
	{
		// Distribute all queued events to their listeners.
		EventQueue.Dispatch();

		// Update engine components.
		for (auto& emitter : All<ParticleEmitter>())
		{
			emitter.Update();
		}

		for (auto& light : All<Light>())
		{
			light.Update();
		}

		// Step the SoundSystem.
		SoundSystem.Update();
	}

	void Application::Exit()
	{
		appIsRunning = false;
	}

	void Application::EnableCursor()
	{
		while (ShowCursor(true) <= 0);
	}

	void Application::DisableCursor()
	{
		while (ShowCursor(false) > 0);
	}

	bool Application::IsFullscreen()
	{
		return fullscreen;
	}

	bool Application::IsBordered()
	{
		return bordered;
	}

	bool Application::IsResizable()
	{
		return resizable;
	}

	std::string Application::GetOpenGLVersionString() const
	{
		ASSERT(hwnd != NULL, "A game window must be created before calling this function.");

		return std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	}

	int Application::GetScreenWidth() const
	{
		return screenViewport.width;
	}

	int Application::GetScreenHeight() const
	{
		return screenViewport.height;
	}

	float Application::GetAspectRatio() const
	{
		return screenViewport.GetAspectRatio();
	}

	float Application::GetDeltaTime() const
	{
		return 1.0f / updatesPerSecond;
	}

	unsigned Application::GetFPS() const
	{
		return fps;
	}

	void Application::SetFPSCap(unsigned _fps)
	{
		if (_fps == 0)
		{
			renderStep = 0;
		}
		else
		{
			renderStep = Timer::GetTicksPerSecond() / _fps;
		}

		FPSCap = _fps;

		if (FPSCap > updatesPerSecond)
		{
			Warning("FPSCap is higher than the Update rate. Frames cannot be rendered more often than there are updates.");
		}
	}
	
	unsigned Application::GetFPSCap() const
	{
		return FPSCap;
	}

	void Application::SetUpdatesPerSecond(unsigned ups)
	{
		ASSERT(ups > 0, "Invalid update rate.");

		updateStep = Timer::GetTicksPerSecond() / ups;

		updatesPerSecond = ups;
	}

	unsigned Application::GetUpdatesPerSecond() const
	{
		return updatesPerSecond;
	}

	const Viewport& Application::GetScreenViewport() const
	{
		return screenViewport;
	}

	bool Application::SetFullscreen(bool state)
	{
		if (hwnd != NULL)
		{
			if (state == fullscreen)
			{
				return true;
			}

			LONG style;
			if (state)
			{
				style = GetStyle(true, false, false);
			}
			else
			{
				style = GetStyle(false, IsBordered(), IsResizable());
			}
			
			if (!ApplyStyle(hwnd, style, GetScreenWidth(), GetScreenHeight()))
			{
				return false;
			}

			if (!ApplyFullscreen(hwnd, state, GetScreenWidth(), GetScreenHeight()))
			{
				return false;
			}
		}

		fullscreen = state;
		return true;
	}

	bool Application::SetBordered(bool state)
	{
		if (hwnd != NULL)
		{
			if (state == bordered)
			{
				return true;
			}
			else if (IsFullscreen())
			{
				// State will be applied next time we exit fullscreen mode.
				bordered = state;
				return true;
			}

			LONG style = GetStyle(false, state, IsResizable());
			if (!ApplyStyle(hwnd, style, GetScreenWidth(), GetScreenHeight()))
			{
				return false;
			}
		}

		bordered = state;
		return true;
	}

	bool Application::SetResizable(bool state)
	{
		if (hwnd != NULL)
		{
			if (state == resizable)
			{
				return true;
			}

			if (IsFullscreen() || !IsBordered())
			{
				// State will be applied next time we exit fullscreen mode and have a border.
				resizable = state;
				return true;
			}

			LONG style = GetStyle(false, true, state);
			if (!ApplyStyle(hwnd, style, GetScreenWidth(), GetScreenHeight()))
			{
				return false;
			}
		}
		
		resizable = state;
		return true;
	}

	bool Application::SetResolution(unsigned width, unsigned height)
	{
		ASSERT(width > 0, "'width' must be greater than 0.");
		ASSERT(height > 0, "'height' must be greater than 0.");

		bool wasFullscreen = fullscreen;

		if (hwnd != NULL)
		{
			if (width == screenViewport.width && height == screenViewport.height)
			{
				return true;
			}

			if (wasFullscreen && !SetFullscreen(false))
			{
				return false;
			}

			LONG style = GetStyle(fullscreen, IsBordered(), IsResizable());
			if (!ApplyResolution(hwnd, style, width, height))
			{
				return false;
			}
		}

		screenViewport.width = width;
		screenViewport.height = height;

		if (hwnd != NULL)
		{
			if (wasFullscreen && !SetFullscreen(true))
			{
				return false;
			}
		}

		return true;
	}

	Application::Application()
		: glMajorVersion(3)
		, glMinorVersion(3)
		, screenViewport(0, 0, 800, 600)
		, hwnd(NULL)
		, apInstance(NULL)
		, renderContext(NULL)
		, deviceContext(NULL)
	{
	}

	Application::~Application()
	{
		if (hwnd != NULL)
		{
			DestroyGameWindow();
		}
	}

	LRESULT CALLBACK Application::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		auto& app = Application::instanceRef;

		switch (uMsg)
		{
		case WM_SIZE:
		{
			// Adjust screen data.
			app.screenViewport.width = LOWORD(lParam);
			app.screenViewport.height = HIWORD(lParam);
			app.screenViewport.bind();

			EventQueue.Push(std::make_unique<Resize>(app.screenViewport.width, app.screenViewport.height));
			return 0; break;
		}

		case WM_CREATE:
		{
			app.hwnd = hWnd;
			app.deviceContext = GetDC(hWnd);

			if (app.deviceContext == 0)
			{
				Error("Console: DeviceContext could not be created.");
				return -1;
			}

			/* Initialize OpenGL */
			PIXELFORMATDESCRIPTOR pfd;
			memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR));

			pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
			pfd.nVersion		= 1;
			pfd.dwFlags			= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
			pfd.iPixelType		= PFD_TYPE_RGBA;
			pfd.cColorBits		= BITS_PER_PIXEL;
			pfd.cDepthBits		= DEPTH_BUFFER_BITS;
			pfd.cStencilBits	= STENCIL_BUFFER_BITS;
			pfd.iLayerType		= PFD_MAIN_PLANE;

			int pixelFormat = ChoosePixelFormat(app.deviceContext, &pfd);
			SetPixelFormat(app.deviceContext, pixelFormat, &pfd);

			int attribs[] = {
				WGL_CONTEXT_MAJOR_VERSION_ARB, static_cast<int>(app.glMajorVersion),
				WGL_CONTEXT_MINOR_VERSION_ARB, static_cast<int>(app.glMinorVersion),
				WGL_CONTEXT_PROFILE_MASK_ARB,
				WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
				WGL_CONTEXT_FLAGS_ARB,
#ifdef _DEBUG
				WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB | WGL_CONTEXT_DEBUG_BIT_ARB,
#else
				WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
#endif
				//WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
				//WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
				//WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
				//WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
				//WGL_COLOR_BITS_ARB, 32,
				//WGL_DEPTH_BITS_ARB, 24,
				//WGL_STENCIL_BITS_ARB, 8,
				//WGL_SAMPLE_BUFFERS_ARB, GL_TRUE,
				//WGL_SAMPLES_ARB, 16,
				0 }; // zero indicates the end of the array

			// Create a temporary context so we can get a pointer to our newer context
			HGLRC tmpContext = wglCreateContext(app.deviceContext);
			// Make it current
			if (!wglMakeCurrent(app.deviceContext, tmpContext))
			{
				Error("Console: Device-Context could not be made current.\nTry updating your graphics drivers.");
				return -1;
			}

			// Get the function we can use to generate a modern context.
			PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

			if (wglCreateContextAttribsARB)
			{
				// Create modern OpenGL context using the new function and delete the old one.
				app.renderContext = wglCreateContextAttribsARB(app.deviceContext, 0, attribs);
			}

			if (app.renderContext == NULL)
			{
				Error("Console: Failed to create OpenGL rendering context.\nTry updating your graphics drivers.");
				return -1;
			}

			// Switch to new context.
			wglMakeCurrent(app.deviceContext, NULL);
			wglDeleteContext(tmpContext);
			if (!wglMakeCurrent(app.deviceContext, app.renderContext))
			{
				Error("Console: Render-Context could not be made current.\nTry updating your graphics drivers.");
				return -1;
			}

			glewExperimental = GL_TRUE;
			auto glewResult = glewInit();
			if (glewResult != GLEW_OK)
			{
				Error("Console: GLEW failed to initialize. ( %s )\nTry updating your graphics drivers.",
					reinterpret_cast<const char*>(glewGetErrorString(glewResult)));

				return -1;
			}

			// GLEW can sometimes cause a GL_INVALID_ENUM error. 
			// We pop it off the error stack here.
			glGetError();

#ifdef _DEBUG
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(OpenGLDebugCallback, NULL);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, 0, GL_FALSE);
#endif

			// Setup OpenGL settings.
			GPUInfo.ScanDevice();
			SetCullFunc(CullFunc::Clockwise);
			SetDepthFunc(DepthFunc::Normal);
		}
		return 0; break;

		case WM_CLOSE:
		case WM_DESTROY:
			app.Exit();
			return 0; break;

		case WM_SYSCOMMAND:
			// Here we block screen savers and monitor power-saving modes.
			if (wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)
			{
				return 0;
			}
			break;
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	Resize::Resize(unsigned _width, unsigned _height)
		: width(_width)
		, height(_height)
	{
	}

	float Resize::GetAspectRatio() const
	{
		return static_cast<float>(width) / static_cast<float>(height);
	}
}

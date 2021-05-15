list(APPEND gemcutter_files
	"AI/ProbabilityMatrix.cpp"
	"AI/ProbabilityMatrix.h"

	"Application/Application.cpp"
	"Application/Application.h"
	"Application/CmdArgs.cpp"
	"Application/CmdArgs.h"
	"Application/Delegate.cpp"
	"Application/Delegate.h"
	"Application/Delegate.inl"
	"Application/Event.cpp"
	"Application/Event.h"
	"Application/Event.inl"
	"Application/FileSystem.cpp"
	"Application/FileSystem.h"
	"Application/HierarchicalEvent.h"
	"Application/Logging.cpp"
	"Application/Logging.h"
	"Application/Threading.cpp"
	"Application/Threading.h"
	"Application/Timer.cpp"
	"Application/Timer.h"

	"Entity/Entity.cpp"
	"Entity/Entity.h"
	"Entity/Entity.inl"
	"Entity/Hierarchy.cpp"
	"Entity/Hierarchy.h"
	"Entity/Name.cpp"
	"Entity/Name.h"
	"Entity/Query.inl"

	"Input/Input.cpp"
	"Input/Input.h"
	"Input/XboxGamePad.cpp"
	"Input/XboxGamePad.h"

	"Math/Math.cpp"
	"Math/Math.h"
	"Math/Matrix.cpp"
	"Math/Matrix.h"
	"Math/Quaternion.cpp"
	"Math/Quaternion.h"
	"Math/Transform.cpp"
	"Math/Transform.h"
	"Math/Vector.cpp"
	"Math/Vector.h"

	"Network/Network.cpp"
	"Network/Network.h"

	"Physics/2D/PhysicsDebug.cpp"
	"Physics/2D/PhysicsDebug.h"
	"Physics/2D/PhysicsWorld.cpp"
	"Physics/2D/PhysicsWorld.h"
	"Physics/2D/PhysicsWorld.inl"
	"Physics/2D/RigidBody.cpp"
	"Physics/2D/RigidBody.h"
	"Physics/Physics.h"

	"Rendering/Camera.cpp"
	"Rendering/Camera.h"
	"Rendering/Light.cpp"
	"Rendering/Light.h"
	"Rendering/Mesh.cpp"
	"Rendering/Mesh.h"
	"Rendering/ParticleEmitter.cpp"
	"Rendering/ParticleEmitter.h"
	"Rendering/Primitives.cpp"
	"Rendering/Primitives.h"
	"Rendering/Renderable.cpp"
	"Rendering/Renderable.h"
	"Rendering/Rendering.cpp"
	"Rendering/Rendering.h"
	"Rendering/RenderPass.cpp"
	"Rendering/RenderPass.h"
	"Rendering/RenderTarget.cpp"
	"Rendering/RenderTarget.h"
	"Rendering/Sprite.cpp"
	"Rendering/Sprite.h"
	"Rendering/Text.cpp"
	"Rendering/Text.h"
	"Rendering/Viewport.cpp"
	"Rendering/Viewport.h"

	"Resource/ConfigTable.cpp"
	"Resource/ConfigTable.h"
	"Resource/Encoder.h"
	"Resource/Font.cpp"
	"Resource/Font.h"
	"Resource/Material.cpp"
	"Resource/Material.h"
	"Resource/Model.cpp"
	"Resource/Model.h"
	"Resource/ParticleBuffer.cpp"
	"Resource/ParticleBuffer.h"
	"Resource/ParticleFunctor.cpp"
	"Resource/ParticleFunctor.h"
	"Resource/Resource.cpp"
	"Resource/Resource.h"
	"Resource/Shader.cpp"
	"Resource/Shader.h"
	"Resource/Shareable.h"
	"Resource/Sound.cpp"
	"Resource/Sound.h"
	"Resource/Texture.cpp"
	"Resource/Texture.h"
	"Resource/UniformBuffer.cpp"
	"Resource/UniformBuffer.h"
	"Resource/UniformBuffer.inl"
	"Resource/VertexArray.cpp"
	"Resource/VertexArray.h"
	"Resource/VertexArray.inl"

	"Sound/SoundListener.cpp"
	"Sound/SoundListener.h"
	"Sound/SoundSource.cpp"
	"Sound/SoundSource.h"
	"Sound/SoundSystem.cpp"
	"Sound/SoundSystem.h"

	"Utilities/Container.h"
	"Utilities/EnumFlags.h"
	"Utilities/Meta.h"
	"Utilities/Random.cpp"
	"Utilities/Random.h"
	"Utilities/ScopeGuard.h"
	"Utilities/String.cpp"
	"Utilities/String.h"
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${gemcutter_files})

add_library(gemcutter STATIC ${gemcutter_files})
sf_target_compile_disable_exceptions(gemcutter OPTIONAL)
sf_target_compile_fast_fp(gemcutter OPTIONAL)
sf_target_compile_warnings_as_errors(gemcutter OPTIONAL)
sf_target_compile_warnings(gemcutter)

target_compile_features(gemcutter PUBLIC cxx_std_20)
target_include_directories(gemcutter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../)
target_link_libraries(gemcutter
	PRIVATE
		dirent
	PUBLIC
		box2d
		OpenAL
		OpenGL::GL
		soil
		Xinput
		glew
)

target_compile_definitions(gemcutter
	PUBLIC
		_CRT_SECURE_NO_WARNINGS
		WIN32_LEAN_AND_MEAN
		NOMINMAX
)

option(ENABLE_PCH "Use a precompiled header file." TRUE)
if (ENABLE_PCH)
	target_precompile_headers(gemcutter
		PRIVATE
			<algorithm>
			<cstring>
			<functional>
			<map>
			<memory>
			<optional>
			<string_view>
			<string>
			<type_traits>
			<unordered_map>
			<vector>
			Application/Logging.h
			Entity/Entity.h
			Math/Matrix.h
			Math/Quaternion.h
			Math/Transform.h
			Math/Vector.h
			Resource/Shareable.h
			Utilities/Meta.h
			Utilities/Random.h
	)
endif()

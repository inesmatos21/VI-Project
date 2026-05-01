cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)

project(VI-RT)

add_subdirectory(external/glm)

#find_library(OIDN_LIBRARY NAMES libOpenImageDenoise PATHS /Applications/oidn-2.4.1.arm64.macos/lib)

set(OIDN_LIBRARY "/Applications/oidn-2.4.1.arm64.macos/lib/libOpenImageDenoise.2.4.1.dylib")

message(${OIDN_LIBRARY})

add_executable(VI-RT src/main.cpp
    src/Log/Log.hpp

    src/Utils/ProgressBar.hpp
    src/Utils/ProgressBar.cpp

    # Ray
    src/Ray/Ray.hpp
    src/Ray/Intersection.hpp

    # Renderer
    src/Renderer/Renderer.hpp
    src/Renderer/DummyRenderer.hpp

    # Image
    src/Image/Image.hpp
    src/Image/FileImages.hpp
    src/Image/FileImages.cpp

    # Camera
    src/Camera/Camera.hpp
    src/Camera/Camera.cpp

    # Scene
    src/Scene/Scene.hpp
    src/Scene/Scene.cpp
    src/Scene/SceneBuilder.hpp
    src/Scene/SceneBuilder.cpp

    # Primitives
    src/Primitive/BRDF.hpp
    src/Primitive/BRDF.cpp
    src/Primitive/BoundingBox.hpp
    src/Primitive/Primitive.hpp
    src/Primitive/Material.hpp
    src/Primitive/Geometry/Geometry.hpp
    src/Primitive/Geometry/Geometry.cpp
    src/Primitive/Geometry/Sphere.hpp
    src/Primitive/Geometry/Sphere.cpp
    src/Primitive/Geometry/Triangle.hpp
    src/Primitive/Geometry/Triangle.cpp
    src/Primitive/Geometry/Mesh.hpp
    src/Primitive/Geometry/Mesh.cpp

    src/Primitive/AccelerationStructures/AccelerationStructure.hpp

    src/Primitive/AccelerationStructures/GridAccelerationStructure.hpp
    src/Primitive/AccelerationStructures/GridAccelerationStructure.cpp

    # Lights
    src/Light/Light.hpp

    # Math
    src/Math/Math.hpp
    src/Math/Math.cpp
    src/Math/DiscreteDistribution.hpp
    src/Math/DiscreteDistribution.cpp
    src/Math/RGB.hpp
    src/Math/Vector.hpp
    src/Math/Random.hpp
    src/Math/Random.cpp

    # Shaders
    src/Shaders/Shader.hpp
    src/Shaders/DummyShader.hpp
    src/Shaders/AmbientShader.hpp
    src/Shaders/AmbientShader.cpp
    src/Shaders/WhittedShader.hpp
    src/Shaders/WhittedShader.cpp
    src/Shaders/PathTracingShader.hpp
    src/Shaders/PathTracingShader.cpp
    src/Shaders/DirectIllumination.hpp
    src/Shaders/DirectIllumination.cpp
    src/Shaders/VeachShader.hpp
    src/Shaders/VeachShader.cpp
)
target_include_directories(VI-RT PRIVATE src)
target_include_directories(VI-RT PRIVATE /Applications/oidn-2.4.1.arm64.macos/include/)
target_precompile_headers(VI-RT PRIVATE src/vi_pch.h)

target_link_libraries(VI-RT PRIVATE glm::glm)
target_link_libraries(VI-RT PRIVATE ${OIDN_LIBRARY})

if (MSVC)
  target_compile_options(VI-RT PRIVATE /W4)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
  target_compile_options(VI-RT PRIVATE -fcolor-diagnostics -Wall -Wextra -Wpedantic)
else()
  target_compile_options(VI-RT PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

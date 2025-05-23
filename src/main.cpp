#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <array>
#include <vector>
#include <format>
#include <memory>
#include <cmath>
#include <iomanip>
#include <cstdint>

#define WEBGPU_CPP_IMPLEMENTATION
#include <wgpu-native/webgpu.hpp>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include <wgpu-native/webgpu/wgpu.h>

#include <sdl2webgpu.h>

#include <Window.hpp>

#include <Helper/Handles.hpp>
#include <Helper/Descriptors.hpp>
#include <Helper/ParametersStructs.hpp>

#include <Resources/Texture/Texture2D.hpp>
#include <Resources/Geometry/Geometry.hpp>

#include <Logger.hpp>
#include <Math/Math.hpp>
#include <Math/Matrix4x4.hpp>
#include <Math/Vector2.hpp>
#include <Math/Vector3.hpp>
#include <Math/Vector4.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

constexpr float const PI = 3.1415926535897932384626433832795f;

constexpr float windowWidth = 640.0f;
constexpr float windowHeight = 480.0f;

struct MyUniforms {
	Math::Matrix4x4 projectionMatrix {};
	Math::Matrix4x4 viewMatrix {};
	Math::Matrix4x4 modelMatrix {};
	Math::Vector4 color {};
	Math::Vector3 cameraPosition {};
	float time = 0.0f;
};

static_assert(sizeof(MyUniforms) % 16 == 0, "MyUniforms must be aligned to 16 bytes.");

Logger logger {};

auto DeviceLostCallback = [](wgpu::Device const* device, wgpu::DeviceLostReason reason, wgpu::StringView message, void* userData1) {
	std::cerr << "[Lost device] (" << reason << "): " << message << std::endl;
};
auto UncapturedErrorCallback = [](wgpu::Device const* device, wgpu::ErrorType type, wgpu::StringView message, void* userData1) {
	std::cerr << "[Uncaptured error] (" << type << "): " << message << std::endl;
};

void LogCallback(WGPULogLevel level, WGPUStringView message, void* userData) {
	switch (level) {
		case wgpu::LogLevel::Error:
			std::cerr << "[Error] " << message.data << std::endl;
			break;

		case wgpu::LogLevel::Warn:
			std::cerr << "[Warning] " << message.data << std::endl;
			break;

		case wgpu::LogLevel::Info:
			std::cout << "[Info] " << message.data << std::endl;
			break;

		case wgpu::LogLevel::Debug:
			std::cout << "[Debug] " << message.data << std::endl;
			break;

		default:
			std::cout << "[Unknown] " << message.data << std::endl;
			break;
	}
}


bool running = false;

static wgpu::TextureView GetNextTexture(wgpu::Device& device, wgpu::Surface& surface) {
	wgpu::SurfaceTexture surfaceTexture {};
	surface.getCurrentTexture(&surfaceTexture);

	//std::cout << "Surface texture: ";
	switch (surfaceTexture.status) {
		case wgpu::SurfaceGetCurrentTextureStatus::DeviceLost:
			std::cerr << "Device lost" << std::endl;
			return nullptr;

		case wgpu::SurfaceGetCurrentTextureStatus::Lost:
			std::cerr << "Lost" << std::endl;
			return nullptr;

		case wgpu::SurfaceGetCurrentTextureStatus::Outdated:
			std::cerr << "Outdated" << std::endl;
			return nullptr;

		case wgpu::SurfaceGetCurrentTextureStatus::OutOfMemory:
			std::cerr << "Out of memory" << std::endl;
			return nullptr;

		case wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal:
			//std::cout << "Success" << std::endl;
			break;

		case wgpu::SurfaceGetCurrentTextureStatus::Timeout:
			std::cerr << "Timeout" << std::endl;
			return nullptr;

		default:
			std::cerr << "Unknown" << std::endl;
			return nullptr;
	}

	wgpu::Texture texture = surfaceTexture.texture;

	wgpu::TextureViewDescriptor textureViewDescriptor {};
	textureViewDescriptor.label = wgpu::StringView("current_surface_texture_view");
	textureViewDescriptor.format = texture.getFormat();
	textureViewDescriptor.dimension = wgpu::TextureViewDimension::_2D;
	textureViewDescriptor.baseMipLevel = 0;
	textureViewDescriptor.mipLevelCount = 1;
	textureViewDescriptor.baseArrayLayer = 0;
	textureViewDescriptor.arrayLayerCount = 1;
	textureViewDescriptor.aspect = wgpu::TextureAspect::All;
	textureViewDescriptor.nextInChain = nullptr;
	wgpu::TextureView textureView = texture.createView(textureViewDescriptor);

	return textureView;
}

WGPULogCallback logCallbackHandle = LogCallback;

int main() {
	try {
		// MARK: Main instances
		Instance instance;
		Window window;
		CompatibleSurface surface(instance, window);
		Adapter adapter(instance, window, surface);

		Limits limits(adapter);
		limits.maxBufferSize = 150000 * sizeof(VertexAttributes);
		limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);

		DeviceLostCallbackInfo deviceLostCallbackInfo(DeviceLostCallback, nullptr, nullptr);
		UncapturedErrorCallbackInfo uncapturedErrorCallbackInfo(UncapturedErrorCallback, nullptr, nullptr);
		DeviceDescriptor deviceDescriptor(adapter, limits, deviceLostCallbackInfo, uncapturedErrorCallbackInfo);
		Device device(adapter, deviceDescriptor);
		Queue queue(device);

		const char* test = "test";
		wgpuSetLogCallback(logCallbackHandle, &test);
		
		surface.Configure(adapter, device, window);

		std::vector<VertexAttributes> vertexData {};
		std::vector<BindGroupLayout> bindGroupLayouts {};
		std::vector<BindGroup> bindGroups {};

		// MARK: Vertex buffer layout
		std::vector<VertexAttribute> vertexAttributes;
		vertexAttributes.push_back(VertexAttribute(0, wgpu::VertexFormat::Float32x3, offsetof(VertexAttributes, position)));
		vertexAttributes.push_back(VertexAttribute(1, wgpu::VertexFormat::Float32x3, offsetof(VertexAttributes, normal)));
		vertexAttributes.push_back(VertexAttribute(2, wgpu::VertexFormat::Float32x3, offsetof(VertexAttributes, color)));
		vertexAttributes.push_back(VertexAttribute(3, wgpu::VertexFormat::Float32x2, offsetof(VertexAttributes, uv)));

		std::vector<VertexBufferLayout> vertexBufferLayouts {};
		VertexBufferLayout vertexBufferLayout(sizeof(VertexAttributes), vertexAttributes);
		vertexBufferLayouts.push_back(vertexBufferLayout);

		// MARK: Cube binding layouts
		std::vector<BindGroupLayoutEntry> bindGroupLayoutEntries {};
		bindGroupLayoutEntries.push_back(BufferBindingLayout(0, wgpu::ShaderStage::Vertex | wgpu::ShaderStage::Fragment, wgpu::BufferBindingType::Uniform, sizeof(MyUniforms)));
		bindGroupLayoutEntries.push_back(TextureBindingLayout(1, wgpu::ShaderStage::Fragment, wgpu::TextureSampleType::Float));
		//bindGroupLayoutEntries[1].texture.viewDimension = wgpu::TextureViewDimension::_3D;
		bindGroupLayoutEntries.push_back(SamplerBindingLayout(2, wgpu::ShaderStage::Fragment, wgpu::SamplerBindingType::Filtering));

		BindGroupLayoutDescriptor bindGroupLayoutDescriptor(bindGroupLayoutEntries);
		bindGroupLayouts.emplace_back(device, bindGroupLayoutDescriptor);

		// MARK: Cube geometry
		bool success = LoadGeometryFromOBJ("resources/cube.obj", vertexData);
		if (!success) {
			logger.Error("Failed to load geometry.");
			return EXIT_FAILURE;
		}
		
		// MARK: Cube vertex buffer
		BufferDescriptor vertexBufferDescriptor(vertexData.size() * sizeof(VertexAttributes), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex, "vertex_buffer");
		Buffer vertexBuffer(device, vertexBufferDescriptor);

		queue->writeBuffer(vertexBuffer.Handle(), 0, vertexData.data(), vertexBufferDescriptor.size);
		int indexCount = static_cast<int>(vertexData.size());

		// MARK: Cube binding handles
		BufferDescriptor uniformBufferDescriptor(sizeof(MyUniforms), wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform, "uniform_buffer");
		Buffer uniformBuffer(device, uniformBufferDescriptor);

		TextureDescriptor textureDescriptor(
			wgpu::TextureFormat::RGBA8Unorm,
			wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
			Extent3D(windowWidth, windowHeight, 1)
		);

		textureDescriptor.size.width = 2048;
		textureDescriptor.size.height = 2048;
		textureDescriptor.size.depthOrArrayLayers = 1;

		TextureViewDescriptor textureViewDescriptor(
			wgpu::TextureAspect::All,
			wgpu::TextureFormat::RGBA8Unorm
		);

		Texture2D texture("resources/futuristic.png", device, queue, textureDescriptor, textureViewDescriptor);

		//TextureView textureView;
		//Texture texture = std::move(LoadTexture("resources/futuristic.png", device.Handle(), &textureView.Handle()));

		/*
		std::array<TextureView, 6> skyboxTextureViews {};
		std::array<Texture, 6> skyboxTextures {
			LoadTexture("resources/pos-x.jpg", device.Handle()),
			LoadTexture("resources/neg-x.jpg", device.Handle()),
			LoadTexture("resources/pos-y.jpg", device.Handle()),
			LoadTexture("resources/neg-y.jpg", device.Handle()),
			LoadTexture("resources/pos-z.jpg", device.Handle()),
			LoadTexture("resources/neg-z.jpg", device.Handle())
		};
		*/

		SamplerDescriptor samplerDescriptor(0.0f, 8.0f);
		Sampler sampler(device, samplerDescriptor);
 
		// MARK: Cube bindings array
		std::vector<BindGroupEntry> bindGroupEntries {};
		bindGroupEntries.push_back(BufferBinding(0, uniformBuffer, sizeof(MyUniforms), 0));
		bindGroupEntries.push_back(TextureBinding(1, texture.View()));
		//bindGroupEntries.push_back(TextureBinding(1, textureView));
		bindGroupEntries.push_back(SamplerBinding(2, sampler));

		BindGroupDescriptor bindGroupDescriptor(bindGroupLayouts[0], bindGroupEntries);
		bindGroups.emplace_back(device, bindGroupDescriptor);

		// MARK: Cube depth texture
		wgpu::TextureFormat depthTextureFormat = wgpu::TextureFormat::Depth24Plus;

		std::vector<wgpu::TextureFormat> viewFormats { depthTextureFormat };
		TextureDescriptor depthTextureDescriptor(depthTextureFormat, wgpu::TextureUsage::RenderAttachment, { static_cast<uint32_t>(windowWidth), static_cast<uint32_t>(windowHeight), 1 }, viewFormats);
		Texture depthTexture(device, depthTextureDescriptor);
	
		TextureViewDescriptor depthTextureViewDescriptor(wgpu::TextureAspect::DepthOnly, depthTextureFormat);
		TextureView depthTextureView(depthTexture, depthTextureViewDescriptor);

		// MARK: Cube render pipeline
		StencilFaceState stencilBackFaceState;
		StencilFaceState stencilFrontFaceState;
		DepthStencilState depthStencilState(stencilFrontFaceState, stencilBackFaceState, depthTextureFormat);

		PrimitiveState primitiveState;
		primitiveState.cullMode = wgpu::CullMode::None; // for debug purposes, put it to None

		MultisampleState multisampleState;

		//ShaderModule skyboxShaderModule(device, "resources/skybox.wgsl");
		ShaderModule skyboxShaderModule(device, "resources/cube.wgsl");

		std::vector<ConstantEntry> vertexConstantEntries {};
		//ShaderModule vertexShaderModule(device, "resources/cube.wgsl");
		VertexState vertexState(wgpu::StringView("vs"), skyboxShaderModule, vertexBufferLayouts, vertexConstantEntries);
	
		BlendComponent colorComponent(wgpu::BlendFactor::SrcAlpha, wgpu::BlendFactor::OneMinusSrcAlpha, wgpu::BlendOperation::Add);
		BlendComponent alphaComponent(wgpu::BlendFactor::Zero, wgpu::BlendFactor::One, wgpu::BlendOperation::Add);
		BlendState blendState(colorComponent, alphaComponent);
		std::vector<ColorTargetState> colorTargetStates {};
		ColorTargetState colorTargetState(adapter, surface, blendState);
		colorTargetStates.push_back(colorTargetState);
		std::vector<ConstantEntry> fragmentConstantEntries {};
		//ShaderModule fragmentShaderModule(device, "resources/cube.wgsl");
		FragmentState fragmentState(wgpu::StringView("fs"), skyboxShaderModule, colorTargetStates, fragmentConstantEntries);
	
		PipelineLayoutDescriptor pipelineLayoutDescriptor(bindGroupLayouts);
		PipelineLayout pipelineLayout(device, pipelineLayoutDescriptor);

		RenderPipelineDescriptor cubeRenderPipelineDescriptor(depthStencilState, primitiveState, multisampleState, vertexState, fragmentState, pipelineLayout);
		RenderPipeline cubeRenderPipeline(device, cubeRenderPipelineDescriptor);
	
		// MARK: Uniforms initialization
		Math::Vector3 cameraPosition(-300.0, -400.0, -300.0);
		Math::Matrix4x4 S = Math::Matrix4x4::Scale(100.0f);	
		Math::Matrix4x4 L = Math::Matrix4x4::LookAt(cameraPosition, Math::Vector3( 0.0f,  0.0f, 0.0f), Math::Vector3( 0.0f,  0.0f, 1.0f));

		float ratio = windowWidth / windowHeight;
		float vfov = 45.0f * PI / 180.0f;
		float near = 0.01f;
		float far = 1000.0f;
		Math::Matrix4x4 P = Math::Matrix4x4::Perspective(vfov, ratio, near, far);

		MyUniforms uniforms = {
			.projectionMatrix = Math::Matrix4x4::Transpose(P),
			.viewMatrix = Math::Matrix4x4::Transpose(L),
			.modelMatrix = Math::Matrix4x4::Transpose(S),
			.color = { 0.0f, 1.0f, 0.4f, 1.0f },
			.cameraPosition = cameraPosition,
			.time = 1.0f
		};

		queue->writeBuffer(uniformBuffer.Handle(), 0, &uniforms, sizeof(MyUniforms));

		Uint8 const* keyboard = SDL_GetKeyboardState(nullptr);

		running = true;
		uint64_t frameCount = 0;
		bool moveCamera = true;

		struct SpaceTimer {
			Uint64 lastSpaceTime = 0;
			Uint64 pauseTimeSum = 0;
		} spaceTimer;

		// MARK: Main loop
		SDL_Event event {};
		while (running) {
			static Uint64 currentTime = 0ULL;

			currentTime = SDL_GetTicks64();

			std::cout << std::endl << "[" << std::setw(20) << frameCount++ << "]" << std::endl;
			std::cout << "moveCamera: " << std::boolalpha << moveCamera << std::endl;

			// MARK: Events handling
			if (SDL_PollEvent(&event) > 0) {
				if (event.type == SDL_QUIT) {
					running = false;
				}

				if (keyboard[SDL_SCANCODE_Z]) {
					cameraPosition.y += 0.01f;
				}

				if (keyboard[SDL_SCANCODE_S]) {
					cameraPosition.y -= 0.01f;
				}

				if (keyboard[SDL_SCANCODE_SPACE] && (currentTime - spaceTimer.lastSpaceTime) > 100) {
					moveCamera = !moveCamera;
					
					if (moveCamera) {
						spaceTimer.pauseTimeSum += currentTime - spaceTimer.lastSpaceTime;
					}
					
					spaceTimer.lastSpaceTime = currentTime;
				}
			}

			// MARK: Update	
			TextureView textureView = std::move(GetNextTexture(device.Handle(), surface.Handle()));

			uniforms.time = -static_cast<float>(currentTime - spaceTimer.pauseTimeSum) / 1000;
			queue->writeBuffer(uniformBuffer.Handle(), offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));

			if (moveCamera) {
				cameraPosition = Math::Vector3(-300.0f * std::cos(uniforms.time), -400.0f, -300.0f * std::sin(uniforms.time));
				uniforms.cameraPosition = cameraPosition;
				queue->writeBuffer(uniformBuffer.Handle(), offsetof(MyUniforms, cameraPosition), &uniforms.cameraPosition, sizeof(MyUniforms::cameraPosition));
			}

			L = Math::Matrix4x4::LookAt(cameraPosition, Math::Vector3(0.0f, 0.0f, 0.0f), Math::Vector3(0.0f, 0.0f, 1.0f));
			uniforms.viewMatrix = Math::Matrix4x4::Transpose(L);
			queue->writeBuffer(uniformBuffer.Handle(), offsetof(MyUniforms, viewMatrix), &uniforms.viewMatrix, sizeof(MyUniforms::viewMatrix));

			// MARK: Render
			CommandEncoderDescriptor commandEncoderDescriptor;
			CommandEncoder commandEncoder(device, commandEncoderDescriptor);

			std::vector<RenderPassColorAttachment> renderPassColorAttachments {};
			renderPassColorAttachments.push_back(RenderPassColorAttachment(textureView));

			RenderPassDepthStencilAttachment renderPassDepthStencilAttachment(depthTextureView);

			QuerySet occlusionQuerySet;

			RenderPassDescriptor renderPassDescriptor(renderPassColorAttachments, renderPassDepthStencilAttachment);
			RenderPassEncoder renderPassEncoder(commandEncoder, renderPassDescriptor);

			renderPassEncoder->setPipeline(cubeRenderPipeline.Handle());
			renderPassEncoder->setVertexBuffer(0, vertexBuffer.Handle(), 0, vertexData.size() * sizeof(VertexAttributes));
			renderPassEncoder->setBindGroup(0, bindGroups[0].Handle(), 0, nullptr);
			renderPassEncoder->draw(indexCount, 1, 0, 0);

			renderPassEncoder->end();

			CommandBufferDescriptor commandBufferDescriptor;
			CommandBuffer commandBuffer(commandEncoder);
		
			queue->submit(commandBuffer.Handle());

			surface->present();
		}
	}

	catch (const std::exception& e) {
		std::cerr << "Exception: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	logger.Info("Successfully exited.");
	return EXIT_SUCCESS;
}

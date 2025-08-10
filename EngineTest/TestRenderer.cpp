#include "TestRenderer.h"
#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"
#include "Components/Geometry.h"
#include "Input/Input.h"
#include "Graphics/Renderer.h"
#include "../EngineDLL/ShaderCompilation.h"
#include "../EngineDLL/ShaderCompilation.cpp"
#include "Content/ContentToEngine.h"

#include <filesystem>
#include <fstream>

#if TEST_RENDERER

using namespace mooncastle;

//////////////// Multithreading test worker span code ////////////////
#define ENABLE_TEST_WORKERS 0

constexpr u32	threadCount{ 8 };
bool			end{ false };
std::thread		workers[threadCount];

utl::vector<u8> emptyBuffer(1024 * 1024, 0);

//Test worker for the upload context.
void bufferTestWorker()
{
	while (!end)
	{
		//auto* resource = graphics::d3D12::d3DX::createBuffer((u32)emptyBuffer.size(), emptyBuffer.data());

		//We can also use core::release(resource) since we're not using the buffer for rendering.
		//graphics::d3D12::core::deferredRelease(resource);
	}
}

template<class FnPtr, class... Args>
void initTestWorkers([[maybe_unused]] FnPtr&& fnPtr, [[maybe_unused]] Args&&... args)
{
#if ENABLE_TEST_WORKERS
	end = false;

	for (auto& w : workers)
	{
		w = std::thread(std::forward<FnPtr>(fnPtr), std::forward<Args>(args)...);
	}
#endif
}

void jointTestWorkers()
{
#if ENABLE_TEST_WORKERS
	end = true;
	for (auto& w : workers) w.join();
#endif
}
//////////////////////////////////////////////////////////////////////

struct cameraSurface
{
	gameEntity::entity entity{};
	graphics::camera camera{};
	graphics::renderSurface surface{};
};

cameraSurface surfaces[4];

timeIt timer{};

bool resized{ false };
bool isRestarting{ false };

utl::vector<id::idType> renderItemIDCache;

//---------Forward declarations---------//
void removeCameraSurface(cameraSurface& surface);
bool testInitialize();
void testShutdown();

void createRenderItems();
void destroyRenderItems();

void generateLights();
void removeLights();

void testLights(f32 dt);
//--------------------------------------//

LRESULT winProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	bool toggleFullscreen{ false };

	switch (msg)
	{
	case WM_DESTROY:
	{
		bool allClosed{ true };
		
		for (u32 i{ 0 }; i < _countof(surfaces); ++i)
		{
			if (surfaces[i].surface.window.isValid()) 
			{
				if (surfaces[i].surface.window.isClosed())
				{
					removeCameraSurface(surfaces[i]);
				}
				else
				{
					allClosed = false;
				}
			}
		}
		if (allClosed && !isRestarting)
		{
			PostQuitMessage(0);
			return 0;
		}
	}
	break;
	case WM_SIZE:
		resized = (wparam != SIZE_MINIMIZED);
		break;
	case WM_SYSCHAR:
		toggleFullscreen = (wparam == VK_RETURN && (HIWORD(lparam) & KF_ALTDOWN));
		break;
	case WM_KEYDOWN:
		if (wparam == VK_ESCAPE)
		{
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		else if (wparam == VK_F11)
		{
			isRestarting = true;
			testShutdown();
			testInitialize();
		}

	}

	if ((resized && GetKeyState(VK_LBUTTON) >= 0) || toggleFullscreen)
	{
		platform::window win{ platform::windowId{(id::idType)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };

		for (u32 i{ 0 }; i < _countof(surfaces); ++i)
		{
			if (win.getId() == surfaces[i].surface.window.getId())
			{
				if (toggleFullscreen)
				{
					win.setFullScreen(!win.isFullScreen());

					/*The default window procedure will play a system notification sound
					when pressing the Alt + Enter keyboard combination if WM_SYSCHAR is
					not handled.*/
					return 0;
				}
				else
				{
					surfaces[i].surface.surface.resize(win.width(), win.height());
					surfaces[i].camera.aspectRatio((f32)win.width() / win.height());
					resized = false;
				}

				break;
			}
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//Method that creates a game entity in order to test the camera.
gameEntity::entity createOneGameEntity(math::v3 position, math::v3 rotation, geometry::initInfo* geometryInfo, const char* scriptName)
{
	transform::initInfo transformInfo{};
	DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3(&rotation)) };
	math::v4a rotQuaternion;
	DirectX::XMStoreFloat4A(&rotQuaternion, quat);
	memcpy(&transformInfo.rotation[0], &rotQuaternion.x, sizeof(transformInfo.rotation));
	memcpy(&transformInfo.position[0], &position.x, sizeof(transformInfo.position));

	script::initInfo scriptInfo{};

	if (scriptName)
	{
		scriptInfo.scriptCreator = script::detail::getScriptCreator(script::detail::string_hash()(scriptName));
		assert(scriptInfo.scriptCreator);
	}

	gameEntity::entityInfo entityInfo{};
	entityInfo.transform = &transformInfo;
	entityInfo.script = &scriptInfo;
	entityInfo.geometry = geometryInfo;
	gameEntity::entity ntt{ gameEntity::create(entityInfo) };

	assert(ntt.isValid());

	return ntt;
}

void removeGameEntity(gameEntity::entityId id)
{
	gameEntity::remove(id);
}

bool readFile(std::filesystem::path path, std::unique_ptr<u8[]>& data, u64& size)
{
	if (!std::filesystem::exists(path)) return false;

	size = std::filesystem::file_size(path);
	assert(size);
	if (!size) return false;

	data = std::make_unique<u8[]>(size);

	std::ifstream file{ path, std::ios::in | std::ios::binary };

	if (!file || !file.read((char*)data.get(), size))
	{
		file.close();
		return false;
	}
	file.close();

	return true;
}

void createCameraSurface(cameraSurface& surface, platform::windowInitInfo info)
{
	surface.surface.window = platform::createWindow(&info);
	surface.surface.surface = graphics::createSurface(surface.surface.window);
	surface.entity = createOneGameEntity({ -5.49f, 1.73f, 9.26f }, { 0.19f, 5.61f, 0.f }, nullptr, "cameraScript");
	surface.camera = graphics::createCamera(graphics::perspectiveCameraInitInfo{ surface.entity.getId() });
	surface.camera.aspectRatio((f32)surface.surface.window.width() / surface.surface.window.height());
}

void removeCameraSurface(cameraSurface& surface)
{
	cameraSurface temp{ surface };
	surface = {};

	if (temp.surface.surface.isValid())
	{
		graphics::removeSurface(temp.surface.surface.getId());
	}
	if (temp.surface.window.isValid())
	{
		platform::removeWindow(temp.surface.window.getId());
	}
	if (temp.camera.isValid())
	{
		graphics::removeCamera(temp.camera.getId());
	}
	if (temp.entity.isValid())
	{
		gameEntity::remove(temp.entity.getId());
	}
}

bool testInitialize()
{
	while (!compileShaders())
	{
		//Pop up a message box allowing the user to retry compilation.
		if (MessageBox(nullptr, L"Failed to compile engine shaders.", L"Shader Compilation Error", MB_RETRYCANCEL) != IDRETRY)
		{
			return false;
		}
	}

	if (!graphics::initialize(graphics::graphicsPlatform::direct3D12)) return false;

	platform::windowInitInfo info[]
	{
		{&winProc, nullptr, L"Render Window 1", 500, 100, 400, 800},
		{&winProc, nullptr, L"Render Window 2", 550, 150, 800, 400},
		{&winProc, nullptr, L"Render Window 3", 600, 200, 400, 400},
		{&winProc, nullptr, L"Render Window 4", 650, 250, 800, 600}
	};

	static_assert(_countof(info) == _countof(surfaces));

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		createCameraSurface(surfaces[i], info[i]);
	}

	initTestWorkers(bufferTestWorker);

	generateLights();
	createRenderItems();

	renderItemIDCache.resize(4 + 12);
	geometry::getRenderItemIDs(renderItemIDCache.data(), (u32)renderItemIDCache.size());

	input::inputSource source{};
	source.binding = std::hash<std::string>()("move");
	source.sourceType = input::inputSource::keyboard;
	source.code = input::inputCode::keyA;
	source.multiplier = 1.f;
	source.axis = input::axis::x;
	input::bind(source);

	source.code = input::inputCode::keyD;
	source.multiplier = -1.f;
	input::bind(source);

	source.code = input::inputCode::keyW;
	source.multiplier = 1.f;
	source.axis = input::axis::z;
	input::bind(source);

	source.code = input::inputCode::keyS;
	source.multiplier = -1.f;
	input::bind(source);

	source.code = input::inputCode::keyQ;
	source.multiplier = -1.f;
	source.axis = input::axis::y;
	input::bind(source);

	source.code = input::inputCode::keyE;
	source.multiplier = 1.f;
	input::bind(source);

	isRestarting = false;

	return true;
}

void testShutdown()
{
	input::unbind(std::hash<std::string>()("move"));
	
	destroyRenderItems();
	removeLights();
	jointTestWorkers();

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		removeCameraSurface(surfaces[i]);
	}

	graphics::shutdown();
}

bool engineTest::initialize()
{
	return testInitialize();
}

void engineTest::run()
{
	static u32 counter{ 0 };
	static u32 lightSetKey{ 0 };
	++counter;
	//if ((counter % 90) == 0) lightSetKey = (lightSetKey + 1) % 2;

	timer.begin();
	std::this_thread::sleep_for(std::chrono::milliseconds(10));
	const f32 dt{ timer.dtAverage() };
	script::update(dt);
	//testLights(dt);

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		if (surfaces[i].surface.surface.isValid())
		{
			f32 thresholds[4 + 12]{};
			graphics::frameInfo info{};
			info.renderItemIDs = renderItemIDCache.data() + 1;
			info.renderItemCount = 4 + 12 - 1;
			info.thresholds = &thresholds[0];
			info.lightSetKey = lightSetKey;
			info.averageFrameTime = dt;
			info.cameraID = surfaces[i].camera.getId();

			assert(_countof(thresholds) >= info.renderItemCount);

			surfaces[i].surface.surface.render(info);
		}
	}

	timer.end();
}

void engineTest::shutdown()
{
	return testShutdown();
}

#endif
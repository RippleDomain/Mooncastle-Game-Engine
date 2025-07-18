#include "TestRenderer.h"
#include "ShaderCompilation.h"
#include "Platform/PlatformTypes.h"
#include "Platform/Platform.h"
#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Graphics/Renderer.h"
#include "Graphics/DirectX12/D3D12Core.h"
#include "Content/ContentToEngine.h"

#include <filesystem>
#include <fstream>

#if TEST_RENDERER

using namespace mooncastle;

//// Multithreading test worker span code /////////////////////////////////////
#define ENABLE_TEST_WORKERS 0

constexpr u32	threadCount{ 8 };
bool			end{ false };
std::thread		workers[threadCount];

utl::vector<u8> buffer(1024 * 1024, 0);

//Test worker for the upload context.
void bufferTestWorker()
{
	while (!end)
	{
		auto* resource = graphics::d3D12::d3DX::createBuffer((u32)buffer.size(), buffer.data());

		//We can also use core::release(resource) since we're not using the buffer for rendering.
		graphics::d3D12::core::deferredRelease(resource);
	}
}

template<class FnPtr, class... Args>
void initTestWorkers(FnPtr&& fnPtr, Args&&... args)
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
///////////////////////////////////////////////////////////////////////////////

gameEntity::entity entity{};
id::idType modelID{ id::invalidId };
graphics::camera camera{};
graphics::renderSurface surfaces[4];

timeIt timer{};

bool resized{ false };
bool isRestarting{ false };

void removeRenderSurface(graphics::renderSurface& surface);
bool testInitialize();
void testShutdown();

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
			if (surfaces[i].window.isValid()) 
			{
				if (surfaces[i].window.isClosed())
				{
					removeRenderSurface(surfaces[i]);
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

	if ((resized && GetAsyncKeyState(VK_LBUTTON) >= 0) || toggleFullscreen)
	{
		platform::window win{ platform::windowId{(id::idType)GetWindowLongPtr(hwnd, GWLP_USERDATA)} };

		for (u32 i{ 0 }; i < _countof(surfaces); ++i)
		{
			if (win.getId() == surfaces[i].window.getId())
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
					surfaces[i].surface.resize(win.width(), win.height());
					resized = false;
				}

				break;
			}
		}
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

//Method that creates a game entity in order to test the camera.
gameEntity::entity createOneGameEntity()
{
	transform::initInfo transformInfo{};
	math::v3a rot{ 0, 3.14f, 0 };
	DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
	math::v4a rotQuaternion;
	DirectX::XMStoreFloat4A(&rotQuaternion, quat);
	memcpy(&transformInfo.rotation[0], &rotQuaternion.x, sizeof(transformInfo.rotation));

	gameEntity::entityInfo entityInfo{};
	entityInfo.transform = &transformInfo;
	gameEntity::entity ntt{ gameEntity::create(entityInfo) };

	assert(ntt.isValid());

	return ntt;
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

void createRenderSurface(graphics::renderSurface& surface, platform::windowInitInfo info)
{
	surface.window = platform::createWindow(&info);
	surface.surface = graphics::createSurface(surface.window);
}

void removeRenderSurface(graphics::renderSurface& surface)
{
	graphics::renderSurface temp{ surface };
	surface = {};

	if (temp.surface.isValid())
	{
		graphics::removeSurface(temp.surface.getId());
	}

	if (temp.window.isValid())
	{
		platform::removeWindow(temp.window.getId());
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
		{&winProc, nullptr, L"Render Window 4", 650, 250, 800, 600},
	};

	static_assert(_countof(info) == _countof(surfaces));

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		createRenderSurface(surfaces[i], info[i]);
	}

	//Loads the test model.
	std::unique_ptr<u8[]> model;
	u64 size{ 0 };
	if (!readFile("..\\..\\EngineTest\\model.model", model, size)) return false;
	modelID = content::createResource(model.get(), content::assetType::mesh);
	if (!id::isValid(modelID)) return false;

	initTestWorkers(bufferTestWorker);

	entity = createOneGameEntity();
	camera = graphics::createCamera(graphics::perspectiveCameraInitInfo(entity.getId()));
	assert(camera.isValid());

	isRestarting = false;

	return true;
}

void testShutdown()
{
	if (camera.isValid()) graphics::removeCamera(camera.getId());
	if (entity.isValid()) gameEntity::remove(entity.getId());

	jointTestWorkers();

	if (id::isValid(modelID))
	{
		content::destroyResource(modelID, content::assetType::mesh);
	}

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		removeRenderSurface(surfaces[i]);
	}

	graphics::shutdown();
}

bool engineTest::initialize()
{
	return testInitialize();
}

void engineTest::run()
{
	timer.begin();

	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	for (u32 i{ 0 }; i < _countof(surfaces); ++i)
	{
		if (surfaces[i].surface.isValid())
		{
			surfaces[i].surface.render();
		}
	}

	timer.end();
}

void engineTest::shutdown()
{
	return testShutdown();
}

#endif
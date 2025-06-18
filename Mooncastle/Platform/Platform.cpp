#include "Platform.h"
#include "PlatformTypes.h"

#ifdef _WIN64

namespace mooncastle::platform
{
	namespace
	{
		struct windowInfo
		{
			HWND      hwnd{ nullptr };
			RECT      clientArea{ 0, 0, 1920, 1080 };
			RECT      fullScreenArea{};
			POINT     topLeft{ 0, 0 };
			DWORD     style{ WS_VISIBLE };
			bool      isFullScreen{ false };
			bool      isClosed{ false };
		};

		utl::vector<windowInfo> windows;
		/////////////////////////////////////////////////////////////////
		//TODO: this part will be handled by a free-list container later
		utl::vector<u32> availableSlots;

		u32 addToWindows(windowInfo info)
		{
			u32 id{ u32_invalid_id };

			if (availableSlots.empty())
			{
				id = (u32)windows.size();
				windows.emplace_back(info);
			}
			else
			{
				id = availableSlots.back();
				availableSlots.pop_back();
				assert(id != u32_invalid_id);
				windows[id] = info;
			}
			return id;
		}

		void removeFromWindows(u32 id)
		{
			assert(id < windows.size());
			availableSlots.emplace_back(id);
		}
		/////////////////////////////////////////////////////////////////

		windowInfo& getWindowInfoFromId(windowId id)
		{
			assert(id < windows.size());
			assert(windows[id].hwnd);
			return windows[id];
		}

		windowInfo& getWindowInfoFromHandle(windowHandle handle)
		{
			const windowId id{ (id::idType)GetWindowLongPtr(handle, GWLP_USERDATA) };
			return getWindowInfoFromId(id);
		}

		LRESULT CALLBACK internalWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
		{
			windowInfo* info{ nullptr };
			
			switch (msg) 
			{
			case WM_DESTROY:
				getWindowInfoFromHandle(hwnd).isClosed = true;
				break;
			case WM_EXITSIZEMOVE:
				info = &getWindowInfoFromHandle(hwnd);
				break;
			case WM_SIZE:
				if (wparam == SIZE_MAXIMIZED) 
				{
					info = &getWindowInfoFromHandle(hwnd);
				}
				break;
			case WM_SYSCOMMAND:
				if (wparam == SC_RESTORE)
				{
					info = &getWindowInfoFromHandle(hwnd);
				}
				break;
			default:
				break;
			}

			if (info)
			{
				assert(info->hwnd);
				GetClientRect(info->hwnd, info->isFullScreen ? &info->fullScreenArea : &info->clientArea);
			}

			LONG_PTR longPtr{ GetWindowLongPtr(hwnd, 0) };
			return longPtr ? ((windowProc)longPtr)(hwnd, msg, wparam, lparam) : DefWindowProc(hwnd, msg, wparam, lparam);
		}

		void resizeWindow(const windowInfo& info, const RECT& area)
		{
			//Adjust the window size for correct device size
			RECT window_rect{ area };
			AdjustWindowRect(&window_rect, info.style, FALSE);

			const i32 width{ window_rect.right - window_rect.left };
			const i32 height{ window_rect.bottom - window_rect.top };

			MoveWindow(info.hwnd, info.topLeft.x, info.topLeft.y, width, height, true);
		}

		void setWindowFullScreen(windowId id, bool isFullScreen)
		{
			windowInfo& info{ getWindowInfoFromId(id) };
			if (info.isFullScreen != isFullScreen)
			{
				info.isFullScreen = isFullScreen;

				if (isFullScreen)
				{
					//Store the current window dimensions so they can be restored
					GetClientRect(info.hwnd, &info.clientArea);
					RECT rect;
					GetWindowRect(info.hwnd, &rect);
					info.topLeft.x = rect.left;
					info.topLeft.y = rect.top;
					info.style = 0;
					SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
					ShowWindow(info.hwnd, SW_MAXIMIZE);
				}
				else
				{
					info.style = WS_VISIBLE | WS_OVERLAPPEDWINDOW;
					SetWindowLongPtr(info.hwnd, GWL_STYLE, info.style);
					resizeWindow(info, info.clientArea);
					ShowWindow(info.hwnd, SW_SHOWNORMAL);
				}
			}
		}

		bool isWindowFullScreen(windowId id)
		{
			return getWindowInfoFromId(id).isFullScreen;
		}

		windowHandle getWindowHandle(windowId id)
		{
			return getWindowInfoFromId(id).hwnd;
		}

		void setWindowCaption(windowId id, const wchar_t* caption)
		{
			windowInfo& info{ getWindowInfoFromId(id) };
			SetWindowText(info.hwnd, caption);
		}

		math::u32v4 getWindowSize(windowId id)
		{
			windowInfo& info{ getWindowInfoFromId(id) };
			RECT area{ info.isFullScreen ? info.fullScreenArea : info.clientArea };
			return { (u32)area.left, (u32)area.top , (u32)area.right , (u32)area.bottom };
		}

		void resizeWindow(windowId id, u32 width, u32 height)
		{
			windowInfo& info{ getWindowInfoFromId(id) };

			//We also resize while in fullscreen mode to support the case when the user changes the screen resolution.
			RECT& area{ info.isFullScreen ? info.fullScreenArea : info.clientArea };
			area.bottom = area.top + height;
			area.right = area.left + width;

			resizeWindow(info, area);
		}

		bool isWindowClosed(windowId id)
		{
			return getWindowInfoFromId(id).isClosed;
		}
	}

	window createWindow(const windowInitInfo* const initInfo)
	{
		windowProc callback{ initInfo ? initInfo->callback : nullptr };
		windowHandle parent{ initInfo ? initInfo->parent : nullptr };

		//Setup a window class
		WNDCLASSEX windowClass;
		ZeroMemory(&windowClass, sizeof(windowClass));
		windowClass.cbSize = sizeof(WNDCLASSEX);
		windowClass.style = CS_HREDRAW | CS_VREDRAW;
		windowClass.lpfnWndProc = internalWindowProc;
		windowClass.cbClsExtra = 0;
		windowClass.cbWndExtra = callback ? sizeof(callback) : 0;
		windowClass.hInstance = 0;
		windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
		windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		windowClass.hbrBackground = CreateSolidBrush(RGB(26, 48, 76));
		windowClass.lpszMenuName = NULL;
		windowClass.lpszClassName = L"MooncastleWindow";
		windowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

		//Register the window class
		RegisterClassEx(&windowClass);

		windowInfo info{};

		info.clientArea.right = (initInfo && initInfo->width) ? info.clientArea.left + initInfo->width : info.clientArea.right;
		info.clientArea.bottom = (initInfo && initInfo->height) ? info.clientArea.top + initInfo->height : info.clientArea.bottom;

		RECT rect{ info.clientArea };

		//Adjust the window sized for the correct device size
		AdjustWindowRect(&rect, info.style, FALSE);

		const wchar_t* caption{ (initInfo && initInfo->caption) ? initInfo->caption : L"Mooncastle Game" };
		const i32 left{ initInfo ? initInfo->left : info.topLeft.x };
		const i32 top{ initInfo ? initInfo->top : info.topLeft.y };
		const i32 width{ rect.right - rect.left };
		const i32 height{ rect.bottom - rect.top };

		info.style |= parent ? WS_CHILD : WS_OVERLAPPEDWINDOW;

		//Create an instance of the window class
		info.hwnd = CreateWindowEx(
			0,                           //Extended style
			windowClass.lpszClassName,   //Window class name
			caption,                     //Instance title
			info.style,                  //Window style
			left, top,                   //Initial window position
			width, height,               //Initial window dimensions
			parent,                      //Handle to parent window
			NULL,                        //Handle to menu
			NULL,                        //Instance of this application
			NULL);                       //Extra creation parameters

		if (info.hwnd) 
		{
			DEBUG_OP(SetLastError(0));
			const windowId id{ addToWindows(info) };
			SetWindowLongPtr(info.hwnd, GWLP_USERDATA, (LONG_PTR)id);

			//Set (in the extra bytes) the pointer to the window callback function that handles messages.
			if (callback) 
			{
				SetWindowLongPtr(info.hwnd, 0, (LONG_PTR)callback);
			}
			assert(GetLastError() == 0);

			ShowWindow(info.hwnd, SW_SHOWNORMAL);
			UpdateWindow(info.hwnd);
			return window{ id };
		}
		return { };
	}

	void removeWindow(windowId id) 
	{
		windowInfo& info{ getWindowInfoFromId(id) };
		DestroyWindow(info.hwnd);
		removeFromWindows(id);
	}

#else
#error "Must implement at least one platform."
#endif

	void window::setFullScreen(bool isFullScreen) const
	{
		assert(isValid());
		setWindowFullScreen(id, isFullScreen);
	}

	bool window::isFullScreen() const 
	{
		assert(isValid());
		return isWindowFullScreen(id);
	}

	void* window::handle() const 
	{
		assert(isValid());
		return getWindowHandle(id);
	}

	void window::setCaption(const wchar_t* caption) const 
	{
		assert(isValid());
		setWindowCaption(id, caption);
	}

	math::u32v4 window::size() const 
	{
		assert(isValid());
		return getWindowSize(id);
	}

	void window::resize(u32 width, u32 height) const 
	{
		assert(isValid());
		resizeWindow(id, width, height);
	}

	u32 window::width() const 
	{
		math::u32v4 s{ size() };
		return s.z - s.x;
	}

	u32 window::height() const 
	{
		math::u32v4 s{ size() };
		return s.w - s.y;
	}

	bool window::isClosed() const 
	{
		assert(isValid());
		return isWindowClosed(id);
	}
}
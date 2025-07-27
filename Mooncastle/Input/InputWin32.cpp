#ifdef _WIN64

#include "InputWin32.h"
#include "Input.h"

namespace mooncastle::input
{
	namespace
	{
		//TODO: Add all keys. 
		constexpr u32 vkMapping[256]
		{
			/* 0x00 */ u32_invalid_id,
			/* 0x01 */ inputCode::mouseLeft,
			/* 0x02 */ inputCode::mouseRight,
			/* 0x03 */ u32_invalid_id,
			/* 0x04 */ inputCode::mouseMiddle,
			/* 0x05 */ u32_invalid_id,
			/* 0x06 */ u32_invalid_id,
			/* 0x07 */ u32_invalid_id,
			/* 0x08 */ inputCode::keyBackspace,
			/* 0x09 */ inputCode::keyTab,
			/* 0x0A */ u32_invalid_id,
			/* 0x0B */ u32_invalid_id,
			/* 0x0C */ u32_invalid_id,
			/* 0x0D */ inputCode::keyReturn,
			/* 0x0E */ u32_invalid_id,
			/* 0x0F */ u32_invalid_id,

			/* 0x10 */ inputCode::keyShift,
			/* 0x11 */ inputCode::keyControl,
			/* 0x12 */ inputCode::keyAlt,
			/* 0x13 */ inputCode::keyPause,
			/* 0x14 */ inputCode::keyCapsLock,
			/* 0x15 */ u32_invalid_id,
			/* 0x16 */ u32_invalid_id,
			/* 0x17 */ u32_invalid_id,
			/* 0x18 */ u32_invalid_id,
			/* 0x19 */ u32_invalid_id,
			/* 0x1A */ u32_invalid_id,
			/* 0x1B */ inputCode::keyEscape,
			/* 0x1C */ u32_invalid_id,
			/* 0x1D */ u32_invalid_id,
			/* 0x1E */ u32_invalid_id,
			/* 0x1F */ u32_invalid_id,

			/* 0x20 */ inputCode::keySpace,
			/* 0x21 */ inputCode::keyPageUp,
			/* 0x22 */ inputCode::keyPageDown,
			/* 0x23 */ inputCode::keyEnd,
			/* 0x24 */ inputCode::keyHome,
			/* 0x25 */ inputCode::keyLeft,
			/* 0x26 */ inputCode::keyUp,
			/* 0x27 */ inputCode::keyRight,
			/* 0x28 */ inputCode::keyDown,
			/* 0x29 */ u32_invalid_id,
			/* 0x2A */ u32_invalid_id,
			/* 0x2B */ u32_invalid_id,
			/* 0x2C */ inputCode::keyPrintScreen,
			/* 0x2D */ inputCode::keyInsert,
			/* 0x2E */ inputCode::keyDelete,
			/* 0x2F */ u32_invalid_id,

			/* 0x30 */ inputCode::key0,
			/* 0x31 */ inputCode::key1,
			/* 0x32 */ inputCode::key2,
			/* 0x33 */ inputCode::key3,
			/* 0x34 */ inputCode::key4,
			/* 0x35 */ inputCode::key5,
			/* 0x36 */ inputCode::key6,
			/* 0x37 */ inputCode::key7,
			/* 0x38 */ inputCode::key8,
			/* 0x39 */ inputCode::key9,
			/* 0x3A */ u32_invalid_id,
			/* 0x3B */ u32_invalid_id,
			/* 0x3C */ u32_invalid_id,
			/* 0x3D */ u32_invalid_id,
			/* 0x3E */ u32_invalid_id,
			/* 0x3F */ u32_invalid_id,

			/* 0x40 */ u32_invalid_id,
			/* 0x41 */ inputCode::keyA,
			/* 0x42 */ inputCode::keyB,
			/* 0x43 */ inputCode::keyC,
			/* 0x44 */ inputCode::keyD,
			/* 0x45 */ inputCode::keyE,
			/* 0x46 */ inputCode::keyF,
			/* 0x47 */ inputCode::keyG,
			/* 0x48 */ inputCode::keyH,
			/* 0x49 */ inputCode::keyI,
			/* 0x4A */ inputCode::keyJ,
			/* 0x4B */ inputCode::keyK,
			/* 0x4C */ inputCode::keyL,
			/* 0x4D */ inputCode::keyM,
			/* 0x4E */ inputCode::keyN,
			/* 0x4F */ inputCode::keyO,

			/* 0x50 */ inputCode::keyP,
			/* 0x51 */ inputCode::keyQ,
			/* 0x52 */ inputCode::keyR,
			/* 0x53 */ inputCode::keyS,
			/* 0x54 */ inputCode::keyT,
			/* 0x55 */ inputCode::keyU,
			/* 0x56 */ inputCode::keyV,
			/* 0x57 */ inputCode::keyW,
			/* 0x58 */ inputCode::keyX,
			/* 0x59 */ inputCode::keyY,
			/* 0x5A */ inputCode::keyZ,
			/* 0x5B */ u32_invalid_id,
			/* 0x5C */ u32_invalid_id,
			/* 0x5D */ u32_invalid_id,
			/* 0x5E */ u32_invalid_id,
			/* 0x5F */ u32_invalid_id,

			/* 0x60 */ inputCode::keyNumpad0,
			/* 0x61 */ inputCode::keyNumpad1,
			/* 0x62 */ inputCode::keyNumpad2,
			/* 0x63 */ inputCode::keyNumpad3,
			/* 0x64 */ inputCode::keyNumpad4,
			/* 0x65 */ inputCode::keyNumpad5,
			/* 0x66 */ inputCode::keyNumpad6,
			/* 0x67 */ inputCode::keyNumpad7,
			/* 0x68 */ inputCode::keyNumpad8,
			/* 0x69 */ inputCode::keyNumpad9,
			/* 0x6A */ inputCode::keyMultiply,
			/* 0x6B */ inputCode::keyAdd,
			/* 0x6C */ u32_invalid_id,
			/* 0x6D */ inputCode::keySubtract,
			/* 0x6E */ inputCode::keyDecimal,
			/* 0x6F */ inputCode::keyDivide,

			/* 0x70 */ inputCode::keyF1,
			/* 0x71 */ inputCode::keyF2,
			/* 0x72 */ inputCode::keyF3,
			/* 0x73 */ inputCode::keyF4,
			/* 0x74 */ inputCode::keyF5,
			/* 0x75 */ inputCode::keyF6,
			/* 0x76 */ inputCode::keyF7,
			/* 0x77 */ inputCode::keyF8,
			/* 0x78 */ inputCode::keyF9,
			/* 0x79 */ inputCode::keyF10,
			/* 0x7A */ inputCode::keyF11,
			/* 0x7B */ inputCode::keyF12,
			/* 0x7C */ u32_invalid_id,
			/* 0x7D */ u32_invalid_id,
			/* 0x7E */ u32_invalid_id,
			/* 0x7F */ u32_invalid_id,
		};

		struct modifierFlags
		{
			enum Flags : u8
			{
				leftShift = 0x10,
				leftControl = 0x20,
				leftAlt = 0x40,

				rightShift = 0x01,
				rightControl = 0x02,
				rightAlt = 0x03,
			};
		};

		u8 modifierKeysState{ 0 };

		void setModifierInput(u8 virtualKey, inputCode::code code, modifierFlags::Flags flags)
		{
			if (GetKeyState(virtualKey) < 0)
			{
				set(inputSource::keyboard, code, { 1.f, 0.f, 0.f });
				modifierKeysState |= flags;
			}
			else if (modifierKeysState & flags)
			{
				set(inputSource::keyboard, code, { 0.f, 0.f, 0.f });
				modifierKeysState &= ~flags;
			}
		}

		void setModifierInputs(inputCode::code code)
		{
			if (code == inputCode::keyShift)
			{
				setModifierInput(VK_LSHIFT, inputCode::keyLeftShift, modifierFlags::leftShift);
				setModifierInput(VK_RSHIFT, inputCode::keyRightShift, modifierFlags::rightShift);
			}
			else if (code == inputCode::keyControl)
			{
				setModifierInput(VK_LCONTROL, inputCode::keyLeftControl, modifierFlags::leftControl);
				setModifierInput(VK_RCONTROL, inputCode::keyRightControl, modifierFlags::rightControl);
			}
			else if (code == inputCode::keyAlt)
			{
				setModifierInput(VK_LMENU, inputCode::keyLeftAlt, modifierFlags::leftAlt);
				setModifierInput(VK_RMENU, inputCode::keyRightAlt, modifierFlags::rightAlt);
			}
		}

		constexpr math::v2 getMousePosition(LPARAM lParam)
		{
			return { (f32)((i16)(lParam & 0x0000ffff)), (f32)((i16)(lParam >> 16)) };
		}
	}

	HRESULT processInputMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			assert(wParam <= 0xff);
			const inputCode::code code{ vkMapping[wParam & 0xff] };
			if (code != u32_invalid_id)
			{
				set(inputSource::keyboard, code, { 1.f, 0.f, 0.f });
				setModifierInputs(code);
			}
		}
		break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			assert(wParam <= 0xff);
			const inputCode::code code{ vkMapping[wParam & 0xff] };
			if (code != u32_invalid_id)
			{
				set(inputSource::keyboard, code, { 0.f, 0.f, 0.f });
				setModifierInputs(code);
			}
		}
		break;

		case WM_MOUSEMOVE:
		{
			const math::v2 pos{ getMousePosition(lParam) };
			set(inputSource::mouse, inputCode::mousePositionX, { pos.x, 0.f, 0.f });
			set(inputSource::mouse, inputCode::mousePositionY, { pos.y, 0.f, 0.f });
			set(inputSource::mouse, inputCode::mousePosition, { pos.x, pos.y, 0.f });
		}
		break;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		{
			SetCapture(hWnd);
			const inputCode::code code{ msg == WM_LBUTTONDOWN ? inputCode::mouseLeft : msg == WM_RBUTTONDOWN ? inputCode::mouseRight : inputCode::mouseMiddle };
			const math::v2 pos{ getMousePosition(lParam) };
			set(inputSource::mouse, code, { pos.x, pos.y, 1.f });
		}
		break;

		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		{
			ReleaseCapture();
			const inputCode::code code{ msg == WM_LBUTTONUP ? inputCode::mouseLeft : msg == WM_RBUTTONUP ? inputCode::mouseRight : inputCode::mouseMiddle };
			const math::v2 pos{ getMousePosition(lParam) };
			set(inputSource::mouse, code, { pos.x, pos.y, 0.f });
		}
		break;

		case WM_MOUSEHWHEEL:
		{
			set(inputSource::mouse, inputCode::mouseWheel, { (f32)(GET_WHEEL_DELTA_WPARAM(wParam)), 0.f, 0.f });
		}
		break;
		}

		return S_OK;
	}
}

#endif
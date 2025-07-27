#pragma once

#include "CommonHeaders.h"

namespace mooncastle::input
{
	struct axis
	{
		enum type : u32
		{
			x = 0,
			y = 1,
			z = 2,
		};
	};

	struct  modifierKey
	{
		enum key : u32
		{
			none = 0x00,
			leftShift = 0x01,
			rightShift = 0x02,
			shift = leftShift | rightShift,
			leftCtrl = 0x04,
			rightCtrl = 0x08,
			ctrl = leftCtrl | rightCtrl,
			leftAlt = 0x010,
			rightAlt = 0x020,
			alt = leftAlt | rightAlt,
		};
	};

	struct inputValue
	{
		math::v3 previous;
		math::v3 current;
	};

	struct inputCode
	{
		enum code : u32
		{
			mousePosition,
			mousePositionX,
			mousePositionY,
			mouseLeft,
			mouseRight,
			mouseMiddle,
			mouseWheel,

			keyBackspace,
			keyTab,
			keyReturn,
			keyShift,
			keyLeftShift,
			keyRightShift,
			keyControl,
			keyLeftControl,
			keyRightControl,
			keyAlt,
			keyLeftAlt,
			keyRightAlt,
			keyPause,
			keyCapsLock,
			keyEscape,
			keySpace,
			keyPageUp,
			keyPageDown,
			keyHome,
			keyEnd,
			keyLeft,
			keyUp,
			keyRight,
			keyDown,
			keyPrintScreen,
			keyInsert,
			keyDelete,

			key0,
			key1,
			key2,
			key3,
			key4,
			key5,
			key6,
			key7,
			key8,
			key9,

			keyA,
			keyB,
			keyC,
			keyD,
			keyE,
			keyF,
			keyG,
			keyH,
			keyI,
			keyJ,
			keyK,
			keyL,
			keyM,
			keyN,
			keyO,
			keyP,
			keyQ,
			keyR,
			keyS,
			keyT,
			keyU,
			keyV,
			keyW,
			keyX,
			keyY,
			keyZ,

			keyNumpad0,
			keyNumpad1,
			keyNumpad2,
			keyNumpad3,
			keyNumpad4,
			keyNumpad5,
			keyNumpad6,
			keyNumpad7,
			keyNumpad8,
			keyNumpad9,

			keyMultiply,
			keyAdd,
			keySubtract,
			keyDecimal,
			keyDivide,

			keyF1,
			keyF2,
			keyF3,
			keyF4,
			keyF5,
			keyF6,
			keyF7,
			keyF8,
			keyF9,
			keyF10,
			keyF11,
			keyF12,

			//TODO: Add these keys in the platform dependent part of the input implementation.
			keyNumLock,
			keyScrolLock,
			keyColon,				
			keyPlus,				
			keyComma,				
			keyMinus,				
			keyPeriod,				
			keyQuestion,			
			keyBracketOpen,		
			keyPipe,				
			keyBracketClose,		
			keyQuote,
			keyTilde
		};
	};

	struct inputSource
	{
		enum type : u32
		{
			keyboard,
			mouse,
			controller,
			raw,
			count
		};

		u64					binding{ 0 };
		type				sourceType;
		u32					code{ 0 };
		float				multiplier{ 0 };
		bool				isDiscrete{ true };
		axis::type			sourceAxis{};
		axis::type			axis{};
		modifierKey::key	modifier{};
	};

	void get(inputSource::type type, inputCode::code code, inputValue& value);
	void get(u64, inputValue&);

	namespace detail
	{
		class inputSystemBase
		{
		public:
			virtual void onEvent(inputSource::type, inputCode::code, const inputValue&) = 0;
			virtual void onEvent(u64, const inputValue&) = 0;
		protected:
			inputSystemBase();
			~inputSystemBase();
		};
	}

	template<typename T>
	class inputSystem final : public detail::inputSystemBase
	{
	public:
		using inputCallBackT = void(T::*)(inputSource::type, inputCode::code, const inputValue&);
		using bindingCallBackT = void(T::*)(u64, const inputValue&);

		void addHandler(inputSource::type type, T* instance, inputCallBackT callback)
		{
			assert(instance && callback && type < inputSource::count);

			auto& collection = inputCallBacks[type];

			for (const auto& func : collection)
			{
				if (func.instance == instance && func.callBack == callback) return;
			}

			collection.emplace_back(inputCallBack{ instance, callback });
		}

		void addHandler(u64 binding, T* instance, bindingCallBackT callback)
		{
			assert(instance && callback);

			for (const auto& func : bindingCallBacks)
			{
				if (func.binding == binding && func.instance == instance && func.callBack == callback) return;
			}

			bindingCallBacks.emplace_back(bindingCallBack{ binding, instance, callback });
		}

		void onEvent(inputSource::type type, inputCode::code code, const inputValue& value) override
		{
			assert(type < inputSource::count);

			for (const auto& item : inputCallBacks[type])
			{
				(item.instance->*item.callBack)(type, code, value);
			}
		}

		void onEvent(u64 binding, const inputValue& value) override
		{
			for (const auto& item : bindingCallBacks)
			{
				if (item.binding == binding)
				{
					(item.instance->*item.callBack)(binding, value);
				}
			}
		}

	private:
		struct inputCallBack
		{
			T*					instance;
			inputCallBackT		callBack;
		};

		struct bindingCallBack
		{
			u64					binding;
			T*					instance;
			bindingCallBackT	callBack;
		};

		utl::vector<inputCallBack>		inputCallBacks[inputSource::count];
		utl::vector<bindingCallBack>	bindingCallBacks;
	};
}
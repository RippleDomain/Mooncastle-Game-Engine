#include "Input.h"

namespace mooncastle::input
{
	namespace
	{
		struct inputBinding
		{
			utl::vector<inputSource>	sources;
			inputValue					value{};
			bool						isDirty{ true };
		};

		std::unordered_map<u64, inputValue>		inputValues;
		std::unordered_map<u64, inputBinding>	inputBindings;
		std::unordered_map<u64, u64>			sourceBindingMap;
		utl::vector<detail::inputSystemBase*>	inputCallBacks;

		constexpr u64 getKey(inputSource::type type, u32 code)
		{
			return ((u64)type << 32) | (u64)code;
		}
	}

	void bind(inputSource source)
	{
		assert(source.sourceType < inputSource::count);

		const u64 key{ getKey(source.sourceType, source.code) };
		unbind(source.sourceType, (inputCode::code)source.code);
		inputBindings[source.binding].sources.emplace_back(source);
		sourceBindingMap[key] = source.binding;
	}

	void unbind(inputSource::type type, inputCode::code code)
	{
		assert(type < inputSource::count);

		const u64 key{ getKey(type, code) };

		if (!sourceBindingMap.count(key))
		{
			return;
		}

		const u64 bindingKey{ sourceBindingMap[key] };
		assert(inputBindings.count(bindingKey));
		inputBinding& binding{ inputBindings[bindingKey] };
		utl::vector<inputSource>& sources{ binding.sources };

		for (u32 i{ 0 }; i < sources.size(); ++i)
		{
			if (sources[i].sourceType == type && sources[i].code == code)
			{
				assert(sources[i].binding == sourceBindingMap[key]);

				utl::erase_unordered(sources, i);
				sourceBindingMap.erase(key);

				break;
			}
		}

		if (!sources.size())
		{
			assert(!sourceBindingMap.count(key));
			inputBindings.erase(bindingKey);
		}
	}

	void unbind(u64 binding)
	{
		if (!inputBindings.count(binding))
		{
			return;
		}

		utl::vector<inputSource>& sources{ inputBindings[binding].sources };

		for (const auto& source : sources)
		{
			assert(source.binding == binding);
			const u64 key{ getKey(source.sourceType, source.code) };
			assert(sourceBindingMap.count(key) && sourceBindingMap[key] == binding);
			sourceBindingMap.erase(key);
		}

		inputBindings.erase(binding);
	}

	void set(inputSource::type type, inputCode::code code, math::v3 value)
	{
		assert(type < inputSource::count);

		const u64 key{ getKey(type, code) };
		inputValue& input{ inputValues[key] };
		input.previous = input.current;
		input.current = value;

		if (sourceBindingMap.count(key))
		{
			const u64 bindingKey{ sourceBindingMap[key] };

			assert(inputBindings.count(bindingKey));

			inputBinding& binding{ inputBindings[bindingKey] };
			binding.isDirty = true;

			inputValue bindingValue;
			get(bindingKey, bindingValue);

			for (const auto& c : inputCallBacks)
			{
				c->onEvent(bindingKey, bindingValue);
			}
		}

		for (const auto& c : inputCallBacks)
		{
			c->onEvent(type, code, input);
		}
	}

	void get(inputSource::type type, inputCode::code code, inputValue& value)
	{
		assert(type < inputSource::count);

		const u64 key{ getKey(type, code) };
		value = inputValues[key];
	}

	void get(u64 binding, inputValue& value)
	{
		if (!inputBindings.count(binding))
		{
			return;
		}

		inputBinding& inputBinding{ inputBindings[binding] };

		if (!inputBinding.isDirty)
		{
			value = inputBinding.value;
			return;
		}

		utl::vector<inputSource>& sources{ inputBinding.sources };
		inputValue subValue{};
		inputValue result{};

		for (const auto& source : sources)
		{
			assert(source.binding == binding);
			get(source.sourceType, (inputCode::code)source.code, subValue);
			assert(source.axis <= axis::z);

			if (source.sourceType == inputSource::mouse)
			{
				const f32 current{ (&subValue.current.x)[source.sourceAxis] };
				const f32 previous{ (&subValue.previous.x)[source.sourceAxis] };
				(&result.current.x)[source.axis] += (current - previous) * source.multiplier;
			}
			else
			{
				(&result.previous.x)[source.axis] += (&subValue.previous.x)[source.sourceAxis] * source.multiplier;
				(&result.current.x)[source.axis] += (&subValue.current.x)[source.sourceAxis] * source.multiplier;
			}
		}

		inputBinding.value = result;
		inputBinding.isDirty = false;
		value = result;
	}

	detail::inputSystemBase::inputSystemBase()
	{
		inputCallBacks.emplace_back(this);
	}

	detail::inputSystemBase::~inputSystemBase()
	{
		for (u32 i{ 0 }; i < inputCallBacks.size(); ++i)
		{
			if (inputCallBacks[i] == this)
			{
				utl::erase_unordered(inputCallBacks, i);
				break;
			}
		}
	}
}
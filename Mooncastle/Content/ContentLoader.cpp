#include "ContentLoader.h"

#include "..\Components\Entity.h"
#include "..\Components\Transform.h"
#include "..\Components\Script.h"
#include "Graphics\Renderer.h"

#if !defined(SHIPPING)

#include <filesystem>
#include <Windows.h>
#include <fstream>

namespace mooncastle::content
{
    namespace
    {
        enum ComponentType
        {
            transform, 
            script,
            count
		};

		utl::vector<gameEntity::entity> entities;

		transform::initInfo transformInfo{};
		script::initInfo scriptInfo{};

        bool readTransform(const u8*& data, gameEntity::entityInfo& info)
        {
			using namespace DirectX;

            f32 rotation[3];

            assert(!info.transform);

            memcpy(&transformInfo.position[0], data, sizeof(transformInfo.position)); data += sizeof(transformInfo.position);
            memcpy(&rotation[0], data, sizeof(rotation)); data += sizeof(rotation);
            memcpy(&transformInfo.scale[0], data, sizeof(transformInfo.scale)); data += sizeof(transformInfo.scale);

            XMFLOAT3A rot{ &rotation[0] };
            XMVECTOR  quat{ XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3A(&rot)) };
            XMFLOAT4A rotQuat{};
            XMStoreFloat4A(&rotQuat, quat);
            memcpy(&transformInfo.rotation[0], &rotQuat.x, sizeof(transformInfo.rotation));

			info.transform = &transformInfo;
            return true;
		}

        bool readScript(const u8*& data, gameEntity::entityInfo& info)
        {
            assert(!info.script);
            const u32 nameLength{ *data }; data += sizeof(u32);
            if (!nameLength) return false;
            assert(nameLength < 256);

            char script_name[256];

            memcpy(&script_name[0], data, nameLength); data += nameLength;
            script_name[nameLength] = 0;
            scriptInfo.scriptCreator = script::detail::getScriptCreator(script::detail::string_hash()(script_name));
			info.script = &scriptInfo;

            return scriptInfo.scriptCreator != nullptr;
        }

        using componentReader = bool(*)(const u8*&, gameEntity::entityInfo&);

        componentReader componentReaders[] =
        {
            readTransform,
            readScript
		};
        static_assert(_countof(componentReaders) == ComponentType::count);

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
    }

    bool loadGame()
    {
        //Read the game binary.
        std::unique_ptr<u8[]> gameData{};
        u64 size{ 0 };

        if (!readFile("game.bin", gameData, size)) return false;
        assert(gameData.get());

        const u8* at{ gameData.get() };
        constexpr u32 su32{ sizeof(u32) };
        const u32 numEntities{ *at }; at += su32;
        if (!numEntities) return false;

        for (u32 entity_index{ 0 }; entity_index < numEntities; ++entity_index)
        {
            gameEntity::entityInfo info{};
            const u32 entityType{ *at }; at += su32;
            const u32 numComponents{ *at }; at += su32;
            if (!numComponents) return false;

            for (u32 component_index{ 0 }; component_index < numComponents; ++component_index)
            {
                const u32 componentType{ *at }; at += su32;
				assert(componentType < ComponentType::count);
                if (!componentReaders[componentType](at, info)) return false;
            }

			assert(info.transform);
			gameEntity::entity entity{ gameEntity::create(info) };
            
            if (!entity.isValid()) return false;
			entities.emplace_back(entity);
        }
        assert(at == gameData.get() + size);
        return true;
    }

    void unloadGame()
    {
        for (auto entity : entities)
        {
			gameEntity::remove(entity.getId());
		}
	}

    bool loadEngineShaders(std::unique_ptr<u8[]>& shaders, u64& size)
    {
        auto path = graphics::getEngineShadersPath();
        return readFile(path, shaders, size);
    }
}

#endif
#include "PrimitiveMesh.h"
#include "Geometry.h"

namespace mooncastle::tools
{
	namespace
	{
		using primitiveMeshCreator = void(*)(scene&, const primitiveInitInfo& info);

		void createPlane(scene& scene, const primitiveInitInfo& info);
		void createCube(scene& scene, const primitiveInitInfo& info);
		void createUvSphere(scene& scene, const primitiveInitInfo& info);
		void createIcoSphere(scene& scene, const primitiveInitInfo& info);
		void createCylinder(scene& scene, const primitiveInitInfo& info);
		void createCapsule(scene& scene, const primitiveInitInfo& info);

		primitiveMeshCreator creators[]
		{
			createPlane,
			createCube,
			createUvSphere,
			createIcoSphere,
			createCylinder,
			createCapsule
		};

		static_assert(_countof(creators) == primitiveMeshType::count);
	}

	EDITOR_INTERFACE void CreatePrimitiveMesh(sceneData* data, primitiveInitInfo* info) 
	{
		assert(data && info);
		assert(info->type < primitiveMeshType::count);
		scene scene{};
	}
}
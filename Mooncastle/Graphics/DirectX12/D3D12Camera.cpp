#include "D3D12Camera.h"
#include "EngineAPI/GameEntity.h"

namespace mooncastle::graphics::d3D12::camera
{
	namespace
	{
		utl::freeList<D3D12Camera> cameras;

		void setUpVector(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size)
		{
			math::v3 upVector{ *(math::v3*)data };
			assert(sizeof(upVector) == size);
			camera.setUp(upVector);
		}

		constexpr void setFOV(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size)
		{
			assert(camera.getProjectionType() == graphics::camera::perspective);

			f32 fov{ *(f32*)data };
			assert(sizeof(fov) == size);
			camera.setFOV(fov);
		}

		constexpr void setAspectRatio(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size)
		{
			assert(camera.getProjectionType() == graphics::camera::perspective);

			f32 aspectRatio{ *(f32*)data };
			assert(sizeof(aspectRatio) == size);
			camera.setAspectRatio(aspectRatio);
		}

		constexpr void setViewWidth(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size)
		{
			assert(camera.getProjectionType() == graphics::camera::orthographic);

			f32 viewWidth{ *(f32*)data };
			assert(sizeof(viewWidth) == size);
			camera.setViewWidth(viewWidth);
		}

		constexpr void setViewHeight(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size)
		{
			assert(camera.getProjectionType() == graphics::camera::orthographic);

			f32 viewHeight{ *(f32*)data };
			assert(sizeof(viewHeight) == size);
			camera.setViewHeight(viewHeight);
		}

		constexpr void setNearZ(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size)
		{
			f32 nearZ{ *(f32*)data };
			assert(sizeof(nearZ) == size);
			camera.setNearZ(nearZ);
		}

		constexpr void setFarZ(D3D12Camera& camera, const void* const data, [[maybe_unused]] u32 size)
		{
			f32 farZ{ *(f32*)data };
			assert(sizeof(farZ) == size);
			camera.setFarZ(farZ);
		}

		void getView(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.getView());
		}

		void getProjection(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.getProjection());
		}

		void getInverseProjection(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.getInverseProjection());
		}

		void getViewProjection(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.getViewProjection());
		}

		void getInverseViewProjection(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			math::m4x4* const matrix{ (math::m4x4* const)data };
			assert(sizeof(math::m4x4) == size);
			DirectX::XMStoreFloat4x4(matrix, camera.getInverseViewProjection());
		}

		void getUp(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			math::v3* const upVector{ (math::v3* const)data };
			assert(sizeof(math::v3) == size);
			DirectX::XMStoreFloat3(upVector, camera.getUp());
		}

		constexpr void getFOV(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			assert(camera.getProjectionType() == graphics::camera::perspective);

			f32* const fov{ (f32* const)data };
			assert(sizeof(f32) == size);
			*fov = camera.getFOV();
		}

		constexpr void getAspectRatio(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			assert(camera.getProjectionType() == graphics::camera::perspective);

			f32* const aspectRatio{ (f32* const)data };
			assert(sizeof(f32) == size);
			*aspectRatio = camera.getAspectRatio();
		}

		constexpr void getViewWidth(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			assert(camera.getProjectionType() == graphics::camera::orthographic);

			f32* const viewWidth{ (f32* const)data };
			assert(sizeof(f32) == size);
			*viewWidth = camera.getViewWidth();
		}

		constexpr void getViewHeight(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			assert(camera.getProjectionType() == graphics::camera::orthographic);

			f32* const viewHeight{ (f32* const)data };
			assert(sizeof(f32) == size);
			*viewHeight = camera.getViewHeight();
		}

		constexpr void getNearZ(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			f32* const nearZ{ (f32* const)data };
			assert(sizeof(f32) == size);
			*nearZ = camera.getNearZ();
		}

		constexpr void getFarZ(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			f32* const farZ{ (f32* const)data };
			assert(sizeof(f32) == size);
			*farZ = camera.getFarZ();
		}

		constexpr void getProjectionType(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			graphics::camera::type* const type{ (graphics::camera::type* const)data };
			assert(sizeof(graphics::camera::type) == size);
			*type = camera.getProjectionType();
		}

		constexpr void getEntityID(const D3D12Camera& camera, void* const data, [[maybe_unused]] u32 size)
		{
			id::idType* const entityID{ (id::idType* const)data };
			assert(sizeof(id::idType) == size);
			*entityID = camera.getEntityID();
		}

		constexpr void dummySet(D3D12Camera&, const void* const, u32)
		{

		}

		using setFunction = void(*)(D3D12Camera&, const void* const, u32);
		using getFunction = void(*)(const D3D12Camera&, void* const, u32);

		constexpr setFunction setFunctions[]
		{
			setUpVector,
			setFOV,
			setAspectRatio,
			setViewWidth,
			setViewHeight,
			setNearZ,
			setFarZ,
			dummySet,
			dummySet,
			dummySet,
			dummySet,
			dummySet,
			dummySet,
			dummySet,
		};

		static_assert(_countof(setFunctions) == cameraParameter::count);

		constexpr getFunction getFunctions[]
		{
			getUp,
			getFOV,
			getAspectRatio,
			getViewWidth,
			getViewHeight,
			getNearZ,
			getFarZ,
			getView,
			getProjection,
			getInverseProjection,
			getViewProjection,
			getInverseViewProjection,
			getProjectionType,
			getEntityID,
		};

		static_assert(_countof(getFunctions) == cameraParameter::count);
	}

	D3D12Camera::D3D12Camera(cameraInitInfo info)
		: upVector{ DirectX::XMLoadFloat3(&info.up) }, nearZ{ info.nearZ }, farZ{ info.farZ },
		fieldOfView{ info.fieldOfView }, aspectRatio{ info.aspectRatio },
		projectionType{ info.type }, entityId{ info.entityId }, isDirty{ true }
	{
		assert(id::isValid(entityId));

		update();
	}

	void D3D12Camera::update()
	{
		gameEntity::entity entity{ gameEntity::entityId{entityId} };

		using namespace DirectX;

		math::v3 pos{ entity.transform().position() };
		math::v3 dir{ entity.transform().orientation() };
		position = XMLoadFloat3(&pos);
		direction = XMLoadFloat3(&dir);
		view = XMMatrixLookToRH(position, direction, upVector);

		if (isDirty)
		{
			//nearZ and farZ are swapped because we use inverse depth buffer in D3D12 renderer.
			projection = (projectionType == graphics::camera::perspective)
				? XMMatrixPerspectiveFovRH(fieldOfView * XM_PI, aspectRatio, farZ, nearZ)
				: XMMatrixOrthographicRH(viewWidth, viewHeight, farZ, nearZ);

			inverseProjection = XMMatrixInverse(nullptr, projection);
			isDirty = false;
		}

		viewProjection = XMMatrixMultiply(view, projection);
		inverseViewProjection = XMMatrixInverse(nullptr, viewProjection);
	}

	void D3D12Camera::setUp(math::v3 up)
	{
		upVector = DirectX::XMLoadFloat3(&up);
	}

	constexpr void D3D12Camera::setFOV(f32 fov)
	{
		assert(projectionType == graphics::camera::perspective);

		fieldOfView = fov;
		isDirty = true;
	}

	constexpr void D3D12Camera::setAspectRatio(f32 ratio)
	{
		assert(projectionType == graphics::camera::perspective);

		aspectRatio = ratio;
		isDirty = true;
	}

	constexpr void D3D12Camera::setViewWidth(f32 width)
	{
		assert(width);
		assert(projectionType == graphics::camera::orthographic);

		viewWidth = width;
		isDirty = true;
	}

	constexpr void D3D12Camera::setViewHeight(f32 height)
	{
		assert(height);
		assert(projectionType == graphics::camera::orthographic);

		viewHeight = height;
		isDirty = true;
	}

	constexpr void D3D12Camera::setNearZ(f32 z)
	{
		nearZ = z;
		isDirty = true;
	}

	constexpr void D3D12Camera::setFarZ(f32 z)
	{
		farZ = z;
		isDirty = true;
	}

	graphics::camera create(cameraInitInfo info)
	{
		return graphics::camera{ cameraId{ cameras.add(info) } };
	}

	void remove(cameraId id)
	{
		assert(id::isValid(id));
		cameras.remove(id);
	}

	void setParameter(cameraId id, cameraParameter::parameter parameter, const void *const data, u32 dataSize)
	{
		assert(data && dataSize);
		assert(parameter < cameraParameter::count);

		D3D12Camera& camera{ get(id) };
		setFunctions[parameter](camera, data, dataSize);
	}

	void getParameter(cameraId id, cameraParameter::parameter parameter, void *const data, u32 dataSize)
	{
		assert(data && dataSize);
		assert(parameter < cameraParameter::count);

		D3D12Camera& camera{ get(id) };
		getFunctions[parameter](camera, data, dataSize);
	}

	D3D12Camera& get(cameraId id)
	{
		assert(id::isValid(id));
		return cameras[id];
	}
}
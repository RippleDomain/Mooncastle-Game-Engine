#pragma once

#include "D3D12CommonHeaders.h"

namespace mooncastle::graphics::d3D12::camera
{
	class D3D12Camera
	{
	public:
		explicit D3D12Camera(cameraInitInfo info);

		void update();
		void setUp(math::v3 up);
		constexpr void setFOV(f32 fov);
		constexpr void setAspectRatio(f32 aspectRatio);
		constexpr void setViewWidth(f32 width);
		constexpr void setViewHeight(f32 height);
		constexpr void setNearZ(f32 nearz);
		constexpr void setFarZ(f32 farZ);

		[[nodiscard]] constexpr DirectX::XMMATRIX getView() const { return view; }
		[[nodiscard]] constexpr DirectX::XMMATRIX getProjection() const { return projection; }
		[[nodiscard]] constexpr DirectX::XMMATRIX getInverseProjection() const { return inverseProjection; }
		[[nodiscard]] constexpr DirectX::XMMATRIX getViewProjection() const { return viewProjection; }
		[[nodiscard]] constexpr DirectX::XMMATRIX getInverseViewProjection() const { return inverseViewProjection; }
		[[nodiscard]] constexpr DirectX::XMVECTOR getPosition() const { return position; }
		[[nodiscard]] constexpr DirectX::XMVECTOR getDirection() const { return direction; }
		[[nodiscard]] constexpr DirectX::XMVECTOR getUp() const { return upVector; }
		[[nodiscard]] constexpr f32 getNearZ() const { return nearZ; }
		[[nodiscard]] constexpr f32 getFarZ() const { return farZ; }
		[[nodiscard]] constexpr f32 getFOV() const { return fieldOfView; }
		[[nodiscard]] constexpr f32 getAspectRatio() const { return aspectRatio; }
		[[nodiscard]] constexpr f32 getViewWidth() const { return viewWidth; }
		[[nodiscard]] constexpr f32 getViewHeight() const { return viewHeight; }
		[[nodiscard]] constexpr graphics::camera::type getProjectionType() const { return projectionType; }
		[[nodiscard]] constexpr id::idType getEntityID() const { return entityId; }

	private:
		DirectX::XMMATRIX		view;
		DirectX::XMMATRIX		projection;
		DirectX::XMMATRIX		inverseProjection;
		DirectX::XMMATRIX		viewProjection;
		DirectX::XMMATRIX		inverseViewProjection;
		DirectX::XMVECTOR		position{};
		DirectX::XMVECTOR		direction{};
		DirectX::XMVECTOR		upVector;
		f32						nearZ;
		f32						farZ;

		union
		{
			f32					fieldOfView;	//Only used with perspective camera.
			f32					viewWidth;	    //Only used with orthographic camera.
		};
		union
		{
			f32					aspectRatio;	//Only used with perspective camera.
			f32					viewHeight;	    //Only used with orthographic camera.
		};

		graphics::camera::type	projectionType;
		id::idType				entityId;
		bool					isDirty;
	};

	graphics::camera create(cameraInitInfo info);
	void remove(cameraId id);
	void setParameter(cameraId id, cameraParameter::parameter parameter, const void *const data, u32 dataSize);
	void getParameter(cameraId id, cameraParameter::parameter parameter, void *const data, u32 dataSize);
	[[nodiscard]] D3D12Camera& get(cameraId id);
}
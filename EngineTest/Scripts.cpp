#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"
#include "EngineAPI/Input.h"

using namespace mooncastle;

class rotatorScript;
REGISTER_SCRIPT(rotatorScript);

class rotatorScript : public script::entityScript
{
public:
	constexpr explicit rotatorScript(gameEntity::entity entity) : script::entityScript{ entity } {}

	void beginPlay() override {}

	void update(f32 dt) override
	{
		angle += 0.25f * dt * math::tau;
		if (angle > math::tau) angle -= math::tau;

		math::v3a rot{ 0.f, angle, 0.f };
		DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };

		math::v4 rotQuat{};
		DirectX::XMStoreFloat4(&rotQuat, quat);
		setRotation(rotQuat);
	}

private:
	f32 angle{ 0.f };
};

class fanScript;
REGISTER_SCRIPT(fanScript);

class fanScript : public script::entityScript
{
public:
	constexpr explicit fanScript(gameEntity::entity entity) : script::entityScript{ entity } {}

	void beginPlay() override {}

	void update(f32 dt) override
	{
		angle -= 1.f * dt * math::tau;
		if (angle > math::tau) angle += math::tau;

		math::v3a rot{ angle, 0.f, 0.f };
		DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };

		math::v4 rotQuat{};
		DirectX::XMStoreFloat4(&rotQuat, quat);
		setRotation(rotQuat);
	}

private:
	f32 angle{ 0.f };
};

class shipScript;
REGISTER_SCRIPT(shipScript);

class shipScript : public script::entityScript
{
public:
	constexpr explicit shipScript(gameEntity::entity entity) : script::entityScript{ entity } {}

	void beginPlay() override {}

	void update(f32 dt) override
	{
		angle -= 0.01f * dt * math::tau;

		if (angle > math::tau) angle += math::tau;

		f32 x{ angle * 2.f * math::tau };
		const f32 s1{ 0.05f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x)) };
		x = angle;
		const f32 s2{ 0.05f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x)) };

		math::v3a rot{ s1, 0.f, s2 };
		DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(DirectX::XMLoadFloat3A(&rot)) };
		math::v4 rotQuat{};
		DirectX::XMStoreFloat4(&rotQuat, quat);
		setRotation(rotQuat);

		math::v3 pos{ position() };
		pos.y = 1.3f + 0.2f * std::sin(x) * std::sin(std::sin(x / 1.62f) + std::sin(1.62f * x) + std::sin(3.24f * x));
		setPosition(pos);
	}

private:
	f32 angle{ 0.f };
};

class cameraScript;
REGISTER_SCRIPT(cameraScript);

class cameraScript : public script::entityScript
{
public:
	explicit cameraScript(gameEntity::entity entity) : script::entityScript{ entity } 
	{
		inputSystem.addHandler(input::inputSource::mouse, this, &cameraScript::mouseMove);

		const u64 binding{ std::hash<std::string>()("move") };
		inputSystem.addHandler(binding, this, &cameraScript::onMove);

		math::v3 startPos{ position() };
		positionToReach = pos = DirectX::XMLoadFloat3(&startPos);

		math::v3 dir{ orientation() };
		f32 theta{ DirectX::XMScalarACos(dir.y) };
		f32 phi{ std::atan2(-dir.z, dir.x) };
		math::v3 rot{ theta - math::halfPi, phi + math::halfPi, 0.f };

		sphericalCoordinatesToReach = sphericalCoordinates = DirectX::XMLoadFloat3(&rot);
	}

	void beginPlay() override {}

	void update(f32 dt) override
	{
		using namespace DirectX;
		
		if (moveMagnitude > math::epsilon)
		{
			const f32 fpsScale{ dt / 0.016667f };
			math::v4 rot{ rotation() };
			XMVECTOR d{ XMVector3Rotate(move * 0.05f * fpsScale, XMLoadFloat4(&rot)) };

			if (positionAcceleration < 1.f) positionAcceleration += (0.02f * fpsScale);

			positionToReach += (d * positionAcceleration);
			movePosition = true;
		}
		else if (movePosition)
		{
			positionAcceleration = 0.f;
		}

		if (moveRotation || movePosition) 
		{
			seekCamera(dt);
		}
	}

private:
	void onMove(u64 binding, const input::inputValue& value)
	{
		using namespace DirectX;

		move = XMLoadFloat3(&value.current);
		moveMagnitude = XMVectorGetX(XMVector3LengthSq(move));
	}

	void mouseMove(input::inputSource::type type, input::inputCode::code code, const input::inputValue& mouse_pos)
	{
		using namespace DirectX;

		if (code == input::inputCode::mousePosition)
		{
			input::inputValue value;
			input::get(input::inputSource::mouse, input::inputCode::mouseLeft, value);

			if (value.current.z == 0.f) return;

			const f32 scale{ 0.005f };
			const f32 dx{ (mouse_pos.current.x - mouse_pos.previous.x) * scale };
			const f32 dy{ (mouse_pos.current.y - mouse_pos.previous.y) * scale };

			math::v3 spherical;
			DirectX::XMStoreFloat3(&spherical, sphericalCoordinatesToReach);
			spherical.x += dy;
			spherical.y -= dx;
			spherical.x = math::clamp(spherical.x, 0.0001f - math::halfPi, math::halfPi - 0.0001f);

			sphericalCoordinatesToReach = DirectX::XMLoadFloat3(&spherical);
			moveRotation = true;
		}
	}

	void seekCamera(f32 deltaTime)
	{
		using namespace DirectX;
		XMVECTOR p{ positionToReach - pos };
		XMVECTOR r{ sphericalCoordinatesToReach - sphericalCoordinates };

		movePosition = (XMVectorGetX(XMVector3LengthSq(p)) > math::epsilon);
		moveRotation = (XMVectorGetX(XMVector3LengthSq(r)) > math::epsilon);

		const f32 scale{ 0.2f * deltaTime / 0.016667f };

		if (movePosition)
		{
			pos += (p * scale);
			math::v3 newPos;
			XMStoreFloat3(&newPos, pos);
			setPosition(newPos);
		}

		if (moveRotation)
		{
			sphericalCoordinates += (r * scale);
			math::v3 newRotation;
			XMStoreFloat3(&newRotation, sphericalCoordinates);
			newRotation.x = math::clamp(newRotation.x, 0.0001f - math::halfPi, math::halfPi - 0.0001f);
			sphericalCoordinates = DirectX::XMLoadFloat3(&newRotation);

			DirectX::XMVECTOR quat{ DirectX::XMQuaternionRotationRollPitchYawFromVector(sphericalCoordinates) };
			math::v4 rot_quat;
			DirectX::XMStoreFloat4(&rot_quat, quat);
			setRotation(rot_quat);
		}
	}

	input::inputSystem<cameraScript>	inputSystem{};
	DirectX::XMVECTOR					sphericalCoordinates;
	DirectX::XMVECTOR					pos;
	DirectX::XMVECTOR					sphericalCoordinatesToReach;
	DirectX::XMVECTOR					positionToReach;
	DirectX::XMVECTOR					move{};
	f32									moveMagnitude{ 0.f };
	f32									positionAcceleration{ 0.f };
	bool								moveRotation{ false };
	bool								movePosition{ false };
};
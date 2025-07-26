#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Script.h"

using namespace mooncastle;

class rotatorScript;
REGISTER_SCRIPT(rotatorScript);

class rotatorScript : public script::entityScript
{
public:
	constexpr explicit rotatorScript(gameEntity::entity entity) : script::entityScript{ entity } {}

	void beginPlay() override {}

	void update(float dt) override
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

	void update(float dt) override
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

	void update(float dt) override
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
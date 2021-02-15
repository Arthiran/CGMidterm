#pragma once
#include "IBehaviour.h"


class RotateObjectBehaviour : public IBehaviour
{
public:
	RotateObjectBehaviour() = default;
	~RotateObjectBehaviour() = default;

	void OnLoad(entt::handle entity) override;

	void Update(entt::handle entity) override;

	float GetSpeed();

	void SetSpeed(float _value);

private:
	float speed;
};

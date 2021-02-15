#include "RotateObjectBehaviour.h"
#include "Transform.h"
#include "Timing.h"

#include "GLFW/glfw3.h"


void RotateObjectBehaviour::OnLoad(entt::handle entity)
{
	speed = 1.0f;
}

void RotateObjectBehaviour::Update(entt::handle entity)
{
	float dt = Timing::Instance().DeltaTime;
	Transform& transform = entity.get<Transform>();

	transform.RotateLocal(0, 0, speed);
}

float RotateObjectBehaviour::GetSpeed()
{
	return speed;
}

void RotateObjectBehaviour::SetSpeed(float _value)
{
	speed = _value;
}

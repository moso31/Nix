#include "NSTest.h"
#include "NXTimer.h"
#include "NXPrimitive.h"

NSTest::NSTest() :
	m_rotValue(0.0f)
{
}

void NSTest::Update()
{
	auto timeDelta = g_timer->GetTimeDelta() / 1000000.0f;
	m_rotValue += timeDelta;

	auto pPrimitive = dynamic_pointer_cast<NXPrimitive>(m_pObject);
	pPrimitive->SetRotation(Vector3(m_rotValue, 0.0f, 0.0f));
}

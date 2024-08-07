#include "NSTest.h"
#include "NXTimer.h"
#include "NXPrimitive.h"

NSTest::NSTest() :
	m_rotValue(0.0f)
{
}

NSTest::~NSTest()
{
}

void NSTest::Update()
{
	//return;

	float speed = 0.6f;
	auto timeDelta = NXGlobalApp::Timer->GetTimeDelta() / 1000.0f;
	m_rotValue += 0.01f;// timeDelta* speed;

	auto pPrimitive = dynamic_cast<NXPrimitive*>(m_pObject);
	pPrimitive->SetRotation(Vector3(0.0f, m_rotValue, 0.0f));
}

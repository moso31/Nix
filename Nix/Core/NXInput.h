#pragma once
#include "BaseDefs/DX12.h" 
#include "BaseDefs/Math.h"
#include "NXInstance.h"

class NXInput : public NXInstance<NXInput>
{
public:
	NXInput();
	virtual ~NXInput();

	XMINT2 MousePosition();	// ��ȡ����ڵ�ǰ�ͻ��˵�λ�ã����Ͻ�0,0��
	void RestoreData();

	void UpdateMousePosInfo();
	void UpdateRawInput(LPARAM lParam);
	void UpdateViewPortInput(bool isMouseHovering, const Vector4& vpRect);

	void PrintMouseState();

	void Release();

private:
	// ��¼��ԭ������/���״̬��
	// ��ͬ��״̬��Ӧ�˲�ͬ���¼�����KeyΪ����
	//		���� + ����	= KeyDown
	//		���� + δ���� = KeyPressing
	//		���� + ����	= KeyUp
	//		���� + δ���� = �޴����¼�
	// MouseҲ��ͬ��
	bool m_keyState[256];		// �����Ƿ���
	bool m_keyActivite[256];	// �����Ƿ񼤻��ǰ֡������/����
	bool m_mouseState[256];		// ����Ƿ���
	bool m_mouseActivite[256];	// ����Ƿ񼤻��ǰ֡������/����

	Vector2 m_mouseAbsolutePos;	// �����Ļȫ������
	Vector2 m_mouseWindowPos;	// ������Win32��������

	// ��¼GUIView������
	bool m_isMouseHoverOnView;	// �������Ƿ����ӿڣ�"view"������ͣ
	Vector4 m_viewPortRect;
};


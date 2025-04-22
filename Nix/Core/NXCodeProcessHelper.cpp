#include "NXCodeProcessHelper.h"

std::string NXCodeProcessHelper::RemoveHLSLComment(const std::string& strCode)
{
	// �Ƴ�һ��strCode�е�����ע�����ݣ���ʽ������HLSL����
	// ����
	// 1. �������±���
	// 2. ���ȼ�⵽ "//"
	//		2a. ȥ����ǰ��//֮����������ݣ�result ��ͬλ���ַ� ȫ���ɿո�
	// 3. ���ȼ�⵽ "/*"
	//		3a. ��������ֱ��Ѱ�ҵ�"*/"ͣ��
	//		3b. ȥ��/*...*/֮����������ݣ�result ��ͬλ���ַ� ȫ���ɿո�

	std::string result = strCode;

	size_t i = 0;
	size_t end = strCode.length();
	while (i < end) // 1. �������±���
	{
		size_t pos2 = strCode.find("//", i);
		size_t pos3 = strCode.find("/*", i);

		if (pos2 < pos3) // 2. ���ȼ�⵽ "//"
		{
			size_t pos2a = strCode.find("\n", pos2);
			if (pos2a == std::string::npos) pos2a = end;

			// 2a. ȥ����ǰ��//֮����������ݣ�result ��ͬλ���ַ� ȫ���ɿո�
			result.replace(pos2, pos2a - pos2, pos2a - pos2, ' ');
			i = pos2a; // �������±���
		}
		else if (pos3 < pos2) // 3. ���ȼ�⵽ "/*"
		{
			// 3a. ��������ֱ��Ѱ�ҵ�"*/"ͣ��
			size_t pos3a = strCode.find("*/", pos3);
			if (pos3a == std::string::npos) pos3a = end;

			// 3b. ȥ��/*...*/֮����������ݣ�result ��ͬλ���ַ� ȫ���ɿո�
			result.replace(pos3, pos3a - pos3 + 2, pos3a - pos3 + 2, ' ');
			i = pos3a + 2; // �������±���
		}
		else 
		{
			// û��ע���ˣ�ֱ���˳�
			break;
		}
	}

	return result;
}

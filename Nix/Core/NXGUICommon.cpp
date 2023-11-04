#include "BaseDefs/DearImGui.h"

#include "NXGUICommon.h"
#include "NXConverter.h"
#include "NXPBRMaterial.h"
#include "NXResourceManager.h"
#include "NXGUIFileBrowser.h"

namespace NXGUICommon
{

void RenderSmallTextureIcon(ImTextureID ImTexID, NXGUIFileBrowser* pFileBrowser, std::function<void()> onChange, std::function<void()> onRemove, std::function<void(const std::wstring&)> onDrop)
{
	float texSize = (float)20;

	ImGuiIO& io = ImGui::GetIO();
	{
		int frame_padding = 2;									// -1 == uses default padding (style.FramePadding)
		ImVec2 size = ImVec2(texSize - frame_padding, texSize - frame_padding);                     // Size of the image we want to make visible
		ImVec2 uv0 = ImVec2(0.0f, 0.0f);
		ImVec2 uv1 = ImVec2(1.0f, 1.0f);
		ImVec4 bg_col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);         // Black background
		ImVec4 tint_col = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);       // No tint

		if (onChange && ImGui::ImageButton(ImTexID, size, uv0, uv1, frame_padding, bg_col, tint_col))
		{
			pFileBrowser->SetTitle("Material");
			pFileBrowser->SetTypeFilters({ ".png", ".jpg", ".bmp", ".dds", ".tga", ".tif", ".tiff" });
			pFileBrowser->SetPwd("D:\\NixAssets");

			pFileBrowser->Open();
			pFileBrowser->SetOnDialogOK(onChange);
		}

		if (onDrop && ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("CONTENT_EXPLORER_BUTTON_DRUGING"))
			{
				auto pDropData = (NXGUIAssetDragData*)(payload->Data);
				if (NXConvert::IsImageFileExtension(pDropData->srcPath.extension().string()))
				{
					onDrop(pDropData->srcPath.wstring());
				}
			}
			ImGui::EndDragDropTarget();
		}
		ImGui::SameLine();

        //if (onRemove)
        //{
        //    ImGui::PushID("RemoveTexButtons");
        //    {
        //        ImGui::PushID(ImTexID);
        //        if (ImGui::Button("Reset"))
        //        {
        //            onRemove();
        //        }
        //        ImGui::PopID();
        //    }
        //    ImGui::PopID();
        //}
	}
}

NXGUICBufferStyle GetGUIStyleFromString(const std::string& strTypeString)
{
	if (strTypeString == "Value")		return NXGUICBufferStyle::Value;
	else if (strTypeString == "Value2")		return NXGUICBufferStyle::Value2;
	else if (strTypeString == "Value3")		return NXGUICBufferStyle::Value3;
	else if (strTypeString == "Value4")		return NXGUICBufferStyle::Value4;
	else if (strTypeString == "Slider")		return NXGUICBufferStyle::Slider;
	else if (strTypeString == "Slider2")	return NXGUICBufferStyle::Slider2;
	else if (strTypeString == "Slider3")	return NXGUICBufferStyle::Slider3;
	else if (strTypeString == "Slider4")	return NXGUICBufferStyle::Slider4;
	else if (strTypeString == "Color3")		return NXGUICBufferStyle::Color3;
	else if (strTypeString == "Color4")		return NXGUICBufferStyle::Color4;
	else throw std::runtime_error("Invalid GUI style string: " + strTypeString);
}

NXGUICBufferStyle GetDefaultGUIStyleFromCBufferType(NXCBufferInputType eCBElemType)
{
	switch (eCBElemType)
	{
	case NXCBufferInputType::Float:
		return NXGUICBufferStyle::Value;
	case NXCBufferInputType::Float2:
		return NXGUICBufferStyle::Value2;
	case NXCBufferInputType::Float3:
		return NXGUICBufferStyle::Value3;
	case NXCBufferInputType::Float4:
	default:
		return NXGUICBufferStyle::Value4;
	}
}

Vector2 GetGUIParamsDefaultValue(NXGUICBufferStyle eGUIStyle)
{
	switch (eGUIStyle)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Value4:
	default:
		return { 0.01f, 0.0f }; // speed, ---

	case NXGUICBufferStyle::Slider:
	case NXGUICBufferStyle::Slider2:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Slider4:
	case NXGUICBufferStyle::Color3:
	case NXGUICBufferStyle::Color4:
		return { 0.0f, 1.0f }; // min, max
	}

	return { 0.01f, 0.0f }; // speed, ---
}

std::string ConvertShaderResourceDataToNSLParam(const std::vector<NXGUICBufferData>& cbInfosDisplay, const std::vector<NXGUITextureData>& texInfosDisplay, const std::vector<NXGUISamplerData>& ssInfosDisplay)
{
	std::string strNSLParamBegin = "Params\n{\n";
	std::string strNSLParamEnd = "}\n";

	std::string strNSLParam;
	for (auto& texDisplay : texInfosDisplay)
	{
		strNSLParam += "\t";
		strNSLParam += "Tex2D";
		strNSLParam += " : ";
		strNSLParam += texDisplay.name;
		strNSLParam += "\n";
	}

	for (auto& ssDisplay : ssInfosDisplay)
	{
		strNSLParam += "\t";
		strNSLParam += "SamplerState";
		strNSLParam += " : ";
		strNSLParam += ssDisplay.name;
		strNSLParam += "\n";
	}

	strNSLParam += "\t";
	strNSLParam += "CBuffer\n\t{\n";
	for (auto& cbDisplay : cbInfosDisplay)
	{
		std::string strCBType;
		switch (cbDisplay.guiStyle)
		{
		case NXGUICBufferStyle::Value:
		case NXGUICBufferStyle::Slider:
			strCBType = "float";
			break;
		case NXGUICBufferStyle::Value2:
		case NXGUICBufferStyle::Slider2:
			strCBType = "float2";
			break;
		case NXGUICBufferStyle::Value3:
		case NXGUICBufferStyle::Slider3:
		case NXGUICBufferStyle::Color3:
			strCBType = "float3";
			break;
		case NXGUICBufferStyle::Value4:
		case NXGUICBufferStyle::Slider4:
		case NXGUICBufferStyle::Color4:
			strCBType = "float4";
			break;
		default: continue;
		}

		strNSLParam += "\t\t";
		strNSLParam += strCBType;
		strNSLParam += " : ";
		strNSLParam += cbDisplay.name;
		strNSLParam += "\n";
	}
	strNSLParam += "\t}\n";
	return strNSLParamBegin + strNSLParam + strNSLParamEnd;
}

std::filesystem::path GenerateAssetNameJudge(const std::filesystem::path& strFolderPath, const std::string& strSuffix, const std::string& strJudge)
{
	// 整体思路：
	// 判断一下当前Folder下所有扩展名类型为 strSuffix 的文件，如果文件名是 strJudge + [任意数字] 的形式，记下这个数字。
	// 遍历完成时，确定 最大的那个数字+1。若没有这种文件，则使用 1。
	// 然后在当前文件夹下返回这个路径。

	UINT nMaxNumber = 0;
	for (auto& file : std::filesystem::directory_iterator(strFolderPath))
	{
		if (file.path().extension().string() == strSuffix)
		{
			std::string strFileName = file.path().stem().string();
			if (strFileName.substr(0, strJudge.length() + 1) == strJudge + " ")
			{
				UINT num = std::stoi(strFileName.substr(strJudge.length() + 1));
				if (num > nMaxNumber)
					nMaxNumber = num;
			}
		}
	}
	nMaxNumber++;

	return strFolderPath / (strJudge + " " + std::to_string(nMaxNumber) + strSuffix);
}

std::vector<std::filesystem::path> GetFilesInFolder(const std::filesystem::path& strFolderPath, const std::string& strSuffix)
{
	std::vector<std::filesystem::path> resultPaths;
	if (!std::filesystem::exists(strFolderPath) || !std::filesystem::is_directory(strFolderPath)) 
		return resultPaths;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(strFolderPath)) 
	{
		if (std::filesystem::is_regular_file(entry) && entry.path().extension().string() == strSuffix)
		{
			resultPaths.push_back(entry.path());
		}
	}

	return resultPaths;
}

UINT GetValueNumOfGUIStyle(NXGUICBufferStyle eGuiStyle)
{
	switch (eGuiStyle)
	{
	case NXGUICBufferStyle::Value:
	case NXGUICBufferStyle::Slider:
		return 1;
	case NXGUICBufferStyle::Value2:
	case NXGUICBufferStyle::Slider2:
		return 2;
	case NXGUICBufferStyle::Value3:
	case NXGUICBufferStyle::Slider3:
	case NXGUICBufferStyle::Color3:
		return 3;
	case NXGUICBufferStyle::Value4:
	case NXGUICBufferStyle::Slider4:
	case NXGUICBufferStyle::Color4:
	default:
		return 4;
	}
}

}
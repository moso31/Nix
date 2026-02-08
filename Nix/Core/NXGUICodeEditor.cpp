#include "NXGUICodeEditor.h"

int NXGUICodeEditor::FileData::IDCounter = 0;

// static variables
std::vector<std::vector<std::string>> const NXGUICodeEditor::s_hlsl_tokens =
{
    // comment
    {"//"},
    // values
    { "void", "struct", "true", "false", "bool", "int", "uint", "dword", "half", "float", "double", "min16float", "min10float", "min16int", "min12int", "min16uint", "bool1", "bool2", "bool3", "bool4", "int1", "int2", "int3", "int4", "uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4", "float1", "float2", "float3", "float4", "double1", "double2", "double3", "double4", "min16float1", "min16float2", "min16float3", "min16float4", "min10float1", "min10float2", "min10float3", "min10float4", "min16int1", "min16int2", "min16int3", "min16int4", "min12int1", "min12int2", "min12int3", "min12int4", "min16uint1", "min16uint2", "min16uint3", "min16uint4", "float1x1", "float1x2", "float1x3", "float1x4", "float2x1", "float2x2", "float2x3", "float2x4", "float3x1", "float3x2", "float3x3", "float3x4", "float4x1", "float4x2", "float4x3", "float4x4", "double1x1", "double1x2", "double1x3", "double1x4", "double2x1", "double2x2", "double2x3", "double2x4", "double3x1", "double3x2", "double3x3", "double3x4", "double4x1", "double4x2", "double4x3", "double4x4", "vector", "matrix", "extern", "nointerpolation", "precise", "shared", "groupshared", "static", "uniform", "volatile", "const", "row_major", "column_major", "packoffset", "register" },
    // types
    { "string", "cbuffer", "Buffer", "texture", "sampler", "sampler1D", "sampler2D", "sampler3D", "samplerCUBE", "sampler_state", "SamplerState", "SamplerComparisonState", "AppendStructuredBuffer", "Buffer", "ByteAddressBuffer", "ConsumeStructuredBuffer", "InputPatch", "OutputPatch", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer", "RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "StructuredBuffer", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture3D", "Texture2DMS", "Texture2DMSArray", "TextureCube", "TextureCubeArray" },
    // conditional branches
    { "if", "else", "for", "while", "do", "switch", "case", "default", "break", "continue", "discard", "return" },
    // methods
    { "abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx", "ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync", "distance", "dot", "dst", "errorf", "EvaluateAttributeCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2", "f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount", "GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange", "InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan", "ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf", "Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg", "ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin", "radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step", "tan", "tanh", "tex1D", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2Dgrad", "tex2Dlod", "tex2Dproj", "tex3D", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc" },
    // texture methods
    { "CalculateLevelOfDetail", "CalculateLevelOfDetailUnclamped", "Gather", "GetDimensions", "GetSamplePosition", "Load", "Sample", "SampleLevel", "SampleBias", "SampleCmp", "SampleCmpLevelZero", "SampleGrad" },
};

std::vector<ImU32> NXGUICodeEditor::s_hlsl_token_color =
{
       0xff4fff4f, // comments
       0xffff9f4f, // values
       0xff00ffff, // types
       0xffff6fff, // conditional branches
       0xffffff4f, // methods
       0xffdfff6f, // texture methods
};

NXGUICodeEditor::NXGUICodeEditor(ImFont* pFont) :
    m_pFont(pFont),
    m_threadPool(2)
{
    // get single char size of font
    const ImVec2 fontSize = m_pFont->CalcTextSizeA(m_pFont->FontSize, FLT_MAX, -1.0f, " ");

    // 2023.7.4 浠呮敮鎸佺瓑瀹藉瓧浣擄紒鍏跺畠瀛椾綋鎰熻鐣ユ湁鐐瑰悆鎬ц兘锛屼笖娌′粈涔堝繀瑕併??

    // 棰勫瓨鍌ㄥ崟涓瓧绗︾殑 xy鍍忕礌澶у皬
    m_charWidth = fontSize.x;
    m_charHeight = fontSize.y + ImGui::GetStyle().ItemSpacing.y;

    // 琛屽彿鑷冲皯鏈変袱浣嶇殑瀹藉害
    m_maxLineNumber = 99;
    CalcLineNumberRectWidth();
}

void NXGUICodeEditor::Load(const std::filesystem::path& filePath, bool bRefreshHighLight)
{
    // 閫愯璇诲彇鏌愪釜鏂囦欢鐨勬枃鏈俊鎭? 
    std::ifstream ifs(filePath);

    if (!ifs.is_open())
        return;

    for (const auto& file : m_textFiles)
    {
        // 濡傛灉鏄潵鑷‖鐩樼殑鏂囦欢锛屽苟涓斿凡缁忔墦寮?浜嗚繖涓枃浠讹紝灏变笉鍐嶉噸澶嶆墦寮?
        if (file.SameAs(filePath)) return;
    }

    m_bIsSelecting = false;
    m_selections.clear();

    // 蹇呴』娓呯┖绾跨▼姹狅紒鍥犱负 m_textFiles.emplace_back() 鍙兘浼氬鑷村唴瀛樺湴鍧?鍙樻洿銆?
    // 鑻ヤ笉娓呯┖锛屾鏃剁嚎绋嬫睜task()灏变細缁х画淇敼m_textFiles涔嬪墠鎸囧悜鐨勫唴瀛橈紝瀵艰嚧鏈畾涔夌殑琛屼负銆?
    m_threadPool.Clear();

    // 灏嗘柊鎵撳紑鐨勬枃浠跺姞鍏ュ埌 m_textFiles 涓?
    FileData& newFile = m_textFiles.emplace_back(filePath, true);
    auto& lines = newFile.lines;

    // 閫愯璇诲彇鏂囦欢鍐呭鍒? lines 
    TextString line;
    while (std::getline(ifs, line))
    {
        // 灏嗘墍鏈? tab 鏇挎崲鎴? 4 spaces
        for (int i = 0; i < line.size(); i++)
        {
            if (line[i] == '\t')
            {
                line.replace(i, 1, "    ");
                i += 3;
            }
        }

        lines.push_back(line);
    }

    if (bRefreshHighLight)
    {
        // 鍒濆鍖栨瘡琛岀殑鏇存柊鏃堕棿
        newFile.updateTime.assign(lines.size(), ImGui::GetTime());

        // 寮傛楂樹寒璇硶
        int fileIndex = (int)m_textFiles.size() - 1;
        for (int i = 0; i < lines.size(); i++)
            m_threadPool.Add([this, fileIndex, i]() { HighLightSyntax(fileIndex, i); });
    }

    ifs.close();
}

void NXGUICodeEditor::Load(const std::string& text, bool bRefreshHighLight, const std::string& customName)
{
    m_bIsSelecting = false;
    m_selections.clear();

    // 蹇呴』娓呯┖绾跨▼姹狅紒鍥犱负 m_textFiles.emplace_back() 鍙兘浼氬鑷村唴瀛樺湴鍧?鍙樻洿銆?
    // 鑻ヤ笉娓呯┖锛屾鏃剁嚎绋嬫睜task()灏变細缁х画淇敼m_textFiles涔嬪墠鎸囧悜鐨勫唴瀛橈紝瀵艰嚧鏈畾涔夌殑琛屼负銆?
    m_threadPool.Clear();

    // 濡傛灉涓嶆槸鏉ヨ嚜纭洏鐨勬枃浠讹紝鏃犻渶鏌ラ噸鐩存帴鍒涘缓
    FileData& newFile = m_textFiles.emplace_back(customName, false);
    auto& lines = newFile.lines;

    lines.clear();

    // 鎸? "\n" 鎷嗗垎 text
    size_t start = 0;
    size_t end = text.find("\n");

    while (end != std::string::npos)
    {
        // 鐒跺悗閫愯鍔犺浇鍒? lines 
        std::string line(text.substr(start, end - start));

        // 灏嗘墍鏈? tab 鏇挎崲鎴? 4 spaces
        for (int i = 0; i < line.size(); i++)
        {
            if (line[i] == '\t')
            {
                line.replace(i, 1, "    ");
                i += 3;
            }
        }

        lines.push_back(line);
        start = end + 1;
        end = text.find("\n", start);
    }

    // 鎶婃渶鍚庝竴琛屼篃鍔犲叆鍒? lines 涓?
    std::string line(text.substr(start, end - start));
    for (int i = 0; i < line.size(); i++)
    {
        if (line[i] == '\t')
        {
            line.replace(i, 1, "    ");
            i += 3;
        }
    }
    lines.push_back(line);

    if (bRefreshHighLight)
    {
        // 鍒濆鍖栨瘡琛岀殑鏇存柊鏃堕棿
        newFile.updateTime.assign(lines.size(), ImGui::GetTime());

        // 寮傛楂樹寒璇硶
        int fileIndex = (int)m_textFiles.size() - 1;
        for (int i = 0; i < lines.size(); i++)
            m_threadPool.Add([this, fileIndex, i]() { HighLightSyntax(fileIndex, i); });
    }
}

bool NXGUICodeEditor::RemoveFile(int removeIndex)
{
    if (removeIndex < 0 || removeIndex >= m_textFiles.size())
		return false;

	m_textFiles.erase(m_textFiles.begin() + removeIndex);
	m_bIsSelecting = false;
	m_selections.clear();
	m_threadPool.Clear();

    return true;
}

void NXGUICodeEditor::ClearAllFiles()
{
    m_textFiles.clear();
	m_bIsSelecting = false;
	m_selections.clear();
	m_threadPool.Clear();
}

void NXGUICodeEditor::RefreshAllHighLights()
{
    for (int i = 0; i < m_textFiles.size(); i++)
    {
        auto& file = m_textFiles[i];
        auto& lines = file.lines;

        // 鍒濆鍖栨瘡琛岀殑鏇存柊鏃堕棿
        file.updateTime.assign(lines.size(), ImGui::GetTime());
    }

    for (int i = 0; i < m_textFiles.size(); i++)
    {
        auto& file = m_textFiles[i];

        // 寮傛楂樹寒璇硶
        for (int j = 0; j < file.lines.size(); j++)
            m_threadPool.Add([this, i, j]() { HighLightSyntax(i, j); });
    }
}

void NXGUICodeEditor::Render()
{
    ImGui::PushFont(m_pFont);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 4.0f));

    if (m_bResetFlickerDt)
    {
        m_flickerDt = ImGui::GetTime();
        m_bResetFlickerDt = false;
    }

    if (m_pickingIndex >= 0 && m_pickingIndex < m_textFiles.size())
    {
        auto& currFile = m_textFiles[m_pickingIndex];
        auto& lines = currFile.lines;

        size_t newLineNumber = std::max(m_maxLineNumber, lines.size());
        if (m_maxLineNumber < newLineNumber)
        {
            m_maxLineNumber = newLineNumber;
            CalcLineNumberRectWidth(); // 琛屽彿瓒呭嚭娓叉煋鐭╅樀鑼冨洿鏃讹紝閲嶆柊璁＄畻娓叉煋鐭╅樀鐨勫搴?
        }

        ImGui::BeginChild("TextEditor");
        //if (ImGui::IsWindowFocused()) m_bNeedFocusOnText = true;

        const ImVec2& layerStartCursorPos = ImGui::GetCursorPos();

        ImGui::BeginChild("##main_layer", ImVec2(), false, ImGuiWindowFlags_NoInputs);
        Render_MainLayer();
        ImGui::EndChild();

        //ImGui::SetCursorPos(ImVec2(layerStartCursorPos.x + ImGui::GetContentRegionAvail().x - 400.0f, layerStartCursorPos.y));
        //ImGui::BeginChild("##debug_layer", ImVec2(), false, ImGuiWindowFlags_NoInputs);
        //Render_DebugLayer();
        //ImGui::EndChild();

        ImGui::EndChild();
    }

    ImGui::PopStyleVar();
    ImGui::PopFont();
}

void NXGUICodeEditor::AddSelection(int row, bool bScrollToThere)
{
    AddSelection({ row, 0 }, { row, INT_MAX });
    if (bScrollToThere) m_bNeedScrollCheck = 2;
}

void NXGUICodeEditor::AddSelection(int row, int col, bool bScrollToThere)
{
    AddSelection({ row, col }, { row, col });
    if (bScrollToThere) m_bNeedScrollCheck = 2;
}

void NXGUICodeEditor::AddSelection(int rowL, int colL, int rowR, int colR, bool bScrollToThere)
{
    AddSelection({ rowL, colL }, { rowR, colR });
    if (bScrollToThere) m_bNeedScrollCheck = 2;
}

std::string NXGUICodeEditor::GetCodeText(int index)
{
    if (index < 0 || index >= m_textFiles.size())
        return std::string();

    auto& lines = m_textFiles[index].lines;
    std::string text;
    for (int i = 0; i < lines.size(); i++)
    {
        text += lines[i];
        if (i != lines.size() - 1)
            text += "\n";
    }
    return text;
}

void NXGUICodeEditor::AddSelection(const Coordinate& A, const Coordinate& B)
{
    // 姝ゆ柟娉曞紑閿?涓嶉珮锛岄殢渚挎姌鑵?
    const auto& file = m_textFiles[m_pickingIndex].lines;
    const auto& lineA = file[A.row];
    const auto& lineB = file[B.row];
    SelectionInfo selection(
        { 
            std::clamp(A.row, 0, (int)file.size()),
            std::clamp(A.col, 0, (int)lineA.size()),
        },
        {
            std::clamp(B.row, 0, (int)file.size()),
            std::clamp(B.col, 0, (int)lineB.size()),
        }
    );
	m_selections.push_back(selection);
}

void NXGUICodeEditor::RemoveSelection(const SelectionInfo& removeSelection)
{
    std::erase_if(m_selections, [&removeSelection](const SelectionInfo& selection) { return removeSelection == selection; });
}

void NXGUICodeEditor::ClearSelection()
{
    m_selections.clear();
}

void NXGUICodeEditor::Enter(const std::vector<std::vector<std::string>>& strArray)
{
    auto& lines = m_textFiles[m_pickingIndex].lines;

    // 鎸夎鍒楀彿椤哄簭鎺掑簭
    std::sort(m_selections.begin(), m_selections.end(), [](const SelectionInfo& a, const SelectionInfo& b) { return a.R < b.R; });

    // 浠庡悗寰?鍓嶆尐涓鐞嗭紝澶嶆潅搴(selection^2)
    // 姣忓鐞嗕竴涓? selection锛岄兘闇?瑕佽ˉ鍋胯绠椾箣鍓嶇畻杩囩殑鎵?鏈? selection 鐨勪綅缃?
    for (int i = (int)m_selections.size() - 1; i >= 0; i--)
    {
        auto& selection = m_selections[i];
        const auto& L = selection.L;
        const auto& R = selection.R;

        // 鑻ユ湁閫夊尯锛屽厛娓呯┖
        if (L != R) 
        {
            // 鏈夐?夊尯锛氬垹闄ら?夊尯涓殑鎵?鏈夊唴瀹?
            if (L.row == R.row) // 鍗曡
            {
                auto& line = lines[L.row];
                line.erase(line.begin() + L.col, line.begin() + R.col);

                // 琛ュ伩璁＄畻
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    int shiftLength = CalcSelectionLength(selection);
                    if (sel.L.row != L.row) break; // 鍗曡鏃跺彧闇?澶勭悊鍚岃鍐呭悗闈㈢殑鏂囨湰
                    sel.L.col -= shiftLength;
                    sel.R.col -= shiftLength;
                }
            }
            else // 澶氳
            {
                // 鍒犻櫎宸︿晶琛岀殑鍙充晶閮ㄥ垎
                auto& lineL = lines[L.row];
                int startErasePos = std::min(L.col, (int)lineL.size());
                lineL.erase(lineL.begin() + startErasePos, lineL.end());

                // 鍒犻櫎鍙充晶琛岀殑宸︿晶閮ㄥ垎
                auto& lineR = lines[R.row];
                int endErasePos = std::min(R.col, (int)lineR.size());
                lineR.erase(lineR.begin(), lineR.begin() + endErasePos);

                // 灏嗗彸渚ц鐨勫唴瀹瑰悎骞跺埌宸︿晶琛?
                lineL.append(lineR);

                // 鍒犻櫎涓棿琛?
                lines.erase(std::max(lines.begin(), lines.begin() + L.row + 1), std::min(lines.begin() + R.row + 1, lines.end()));

                // 琛ュ伩璁＄畻
                bool bSameLine = true;
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    if (sel.L.row != R.row) bSameLine = false;
                    if (bSameLine)
                    {
                        // 鍚庣画閫夊尯濡傛灉鍦ㄥ悓涓?琛?
                        int shiftLength = R.col - L.col;
                        sel.L.row = L.row;
                        sel.L.col -= shiftLength;
                        sel.R = sel.L;
                    }
                    else
                    {
                        // 鍚庣画閫夊尯濡傛灉涓嶅湪鍚屼竴琛?
                        int shiftLength = R.row - L.row;
                        sel.L.row -= shiftLength;
                        sel.R.row -= shiftLength;
                    }
                }
            }

            // 鏇存柊鍏夋爣浣嶇疆
            selection.L = selection.R = Coordinate(L.row, L.col);
        }

        // 娓呯┖瀹屾垚锛屽紑濮嬫彃鍏ユ枃鏈?...
        auto& line = lines[L.row];
        int allLineIdx = 0;
        std::string strPart2;
        for (int strIdx = 0; strIdx < strArray.size(); strIdx++)
        {
            const auto& str = strArray[strIdx];
            for (int lineIdx = 0; lineIdx < str.size(); lineIdx++, allLineIdx++)
            {
                std::string strLine = str[lineIdx];

                // 灏? strLine 鐨勬墍鏈? '\t' 鏇挎崲鎴? "    "
                size_t pos = strLine.find('\t');
                while (pos != std::string::npos) 
                {
                    strLine.replace(pos, 1, "    ");
                    pos = strLine.find('\t', pos + 4);
                }

                if (allLineIdx == 0)
                {
                    int cutIdx = std::min(L.col, (int)line.size());
                    // 濡傛灉鏄涓?娈电殑绗竴琛岋紝鍦ㄥ厜鏍囧鎴柇鍘熷瀛楃涓诧紝鍦ㄥ墠鍗婃鍚庨潰鎻掑叆鏂版枃鏈?傚悓鏃朵繚鐣欏悗鍗婃銆?
                    std::string strPart1 = line.substr(0, cutIdx);
                    strPart2 = line.substr(cutIdx);
                    line = strPart1 + strLine;
                }
                else
                {
                    // 鍏朵粬娈靛叏閮ㄧ洿鎺ユ彃鍏ユ暣琛?
                    lines.insert(lines.begin() + L.row + allLineIdx, strLine);
                }

                // 濡傛灉鏄渶鍚庝竴娈电殑鏈?鍚庝竴琛屻?傚皢涔嬪墠淇濈暀鐨勫悗鍗婃缁笂銆?
                if (strIdx == strArray.size() - 1 && lineIdx == str.size() - 1)
                {
                    lines[L.row + allLineIdx] += strPart2;
                }

                SetLineUpdateTime(m_pickingIndex, L.row + allLineIdx);

                // 瀵瑰墠涓よ锛屽悓姝ュ鐞嗛珮浜?傝秴杩囦袱琛岀殑锛屽叏閮ㄥ紓姝ュ鐞?
                if (allLineIdx <= 2 || true)
                    HighLightSyntax(m_pickingIndex, L.row + allLineIdx);
                else
                    m_threadPool.Add([this, L, allLineIdx]() { HighLightSyntax(m_pickingIndex, L.row + allLineIdx); });
            }
        }

        // 鏇存柊 selection 鐨勪綅缃?
        if (allLineIdx <= 0) {} // 浠?涔堥兘涓嶅仛
        else if (allLineIdx == 1) // 杈撳叆鍙湁涓?琛?
        {
            int shiftLength = (int)strArray[0][0].length();
            selection.L.col += shiftLength;
            selection.R = selection.L;

            // 琛ュ伩璁＄畻
            for (int j = i + 1; j < m_selections.size(); j++)
            {
                auto& sel = m_selections[j];
                if (sel.L.row != L.row) break; // 鍗曡鏃跺彧闇?澶勭悊鍚岃鍐呯殑 selection
                sel.L.col += shiftLength;
                sel.R.col += shiftLength;
            }
        }
        else // 杈撳叆鏈夊琛?
        {
            int shiftLength = (int)strArray.back().back().length();
            selection.L.row += allLineIdx - 1;
            selection.L.col = shiftLength;
            selection.R = selection.L;

            // 琛ュ伩璁＄畻
            for (int j = i + 1; j < m_selections.size(); j++)
            {
                auto& sel = m_selections[j];
                sel.L.row += allLineIdx - 1;
                sel.L.col = shiftLength;
                sel.R = sel.L;
            }
        }
    }

    SelectionsOverlayCheckForKeyEvent(false);
    ScrollCheck();
}

void NXGUICodeEditor::Backspace(bool bDelete, bool bCtrl)
{
    auto& lines = m_textFiles[m_pickingIndex].lines;
    std::sort(m_selections.begin(), m_selections.end(), [](const SelectionInfo& a, const SelectionInfo& b) { return a.R < b.R; });

    // 鎸夎鍒楀彿浠庡悗寰?鍓嶆尐涓鐞嗭紝澶嶆潅搴(selection^2)
    // 姣忓鐞嗕竴涓? selection锛岄兘闇?瑕佽ˉ鍋胯绠椾箣鍓嶇畻杩囩殑鎵?鏈? selection 鐨勪綅缃?
    for (int i = (int)m_selections.size() - 1; i >= 0; i--)
    {
        auto& selection = m_selections[i];
        const auto& L = selection.L;
        const auto& R = selection.R;

        if (L == R)
        {
            // 鏃犻?夊尯锛氬垹闄や笂涓?涓瓧绗︼紝鍏夋爣閫?涓?鏍?
            auto& line = lines[L.row];

            bool bNeedCombineLastLine = (L.row > 0 && L.col == 0 && !bDelete); // 闇?瑕佸拰涓婁竴琛屽悎骞讹細浣嶄簬鍒楅锛屼笖鎸変簡backspace锛?
            bool bNeedCombineNextLine = (L.row < lines.size() - 1 && L.col >= line.size() && bDelete); // 闇?瑕佸拰涓嬩竴琛屽悎骞讹細浣嶄簬鍒楀熬锛屼笖鎸変簡delete锛?
            bool bNeedCombineLines = bNeedCombineLastLine || bNeedCombineNextLine;

            if (!bNeedCombineLines)
            {
                int eraseSize = 1; // 鎸夊瓧绗﹀垹闄?
                if (bCtrl) // 鎸夊崟璇嶅垹闄?
                {
                    int pos = L.col;
                    if (!bDelete) // ctrl+backspace
                    {
                        while (pos > 0 && IsWordSplitChar(line[pos - 1])) pos--;
                        while (pos > 0 && !IsWordSplitChar(line[pos - 1])) pos--;
                        eraseSize = std::max(0, L.col - pos);
                    }
                    else // ctrl+delete
                    {
                        while (pos < line.size() && IsWordSplitChar(line[pos])) pos++;
                        while (pos < line.size() && !IsWordSplitChar(line[pos])) pos++;
                        eraseSize = std::max(0, pos - L.col);
                    }
                }

                if (!bDelete)
                {
                    int erasePosR = std::min(L.col, (int)line.size());
                    int erasePosL = std::max(0, erasePosR - eraseSize);
                    line.erase(line.begin() + erasePosL, line.begin() + erasePosR);
                    selection.L = selection.R = Coordinate(L.row, erasePosL);
                }
                else
                {
                    int erasePosL = std::min(L.col, (int)line.size());
                    int erasePosR = std::min(erasePosL + eraseSize, (int)line.size());
                    line.erase(line.begin() + erasePosL, line.begin() + erasePosR);
                }

                // 琛ュ伩璁＄畻
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    if (sel.L.row != L.row) break; // 鍗曡鏃跺彧闇?澶勭悊鍚岃鍐呭悗闈㈢殑鏂囨湰
                    sel.L.col -= eraseSize;
                    sel.R.col -= eraseSize;
                }

                SetLineUpdateTime(m_pickingIndex, L.row, ImGui::GetTime());
                HighLightSyntax(m_pickingIndex, L.row);
            }
            else // 璺ㄨ
            {
                if (bNeedCombineLastLine)
                {
                    auto& lastLine = lines[L.row - 1];
                    int lastLength = (int)lastLine.length();
                    lastLine.append(line);
                    lines.erase(lines.begin() + L.row);

                    selection.L = selection.R = Coordinate(L.row - 1, lastLength);
                }
                else if (bNeedCombineNextLine)
                {
                    auto& nextLine = lines[L.row + 1];
                    int lastLength = (int)line.length();
                    line.append(nextLine);
                    lines.erase(lines.begin() + L.row + 1);
                    selection.L = selection.R = Coordinate(L.row, lastLength);
                }

                // 琛ュ伩璁＄畻
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    sel.L.row--;
                    if (sel.L.row == L.row) sel.L.col += L.col; // 褰撳湪鍒楀熬 delete 鏃讹紝鐗规畩澶勭悊
                    sel.R = sel.L;
                }

                SetLineUpdateTime(m_pickingIndex, L.row);
                HighLightSyntax(m_pickingIndex, L.row);
                if (L.row + 1 < lines.size())
                {
                    SetLineUpdateTime(m_pickingIndex, L.row + 1);
                    HighLightSyntax(m_pickingIndex, L.row + 1);
                }
            }
        }
        else
        {
            // 鏈夐?夊尯锛氬垹闄ら?夊尯涓殑鎵?鏈夊唴瀹?
            if (L.row == R.row) // 鍗曡
            {
                auto& line = lines[L.row];
                int startErasePos = std::min(L.col, (int)line.size());
                int endErasePos = std::min(R.col, (int)line.size());
                line.erase(line.begin() + startErasePos, line.begin() + endErasePos);

                // 琛ュ伩璁＄畻
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    int shiftLength = CalcSelectionLength(selection);
                    if (sel.L.row != L.row) break; // 鍗曡鏃跺彧闇?澶勭悊鍚岃鍐呭悗闈㈢殑鏂囨湰
                    sel.L.col -= shiftLength;
                    sel.R.col -= shiftLength;
                }

                SetLineUpdateTime(m_pickingIndex, L.row, ImGui::GetTime());
                HighLightSyntax(m_pickingIndex, L.row);
            }
            else // 澶氳
            {
                // 鍒犻櫎宸︿晶琛岀殑鍙充晶閮ㄥ垎
                auto& lineL = lines[L.row];
                int startErasePos = std::min(L.col, (int)lineL.size());
                lineL.erase(lineL.begin() + startErasePos, lineL.end());

                // 鍒犻櫎鍙充晶琛岀殑宸︿晶閮ㄥ垎
                auto& lineR = lines[R.row];
                int endErasePos = std::min(R.col, (int)lineR.size());
                lineR.erase(lineR.begin(), lineR.begin() + endErasePos);

                // 灏嗗彸渚ц鐨勫唴瀹瑰悎骞跺埌宸︿晶琛?
                lineL.append(lineR);

                // 鍒犻櫎涓棿琛?
                lines.erase(std::max(lines.begin(), lines.begin() + L.row + 1), std::min(lines.begin() + R.row + 1, lines.end()));

                // 琛ュ伩璁＄畻
                bool bSameLine = true;
                for (int j = i + 1; j < m_selections.size(); j++)
                {
                    auto& sel = m_selections[j];
                    if (sel.L.row != R.row) bSameLine = false;
                    if (bSameLine)
                    {
                        // 鍚庣画閫夊尯濡傛灉鍦ㄥ悓涓?琛?
                        int shiftLength = R.col - L.col;
                        sel.L.row = L.row;
                        sel.L.col -= shiftLength;
                        sel.R = sel.L;
                    }
                    else
                    {
                        // 鍚庣画閫夊尯濡傛灉涓嶅湪鍚屼竴琛?
                        int shiftLength = R.row - L.row;
                        sel.L.row -= shiftLength;
                        sel.R.row -= shiftLength;
                    }
                }

                SetLineUpdateTime(m_pickingIndex, L.row, ImGui::GetTime());
                SetLineUpdateTime(m_pickingIndex, L.row + 1, ImGui::GetTime());
                HighLightSyntax(m_pickingIndex, L.row);
                if (L.row + 1 < lines.size()) HighLightSyntax(m_pickingIndex, L.row + 1);
            }

            // 鏇存柊鍏夋爣浣嶇疆
            selection.L = selection.R = Coordinate(L.row, L.col);
        }
    }

    SelectionsOverlayCheckForKeyEvent(false);
    ScrollCheck();
}

void NXGUICodeEditor::Escape()
{
    if (m_selections.empty()) return;

    // 鎸? Esc 鍚庯紝浠呬繚鐣欐渶鍚庣殑 Selection锛屽叾瀹冪殑娓呴櫎銆?
    auto lastSel = m_selections.back();
    m_selections.assign(1, lastSel);
}

void NXGUICodeEditor::Copy()
{
    auto& lines = m_textFiles[m_pickingIndex].lines;
    std::vector<std::string> copyLines;

    // Copy绛栫暐锛氶亶鍘嗘墍鏈? selections锛?
    // 1. 濡傛灉selection鏄釜鍗曢?夊厜鏍囷紝copy鍏夋爣鎵?鍦ㄧ殑涓?鏁磋銆?
    // 2. 濡傛灉selection鏄釜閫夊尯锛宑opy鏁翠釜閫夊尯锛?
    for (const auto& selection : m_selections)
    {
        const auto& L = selection.L;
        const auto& R = selection.R;
        if (L == R) // rule 1.
        {
            copyLines.push_back("");
            copyLines.push_back(lines[L.row]);
        }
        else // rule 2
        {
            if (L.row == R.row) // 鍚岃
            {
                copyLines.push_back(lines[L.row].substr(L.col, R.col - L.col));
            }
            else // 璺ㄨ
            {
                copyLines.push_back(lines[L.row].substr(std::min(L.col, (int)lines[L.row].size())));
                for (int i = L.row + 1; i < R.row; i++)
                {
                    copyLines.push_back(lines[i]);
                }
                copyLines.push_back(lines[R.row].substr(0, std::min(R.col, (int)lines[R.row].size())));
            }
        }
    }

    std::string clipBoardText;
    for (const auto& str : copyLines) clipBoardText.append(str + '\n');
    ImGui::SetClipboardText(clipBoardText.c_str());
}

void NXGUICodeEditor::Cut()
{
    Copy();
    Backspace(true, false);
}

void NXGUICodeEditor::Paste()
{
    std::string clipText = ImGui::GetClipboardText();
    if (!clipText.empty() && clipText.back() == '\n') clipText.pop_back(); // ImGui鐨勫壀璐存澘浼氬湪鍐呭鏈熬鑷甫涓?涓?'\n'锛屽簲璇ュ幓鎺?

    std::vector<std::string> lines;
    size_t pos = 0;
    std::string token;
    while ((pos = clipText.find('\n')) != std::string::npos)
    {
        token = clipText.substr(0, pos);
        if (!token.empty() && token.back() == '\r') token.pop_back(); // 濡傛灉琛屽熬鏈? '\r'锛屽幓鎺?
        lines.push_back(token);
        clipText.erase(0, pos + 1);
    }
    if (!clipText.empty() && clipText.back() == '\r') clipText.pop_back(); // 濡傛灉琛屽熬鏈? '\r'锛屽幓鎺?
    lines.push_back(clipText);
    Enter({ lines });
}

void NXGUICodeEditor::SelectAll()
{
    auto& lines = m_textFiles[m_pickingIndex].lines;
    m_selections.assign(1, { {0, 0}, {(int)lines.size() - 1, (int)lines.back().length()} });
}

void NXGUICodeEditor::HighLightSyntax(int fileIndex, int lineIndex)
{
    auto& lines = m_textFiles[fileIndex].lines;
    TextString strLine = lines[lineIndex];
    std::vector<TextKeyword> strWords = ExtractKeywords(strLine);
    for (auto& strWord : strWords)
    {
        for (int i = 0; i < s_hlsl_tokens.size(); i++)
        {
            if (strWord.tokenColorIndex != -1) break;
            for (const auto& token : s_hlsl_tokens[i])
            {
                if (strWord.string == token)
                {
                    strWord.tokenColorIndex = i;
                    break;
                }
            }
        }
    }

    strLine.formatArray.clear();
    int idx = 0;
    for (const auto& strWord : strWords)
    {
        if (strWord.tokenColorIndex == -1)
        {
            continue;
        }

        int tokenLength = (int)strWord.string.length();
        if (strWord.startIndex > idx)
            strLine.formatArray.push_back(TextFormat(0xffffffff, strWord.startIndex - idx));

        if (strWord.tokenColorIndex == 0)
        {
            strLine.formatArray.push_back(TextFormat(s_hlsl_token_color[strWord.tokenColorIndex], INT_MAX));
            break;
        }

        strLine.formatArray.push_back(TextFormat(s_hlsl_token_color[strWord.tokenColorIndex], tokenLength));
        idx = strWord.startIndex + tokenLength;
    }

    // callback
    double currTime = ImGui::GetTime();
    auto& linesUpdateTime = m_textFiles[fileIndex].updateTime;
    if (currTime >= linesUpdateTime[lineIndex])
    {
        linesUpdateTime[lineIndex] = currTime;
        lines[lineIndex] = strLine;
    }
}

void NXGUICodeEditor::SetLineUpdateTime(int fileIndex, int lineIndex, double manualTime)
{
    auto& lines = m_textFiles[fileIndex].lines;
    auto& linesUpdateTime = m_textFiles[fileIndex].updateTime;
    double time = manualTime == FLT_MIN ? ImGui::GetTime() : manualTime;
    if (linesUpdateTime.size() < lines.size())
        linesUpdateTime.insert(linesUpdateTime.end(), lines.size() - linesUpdateTime.size(), time);
    linesUpdateTime[lineIndex] = time;
}

void NXGUICodeEditor::SwitchFile(int fileIndex)
{
    if (m_pickingIndex == fileIndex)
        return;

    m_pickingIndex = fileIndex;
    m_bIsSelecting = false;
    m_selections.clear();
}

void NXGUICodeEditor::Render_MainLayer()
{
    const ImVec2& windowSize = ImGui::GetWindowSize();
    Render_OnMouseInputs();

    ImGui::SetCursorPos(ImVec2(m_lineTextStartX, 0.0f));
    ImGui::BeginChild("##text_content", ImVec2(windowSize.x - m_lineTextStartX, windowSize.y), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav);
    RenderTexts_OnKeyInputs();
    RenderTexts_OnMouseInputs();
    RenderSelections();
    RenderTexts();
    float scrollY_textContent = ImGui::GetScrollY();
    float scrollBarHeight = ImGui::GetScrollMaxY() > 0.0f ? ImGui::GetStyle().ScrollbarSize : 0.0f;

    if (m_bNeedFocusOnText)
    {
        ImGui::SetKeyboardFocusHere();
        m_bNeedFocusOnText = false;
    }

    if (m_bNeedScrollCheck)
    {
        m_bNeedScrollCheck--; 
        if (m_bNeedScrollCheck == 0) ScrollCheck();
    }

    ImGui::EndChild();

    ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
    ImGui::BeginChild("##line_number", ImVec2(m_lineNumberWidthWithPaddingX, windowSize.y - scrollBarHeight), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::SetScrollY(scrollY_textContent);
    RenderLineNumber();
    ImGui::EndChild();
}

void NXGUICodeEditor::Render_DebugLayer()
{
    //ImGui::SetCursorPos(ImVec2(ImGui::GetContentRegionAvail().x - 400.0f, 0.0f));
    ImGui::BeginChild("##debug_selections", ImVec2(350.0f, ImGui::GetContentRegionAvail().y * 0.4f), true);
    ImGui::Text("Disable Render_DebugLayer() method to hide me!");

    for (size_t i = 0; i < m_selections.size(); i++)
    {
        const auto& selection = m_selections[i];
        std::string info = "Selection " + std::to_string(i) + ":(" + std::to_string(selection.L.row) + ", " + std::to_string(selection.L.col) + ") " + 
            std::string(selection.flickerAtFront ? "<-" : "->") +
            " (" + std::to_string(selection.R.row) + ", " + std::to_string(selection.R.col) + ")";
        ImGui::Text(info.c_str());
    }

    if (m_bIsSelecting)
    {
        const SelectionInfo selection(m_activeSelectionDown, m_activeSelectionMove);
        std::string info = "Active selection:(" + std::to_string(selection.L.row) + ", " + std::to_string(selection.L.col) + ") " +
            std::string(selection.flickerAtFront ? "<-" : "->") +
            " (" + std::to_string(selection.R.row) + ", " + std::to_string(selection.R.col) + ")";
        ImGui::Text(info.c_str());
    }
    ImGui::EndChild();
}

void NXGUICodeEditor::RenderSelections()
{
    for (const auto& selection : m_selections)
        RenderSelection(selection);

    if (m_bIsSelecting)
    {
        SelectionInfo activeSelection(m_activeSelectionDown, m_activeSelectionMove);
        RenderSelection(activeSelection);
    }
}

void NXGUICodeEditor::RenderTexts()
{
    auto& lines = m_textFiles[m_pickingIndex].lines;

    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contextArea = ImGui::GetContentRegionAvail();
    float scrollY = ImGui::GetScrollY();
    int startRow = (int)(scrollY / m_charHeight);
    int endRow = (int)((scrollY + contextArea.y) / m_charHeight);

    m_maxLineCharCount = 0;
    for (int i = 0; i < lines.size(); i++)
    {
        const auto& strLine = lines[i];
        m_maxLineCharCount = std::max(m_maxLineCharCount, strLine.length()); // 璁板綍姣忚鏈?闀跨殑瀛楃鏁帮紝鐢ㄤ簬濉厖绌烘牸

        if (i < startRow || i > endRow)
        {
            if (i == startRow - 1 || i == endRow + 1)
            {
                // 鍦ㄨ閲庡鐨勪笂涓?琛?/涓嬩竴琛岋紝浣跨敤鏈?闀胯瀛楃鏁伴噺鐨勭┖鏍煎～鍏咃紝浠ヤ繚鎸佸浐瀹氶暱搴︼紙閬垮厤scrollX澶辨晥锛?
                ImGui::TextUnformatted(std::string(m_maxLineCharCount, ' ').c_str());
            }
            else
            {
                // 闄や笂杩拌浠ュ锛岃烦杩囧叾浠栬閲庡鐨勮鐨勬枃鏈粯鍒讹紝浼樺寲鎬ц兘銆?
                ImGui::NewLine();
            }
            continue;
        }

        if (strLine.formatArray.empty())
            ImGui::TextUnformatted(strLine.c_str());
        else
        {
            int index = 0;
            for (const auto& strFormat : strLine.formatArray)
            {
                std::string strPart = strLine.substr(index, strFormat.length);
                index += strFormat.length;

                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(strFormat.color));
                ImGui::TextUnformatted(strPart.c_str());
                ImGui::PopStyleColor();

                if (index < strLine.length())
                    ImGui::SameLine();
                else
                    break;
            }

            if (index < strLine.length())
            {
                std::string strPart = strLine.substr(index);

                ImGui::PushStyleColor(ImGuiCol_Text, ImGui::ColorConvertU32ToFloat4(0xffffffff));
                ImGui::TextUnformatted(strPart.c_str());
                ImGui::PopStyleColor();
            }
        }
    }
}

void NXGUICodeEditor::RenderLineNumber()
{
    auto& lines = m_textFiles[m_pickingIndex].lines;

    const ImVec2& windowPos = ImGui::GetWindowPos();
    const ImVec2& windowSize = ImGui::GetWindowSize();
    const auto& drawList = ImGui::GetWindowDrawList();

    drawList->AddRectFilled(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y), IM_COL32(100, 100, 0, 255));

    size_t strLineSize = std::to_string(lines.size()).length();
    for (int i = 0; i < lines.size(); i++)
    {
        std::string strLineNumber = std::to_string(i + 1);
        while (strLineNumber.size() < strLineSize)
            strLineNumber = " " + strLineNumber;

        ImGui::SetCursorPosX(m_lineNumberPaddingX);
        ImGui::TextUnformatted(strLineNumber.c_str());
    }
}

void NXGUICodeEditor::CalcLineNumberRectWidth()
{
    int nLineNumberDigit = 0;
    for (int k = 1; k <= m_maxLineNumber; k *= 10) nLineNumberDigit++;

    // 璁板綍琛屽彿鏂囨湰鑳借揪鍒扮殑鏈?澶у搴?
    m_lineNumberWidth = m_charWidth * nLineNumberDigit;
    m_lineNumberWidthWithPaddingX = m_lineNumberWidth + m_lineNumberPaddingX * 2.0f;

    // 琛屽彿鐭╁舰 - 鏂囨湰 涔嬮棿鐣欎竴涓?4px鐨勭┖
    float paddingX = 4.0f;
    m_lineTextStartX = m_lineNumberWidthWithPaddingX + paddingX;
}

void NXGUICodeEditor::RenderSelection(const SelectionInfo& selection)
{
    auto& lines = m_textFiles[m_pickingIndex].lines;

    const ImVec2& windowPos = ImGui::GetWindowPos();
    const auto& drawList = ImGui::GetWindowDrawList();

    float scrollX = ImGui::GetScrollX();
    float scrollY = ImGui::GetScrollY();

    const Coordinate fromPos(selection.L.row, selection.L.col);
    const Coordinate toPos(selection.R.row, selection.R.col);

    ImVec2 flickerPos;

    // 鍒ゆ柇 A B 鏄惁鍦ㄥ悓涓?琛?
    const bool bSingleLine = fromPos.row == toPos.row;

    if (bSingleLine)
    {
        int linePosL = std::min(fromPos.col, (int)lines[fromPos.row].length());
        int linePosR = std::min(toPos.col, (int)lines[toPos.row].length());

        // 琛岄鍧愭爣
        ImVec2 linePos(windowPos.x, windowPos.y + m_charHeight * fromPos.row - scrollY);

        // 缁樺埗鎵?閫夊璞＄殑閫変腑鐘舵?佺煩褰?
        ImVec2 selectStartPos(linePos.x + linePosL * m_charWidth - scrollX, linePos.y);
        ImVec2 selectEndPos(linePos.x + linePosR * m_charWidth - scrollX, linePos.y + m_charHeight);
        drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

        // 璁＄畻闂儊浣嶇疆
        flickerPos = selection.flickerAtFront ? selectStartPos : ImVec2(selectEndPos.x, selectEndPos.y - m_charHeight);
    }
    else
    {
        // 澶氳鏂囨湰闄ゆ渶鍚庝竴琛岋紝鍏跺畠琛岀殑閫変腑鐭╁舰閮藉悜鍙冲鍔犱竴涓瓧绗﹂暱搴?
        float enterCharOffset = m_charWidth;

        // 缁樺埗棣栬锛屽厛纭畾棣栬瀛楃鍧愭爣
        int linePosR = (int)lines[fromPos.row].length();
        int linePosL = std::min(fromPos.col, linePosR);

        // 琛岄鍧愭爣
        ImVec2 linePos(windowPos.x, windowPos.y + m_charHeight * fromPos.row - scrollY);
        // 缁樺埗鎵?閫夊璞＄殑閫変腑鐘舵?佺煩褰?
        ImVec2 selectStartPos(linePos.x + linePosL * m_charWidth - scrollX, linePos.y);
        ImVec2 selectEndPos(linePos.x + linePosR * m_charWidth - scrollX, linePos.y + m_charHeight);
        drawList->AddRectFilled(selectStartPos, ImVec2(selectEndPos.x + enterCharOffset, selectEndPos.y), IM_COL32(100, 100, 0, 255));

        // 濡傛灉鏄? B鍦ㄥ墠锛孉鍦ㄥ悗锛屽垯鍦ㄦ枃鏈紑濮嬪闂儊
        if (selection.flickerAtFront) flickerPos = selectStartPos;

        // 缁樺埗涓棿琛?
        for (int i = fromPos.row + 1; i < toPos.row; ++i)
        {
            // 琛岄鍧愭爣
            linePos = ImVec2(windowPos.x, windowPos.y + m_charHeight * i - scrollY);
            // 缁樺埗鎵?閫夊璞＄殑閫変腑鐘舵?佺煩褰?
            selectStartPos = ImVec2(linePos.x - scrollX, linePos.y);
            selectEndPos = ImVec2(linePos.x + lines[i].length() * m_charWidth - scrollX, linePos.y + m_charHeight);
            drawList->AddRectFilled(selectStartPos, ImVec2(selectEndPos.x + enterCharOffset, selectEndPos.y), IM_COL32(100, 100, 0, 255));
        }

        // 缁樺埗灏捐锛岀‘瀹氬熬琛屽瓧绗﹀潗鏍?
        linePosL = 0;
        linePosR = std::min(toPos.col, (int)lines[toPos.row].length());

        // 琛岄鍧愭爣
        linePos = ImVec2(windowPos.x, windowPos.y + m_charHeight * toPos.row - scrollY);
        // 缁樺埗鎵?閫夊璞＄殑閫変腑鐘舵?佺煩褰?
        selectStartPos = ImVec2(linePos.x - linePosL * m_charWidth - scrollX, linePos.y);
        selectEndPos = ImVec2(linePos.x + linePosR * m_charWidth - scrollX, linePos.y + m_charHeight);
        drawList->AddRectFilled(selectStartPos, selectEndPos, IM_COL32(100, 100, 0, 255));

        // 濡傛灉鏄? A鍦ㄥ墠锛孊鍦ㄥ悗锛屽垯鍦ㄦ枃鏈湯灏惧闂儊
        if (!selection.flickerAtFront) flickerPos = ImVec2(selectEndPos.x, selectEndPos.y - m_charHeight);
    }

    // 缁樺埗闂儊鏉? 姣忕閽熼棯鐑佷竴娆?
    if (fmod(ImGui::GetTime() - m_flickerDt, 1.0f) < 0.5f)
        drawList->AddLine(flickerPos, ImVec2(flickerPos.x, flickerPos.y + m_charHeight), IM_COL32(255, 255, 0, 255), 1.0f);
}

void NXGUICodeEditor::SelectionsOverlayCheckForMouseEvent(bool bIsDoubleClick)
{
    // 妫?娴嬬殑 m_selections 鐨勬墍鏈夊厓绱狅紙涓嬮潰绉颁负 selection锛夌殑瑕嗙洊鑼冨洿鏄惁鍜屽綋鍓? { m_activeSelectionDown, m_activeSelectionMove } 閲嶅彔
    // 1. 濡傛灉 activeSelection 鏄綋鍓? selection 鐨勭埗闆嗭紝鍒欏皢 selection 鍒犻櫎
    // 2. 濡傛灉 activeSelection 鏄綋鍓? selection 鐨勫瓙闆嗭紝鍒欏皢 selection 鍒犻櫎锛屼絾灏? activeSelectionDown 绉昏嚦 selection.L
    // 3. 濡傛灉 activeSelection 涓嶆槸 selection 鐨勫瓙闆嗭紝浣嗗拰 selection.L 鐩镐氦锛屽垯灏? selection 鍒犻櫎锛涘鏋滄槸鍙屽嚮浜嬩欢Check锛屽皢 activeSelectionDown 绉昏嚦 selection.R
    // 4. 濡傛灉 activeSelection 涓嶆槸 selection 鐨勫瓙闆嗭紝浣嗗拰 selection.R 鐩镐氦锛屽垯灏? selection 鍒犻櫎锛涘鏋滄槸鍙屽嚮浜嬩欢Check锛屽皢 activeSelectionMove 绉昏嚦 selection.L

    std::erase_if(m_selections, [this, bIsDoubleClick](const SelectionInfo& selection)
        {
            SelectionInfo activeSelection(m_activeSelectionDown, m_activeSelectionMove);
            if (activeSelection.Include(selection)) // rule 1. 
                return true;
            else if (selection.Include(activeSelection)) // rule 2.
            {
                m_activeSelectionDown = selection.L;
                return true;
            }
            else if (activeSelection.Include(selection.L)) // rule 3.
            {
                if (bIsDoubleClick) m_activeSelectionMove = selection.R;
                return true;
            }
            else if (activeSelection.Include(selection.R)) // rule 4.
            {
                if (bIsDoubleClick) m_activeSelectionDown = selection.L;
                return true;
            }
            return false;
        });
}

void NXGUICodeEditor::SelectionsOverlayCheckForKeyEvent(bool bFlickerAtFront)
{
    // 妫?娴嬮敭鐩樿緭鍏ヤ簨浠跺鑷碨election鍙戠敓鍙樺寲浠ュ悗鏄惁鐩镐簰閲嶅彔锛?
    // 濡傛灉涓や釜 selection 閲嶅彔锛屽悎骞舵垚涓?涓?傞噸鍙犱互鍚庣殑 鏂皊election 鏄惁 flickerAtFront 鐢遍敭鐩樹簨浠剁殑绫诲瀷鍐冲畾銆?

    m_overlaySelectCheck.clear();
    m_overlaySelectCheck.reserve(m_selections.size() * 2);
    for (const auto& selection : m_selections)
    {
        m_overlaySelectCheck.push_back({ selection.L, true, selection.flickerAtFront });
        m_overlaySelectCheck.push_back({ selection.R, false, selection.flickerAtFront });

        // 鎺掑簭鍓嶅皢鍙冲潗鏍囧悗绉讳竴鏍硷紝纭繚灏? sel1.R=sel2.L 杩欑鍧愭爣鐩稿悓浣嗕笉鐩镐氦鐨勬儏鍐典篃瑙嗕綔閲嶅彔銆?
        m_overlaySelectCheck.back().value.col++;
    }
    std::sort(m_overlaySelectCheck.begin(), m_overlaySelectCheck.end(), [](const SignedCoordinate& a, const SignedCoordinate& b) { return a.value < b.value; });

    // 鎺掑畬搴忛渶瑕佸啀绉诲姩鍥炴潵
    for (auto& overlayCheck : m_overlaySelectCheck)
        if (!overlayCheck.isLeft) overlayCheck.value.col--;

    Coordinate left, right;
    int leftSignCount = 0; // 閬囧乏+1锛岄亣鍙?-1銆?
    bool overlayed = false; // 褰? leftSignCount >= 2 鏃讹紝璇存槑鏈夐噸鍙犱骇鐢燂紝姝ゆ椂浠? overlayed = true

    // 妫?娴嬮噸鍙狅紝濡傛灉鏈夐噸鍙犵殑selection锛屽幓鎺?
    m_selections.clear();
    for (auto& coord : m_overlaySelectCheck)
    {
        if (coord.isLeft)
        {
            if (leftSignCount == 0)
            {
                left = coord.value;
            }
            else if (leftSignCount > 0) overlayed = true; // 妫?娴嬫槸鍚﹀嚭鐜颁簡閲嶅彔
            leftSignCount++;
        }
        else
        {
            leftSignCount--;
            if (leftSignCount == 0)
            {
                right = coord.value;
                if (overlayed)
                {
                    // 濡傛灉鍑虹幇閲嶅彔锛屽熀浜庡綋鍓嶉敭鐩樹簨浠跺垽鏂? flickerAtFront
                    m_selections.push_back(bFlickerAtFront ? SelectionInfo(right, left) : SelectionInfo(left, right));
                }
                else
                {
                    // 濡傛灉娌℃湁鍑虹幇閲嶅彔锛屼繚鐣欎箣鍓嶇殑 flickerAtFront
                    m_selections.push_back(coord.flickerAtFront ? SelectionInfo(right, left) : SelectionInfo(left, right));
                }
                overlayed = false;
            }
        }
    }
}

void NXGUICodeEditor::ScrollCheck()
{
    if (m_selections.empty())
        return;

    // 姝ゆ柟娉曡礋璐ｅ綋灞忓箷澶栨湁 selection 閫夊彇鍙樺寲鏃讹紝璺宠浆鍒拌浣嶇疆銆?
    // 2023.7.11 鏆傝烦杞埌 m_selections.back() 鎵?鍦ㄧ殑浣嶇疆銆?
    // 灞忓箷鍐呭瓨鍦ㄩ?夊尯鐨勬儏鍐典笅锛屾绛栫暐鏈夊緢鏄庢樉鐨勬墜鎰熼棶棰橈紝鏈夊緟浼樺寲銆?
    const auto& lastSelection = m_selections.back();
    int lastRow = lastSelection.flickerAtFront ? lastSelection.L.row : lastSelection.R.row;
    int lastCol = lastSelection.flickerAtFront ? lastSelection.L.col : lastSelection.R.col;

    // 濡傛灉瓒呭嚭绐楀彛杈圭晫锛宻crollY
    float scrollY = ImGui::GetScrollY();
    float scrollMaxY = ImGui::GetScrollMaxY();
    float contentAreaHeight = ImGui::GetContentRegionAvail().y;
    float newSelectHeight = (float)lastRow * m_charHeight;
    if (newSelectHeight < scrollY)
    {
        ImGui::SetScrollY(newSelectHeight);
    }
    else if (newSelectHeight > scrollY + contentAreaHeight - m_charHeight)
    {
        ImGui::SetScrollY(std::min(newSelectHeight - contentAreaHeight + m_charHeight, scrollMaxY));
    }

    // 濡傛灉瓒呭嚭绐楀彛杈圭晫锛宻crollX
    float scrollX = ImGui::GetScrollX();
    float scrollMaxX = ImGui::GetScrollMaxX();
    float contentAreaWidth = ImGui::GetContentRegionAvail().x;
    float newSelectWidth = (float)lastCol * m_charWidth;
    if (newSelectWidth < scrollX)
    {
        ImGui::SetScrollX(newSelectWidth);
    }
    else if (newSelectWidth > scrollX + contentAreaWidth)
    {
        ImGui::SetScrollX(std::min(newSelectWidth - contentAreaWidth + 2 * m_charWidth, scrollMaxX));
    }
}

int NXGUICodeEditor::CalcSelectionLength(const SelectionInfo& selection)
{
    const auto& L = selection.L;
    const auto& R = selection.R;
    auto& lines = m_textFiles[m_pickingIndex].lines;

    if (L.row == R.row)
    {
        auto& line = lines[L.row];
        int actualLcol = std::min(L.col, (int)line.size());
        int actualRcol = std::min(R.col, (int)line.size());
        return actualRcol - actualLcol;
    }
    else
    {
        auto& lineL = lines[L.row];
        auto& lineR = lines[R.row];
        int actualLcol = std::min(L.col, (int)lineL.size());
        int actualRcol = std::min(R.col, (int)lineR.size());
        int length = (int)lineL.size() - actualLcol + actualRcol;
        for (int i = L.row + 1; i < R.row - 1; i++)
            length += (int)lines[i].size();
        return length;
    }
}

void NXGUICodeEditor::RenderTexts_OnMouseInputs()
{
    if (!ImGui::IsWindowFocused())
        return;

    auto& io = ImGui::GetIO();

    float scrollX = ImGui::GetScrollX();
    float scrollY = ImGui::GetScrollY();

    const auto& mousePos = ImGui::GetMousePos();

    // 鑾峰彇榧犳爣鍦ㄦ枃鏈樉绀哄尯涓殑鐩稿浣嶇疆
    const auto& windowPos = ImGui::GetWindowPos();
    const ImVec2 relativeWindowPos(mousePos.x - windowPos.x, mousePos.y - windowPos.y);
    const ImVec2 relativePos(scrollX + relativeWindowPos.x, scrollY + relativeWindowPos.y);

    // 鑾峰彇鏂囨湰鍐呭鍖哄煙鐨勭煩褰?
    const ImVec2& contentAreaMin(windowPos);
    const ImVec2& textContentArea(ImGui::GetContentRegionAvail());
    const ImVec2 contentAreaMax(contentAreaMin.x + textContentArea.x, contentAreaMin.y + textContentArea.y);

    // 妫?鏌ラ紶鏍囩偣鍑绘槸鍚﹀湪鏂囨湰鍐呭鍖哄煙鍐?
    bool isMouseInContentArea = mousePos.x >= contentAreaMin.x && mousePos.x <= contentAreaMax.x &&
        mousePos.y >= contentAreaMin.y && mousePos.y <= contentAreaMax.y;

    auto& lines = m_textFiles[m_pickingIndex].lines;

    if (isMouseInContentArea && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    {
        // 璁＄畻鍑鸿鍒楀彿
        int row = (int)(relativePos.y / m_charHeight);
        float fCol = relativePos.x / m_charWidth;

        // 鎵嬫劅浼樺寲锛氬疄闄呯偣鍑讳綅缃殑 鍒楀潗鏍? 瓒呰繃褰撳墠瀛楃鐨?50% 鏃讹紝璁や负鏄笅涓?涓瓧绗?
        int col = (int)(fCol + 0.5f);

        // 绾︽潫琛屽垪鍙疯寖鍥?
        row = std::max(0, std::min(row, (int)lines.size() - 1));
        col = std::max(0, std::min(col, (int)lines[row].size()));

        // 鎶撳彇瀵瑰簲瀛楃
        if (col < lines[row].size())
        {
            // 浠庡綋鍓嶄綅缃紑濮嬪悜宸﹀彸鎵弿锛岀洿鍒伴亣鍒扮┖鏍兼垨鑰呮崲琛岀
            int left = col;
            int right = col;
            
            while (left > 0 && !IsWordSplitChar(lines[row][left]) && lines[row][left] != '\n') left--;
            while (right < lines[row].size() && !IsWordSplitChar(lines[row][right]) && lines[row][right] != '\n') right++;

            if (!io.KeyAlt) ClearSelection();

            m_bIsSelecting = true;
            m_activeSelectionDown = { row, left + 1 };
            m_activeSelectionMove = { row, right };
            SelectionsOverlayCheckForMouseEvent(true);
        }

        m_bResetFlickerDt = true;
    }
    else if (isMouseInContentArea && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        // 璁＄畻鍑鸿鍒楀彿
        int row = (int)(relativePos.y / m_charHeight);
        float fCol = relativePos.x / m_charWidth;

        // 鎵嬫劅浼樺寲锛氬疄闄呯偣鍑讳綅缃殑 鍒楀潗鏍? 瓒呰繃褰撳墠瀛楃鐨?50% 鏃讹紝璁や负鏄笅涓?涓瓧绗?
        int col = (int)(fCol + 0.5f);

        // 绾︽潫琛屽垪鍙疯寖鍥?
        row = std::max(0, std::min(row, (int)lines.size() - 1));
        col = std::max(0, std::min(col, (int)lines[row].size()));

        // test: 鎵撳嵃瀵瑰簲瀛楃
        //if (col < lines[row].size()) std::cout << lines[row][col];

        if (!io.KeyAlt) ClearSelection();

        m_bIsSelecting = true;
        for (const auto& selection : m_selections)
        {
            const Coordinate& pos = { row, col };
            if (selection.Include(pos))
            {
                m_bIsSelecting = false;
				if (selection == pos) RemoveSelection(selection);
                break;
            }
        }

        if (m_bIsSelecting)
        {
            m_activeSelectionDown = { row, col };
            m_activeSelectionMove = { row, col };
        }

        m_bResetFlickerDt = true;
    }
    else if (m_bIsSelecting && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
    {
        // 澶勭悊榧犳爣鎷栨嫿
        // 璁＄畻鍑鸿鍒楀彿
        int row = (int)(relativePos.y / m_charHeight);
        float fCol = relativePos.x / m_charWidth;

        // 鎵嬫劅浼樺寲锛氬疄闄呯偣鍑讳綅缃殑 鍒楀潗鏍? 瓒呰繃褰撳墠瀛楃鐨?50% 鏃讹紝璁や负鏄笅涓?涓瓧绗?
        int col = (int)(fCol + 0.5f);

        // 绾︽潫琛屽垪鍙疯寖鍥?
        row = std::max(0, std::min(row, (int)lines.size() - 1));
        col = std::max(0, std::min(col, (int)lines[row].size()));

		m_activeSelectionMove = { row, col };
        SelectionsOverlayCheckForMouseEvent(false);

        m_bResetFlickerDt = true;

        // 瓒呭嚭褰撳墠鏄剧ず鑼冨洿鏃? scrollX/Y
        const ImVec2 dragSpeed(m_charWidth * 4.0f, m_charHeight * 2.0f);
        const auto& contentArea(ImGui::GetContentRegionAvail());
        const ImVec2 scrollMax(ImGui::GetScrollMaxX(), ImGui::GetScrollMaxY());
        float scrollBarWidth = scrollMax.x > 0.0f ? ImGui::GetStyle().ScrollbarSize : 0.0f;
        float scrollBarHeight = scrollMax.y > 0.0f ? ImGui::GetStyle().ScrollbarSize : 0.0f;

        if (relativeWindowPos.x < 0)
            ImGui::SetScrollX(std::max(scrollX - dragSpeed.x, 0.0f));
        else if (relativeWindowPos.x > contentArea.x + scrollBarWidth)
        {
            // 褰? scrollX 鍙崇Щ鏃讹紝涓嶅噯瓒呰繃褰撳墠榧犳爣鎵?鍦ㄨ鐨勬渶澶ф枃鏈暱搴? + 50.0 鍍忕礌
            float scrollXLimit = lines[row].size() * m_charWidth - contentArea.x + 50.0f;
            scrollXLimit = std::min(scrollXLimit, scrollMax.x);
            ImGui::SetScrollX(std::min(scrollX + dragSpeed.x, scrollXLimit));
        }

        if (relativeWindowPos.y < 0)
            ImGui::SetScrollY(std::max(scrollY - dragSpeed.y, 0.0f));
        else if (relativeWindowPos.y > contentArea.y + scrollBarHeight)
            ImGui::SetScrollY(std::min(scrollY + dragSpeed.y, scrollMax.y));
    }
}

void NXGUICodeEditor::Render_OnMouseInputs()
{
    if (m_bIsSelecting && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
    {
        m_bIsSelecting = false;
        AddSelection(m_activeSelectionDown, m_activeSelectionMove);
    }
}

void NXGUICodeEditor::RenderTexts_OnKeyInputs()
{
    ImGuiIO& io = ImGui::GetIO();
	bool bAlt = io.KeyAlt;
	bool bShift = io.KeyShift;
	bool bCtrl = io.KeyCtrl;

    if (ImGui::IsWindowFocused())
    {
        io.WantCaptureKeyboard = true;
        io.WantTextInput = true;

        bool bKeyUpPressed = ImGui::IsKeyPressed(ImGuiKey_UpArrow);
        bool bKeyDownPressed = ImGui::IsKeyPressed(ImGuiKey_DownArrow);
        bool bKeyLeftPressed = ImGui::IsKeyPressed(ImGuiKey_LeftArrow);
        bool bKeyRightPressed = ImGui::IsKeyPressed(ImGuiKey_RightArrow);
        bool bKeyHomePressed = ImGui::IsKeyPressed(ImGuiKey_Home);
        bool bKeyEndPressed = ImGui::IsKeyPressed(ImGuiKey_End);
        bool bKeyPageUpPressed = ImGui::IsKeyPressed(ImGuiKey_PageUp);
        bool bKeyPageDownPressed = ImGui::IsKeyPressed(ImGuiKey_PageDown);
        bool bDeletePressed = ImGui::IsKeyPressed(ImGuiKey_Delete);
        bool bBackspacePressed = ImGui::IsKeyPressed(ImGuiKey_Backspace);
        bool bEnterPressed = ImGui::IsKeyPressed(ImGuiKey_Enter);
        bool bEscPressed = ImGui::IsKeyPressed(ImGuiKey_Escape);

        bool bSelectAllCommand = bCtrl && ImGui::IsKeyPressed(ImGuiKey_A);
        bool bCopyCommand = bCtrl && ImGui::IsKeyPressed(ImGuiKey_C);
        bool bCutCommand = bCtrl && ImGui::IsKeyPressed(ImGuiKey_X);
        bool bPasteCommand = bCtrl && ImGui::IsKeyPressed(ImGuiKey_V);

        bool bCtrlHomePressed = bCtrl && bKeyHomePressed;
        bool bCtrlEndPressed = bCtrl && bKeyEndPressed;
        if (!bAlt && (bKeyUpPressed || bKeyPageUpPressed || bCtrlHomePressed))
        {
            MoveUp(bShift, bKeyPageUpPressed, bCtrlHomePressed);
            m_bResetFlickerDt = true;
        }

        else if (!bAlt && (bKeyDownPressed || bKeyPageDownPressed || bCtrlEndPressed))
        {
            MoveDown(bShift, bKeyPageDownPressed, bCtrlEndPressed);
            m_bResetFlickerDt = true;
        }

        else if (!bAlt && (bKeyLeftPressed || bKeyHomePressed))
        {
            MoveLeft(bShift, bCtrl, bKeyHomePressed, 1);
            m_bResetFlickerDt = true;
        }

        else if (!bAlt && (bKeyRightPressed || bKeyEndPressed))
        {
            MoveRight(bShift, bCtrl, bKeyEndPressed, 1);
            m_bResetFlickerDt = true;
        }

        else if (bDeletePressed || bBackspacePressed)
        {
            Backspace(bDeletePressed, bCtrl);
            m_bResetFlickerDt = true;
        }

        else if (bEnterPressed)
        {
            Enter({ {""}, {""} });
            m_bResetFlickerDt = true;
        }

        else if (bEscPressed)
        {
            Escape();
            m_bResetFlickerDt = true;
        }

        else if (bCopyCommand)
        {
            Copy();
            m_bResetFlickerDt = true;
        }

        else if (bCutCommand)
        {
            Cut();
            m_bResetFlickerDt = true;
        }

        else if (bPasteCommand)
        {
            Paste();
            m_bResetFlickerDt = true;
        }

        else if (bSelectAllCommand)
        {
            SelectAll();
            m_bResetFlickerDt = true;
        }

        if (!io.InputQueueCharacters.empty() && !bCtrl && !bAlt)
        {
            for (const auto& wc : io.InputQueueCharacters)
            {
                auto c = static_cast<char>(wc);
                if (wc >= 32 && wc < 127)
                {
                    Enter({ {{c}} });
                    //Enter({ {"If it looks like food,", "it is not good food", "--senpai810"}, {"114 514", "1919810", "feichangdexinxian", "SOGOKUOISHII", "Desu!"} });
                    m_bResetFlickerDt = true;
                }
                else if (wc == '\t') // tab
                {
                    Enter({ { "    " }}); // 鏆傛椂鍏堝己鍒惰浆鎹㈡垚4涓┖鏍硷紝搴斾粯鏃ュ父瓒冲浜?
                    m_bResetFlickerDt = true;
                }
            }

            io.InputQueueCharacters.resize(0);
        }
    }
}

void NXGUICodeEditor::MoveUp(bool bShift, bool bPageUp, bool bCtrlHome)
{
    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        bCtrlHome ? pos.row = -1 : bPageUp ? pos.row -= 20 : pos.row--;
        if (pos.row < 0)
        {
            pos.row = 0;
            pos.col = 0;
        }

        if (!bShift)
            sel.flickerAtFront ? sel.R = pos : sel.L = pos;
        else
        {
            if (sel.R < sel.L)
            {
                std::swap(sel.L, sel.R);
                sel.flickerAtFront = true;
            }
        }
    }

    SelectionsOverlayCheckForKeyEvent(true);
    ScrollCheck();
}

void NXGUICodeEditor::MoveDown(bool bShift, bool bPageDown, bool bCtrlEnd)
{
    auto& lines = m_textFiles[m_pickingIndex].lines;

    for (auto& sel : m_selections)
    {
        auto& pos = sel.flickerAtFront ? sel.L : sel.R;
        bCtrlEnd ? pos.row = (int)lines.size() : bPageDown ? pos.row += 20 : pos.row++;
        if (pos.row >= lines.size())
        {
            pos.row = (int)lines.size() - 1;
            pos.col = (int)lines[pos.row].size();
        }

        if (!bShift)
            sel.flickerAtFront ? sel.R = pos : sel.L = pos;
        else
        {
            if (sel.L > sel.R)
            {
                std::swap(sel.L, sel.R);
                sel.flickerAtFront = false;
            }
        }
    }

    SelectionsOverlayCheckForKeyEvent(false);
    ScrollCheck();
}

void NXGUICodeEditor::MoveLeft(bool bShift, bool bCtrl, bool bHome, int size)
{
    for (auto& sel : m_selections) MoveLeft(sel, bShift, bCtrl, bHome, size);
    SelectionsOverlayCheckForKeyEvent(true);
    ScrollCheck();
}

void NXGUICodeEditor::MoveRight(bool bShift, bool bCtrl, bool bEnd, int size)
{
    for (auto& sel : m_selections) MoveRight(sel, bShift, bCtrl, bEnd, size);
    SelectionsOverlayCheckForKeyEvent(false);
    ScrollCheck();
}

void NXGUICodeEditor::MoveLeft(SelectionInfo& sel, bool bShift, bool bCtrl, bool bHome, int size)
{
    auto& lines = m_textFiles[m_pickingIndex].lines;
    auto& pos = sel.flickerAtFront ? sel.L : sel.R;
    pos.col = std::min(pos.col, (int)lines[pos.row].size());
    if (bHome)
    {
        // 鎸塇ome 鍏堣烦鍒版湰琛岀涓?涓潪绌虹櫧瀛楃锛?
        int oldCol = pos.col;
        pos.col = 0;
        while(pos.col < lines[pos.row].size() && std::isspace(lines[pos.row][pos.col])) pos.col++;

        // 鑻ュ凡鍦ㄧ涓?涓潪绌虹櫧瀛楃锛屽垯璺冲埌琛岄
        if (oldCol == pos.col) pos.col = 0;
    }
    else
    {
        while (size) // 鎸佺画宸︾Щ锛岀洿鍒? size 鑰楀敖
        {
            if (pos.col > 0)
            {
                int newCol = std::max(pos.col - size, 0);
                size -= pos.col - newCol;
                pos.col = newCol;

                if (bCtrl)
                {
                    while (pos.col > 0 && !IsVariableChar(lines[pos.row][pos.col])) pos.col--;
                    if (pos.col > 0)
                    {
                        while (pos.col > 0 && IsVariableChar(lines[pos.row][pos.col])) pos.col--;
                        pos.col++;
                    }
                }
            }
            else if (pos.row > 0)
            {
                pos.row--;
                pos.col = (int)lines[pos.row].size();
                size--;
            }
            else break; // 濡傛灉宸插埌杈惧叏鏂囧紑澶达紝鍒欏乏绉荤粓姝?
        }
    }

    if (!bShift)
        sel.flickerAtFront ? sel.R = pos : sel.L = pos;
    else
    {
        if (sel.R < sel.L)
        {
            std::swap(sel.L, sel.R);
            sel.flickerAtFront = true;
        }
    }
}

void NXGUICodeEditor::MoveRight(SelectionInfo& sel, bool bShift, bool bCtrl, bool bEnd, int size)
{
    auto& lines = m_textFiles[m_pickingIndex].lines;
    auto& pos = sel.flickerAtFront ? sel.L : sel.R;
    pos.col = std::min(pos.col, (int)lines[pos.row].size());
    if (bEnd) pos.col = (int)lines[pos.row].size();
    else
    {
        while (size) // 鎸佺画鍙崇Щ锛岀洿鍒? size 鑰楀敖
        {
            if (pos.col < lines[pos.row].size())
            {
                int newCol = std::min(pos.col + size, (int)lines[pos.row].size());
                size -= newCol - pos.col;
                pos.col = newCol;

                if (bCtrl)
                {
                    while (pos.col < lines[pos.row].size() && !IsVariableChar(lines[pos.row][pos.col])) pos.col++;
                    if (pos.col < lines[pos.row].size())
                    {
                        while (pos.col < lines[pos.row].size() && IsVariableChar(lines[pos.row][pos.col])) pos.col++;
                    }
                }
            }
            else if (pos.row < lines.size() - 1)
            {
                pos.row++;
                pos.col = 0;
                size--;
            }
            else break; // 濡傛灉宸插埌杈惧叏鏂囨湯灏撅紝鍒欏彸绉荤粓姝?
        }
    }

    if (!bShift)
        sel.flickerAtFront ? sel.R = pos : sel.L = pos;
    else
    {
        if (sel.L > sel.R)
        {
            std::swap(sel.L, sel.R);
            sel.flickerAtFront = false;
        }
    }
}

bool NXGUICodeEditor::IsVariableChar(const char& ch)
{
    // ctrl+left/right 鍙互璺宠繃鐨勫瓧绗︼細
    return std::isalnum(ch) || ch == '_';
}

bool NXGUICodeEditor::IsWordSplitChar(const char& ch)
{
    // ctrl+delete/backspace 鍒犻櫎澶氫釜瀛楃鏃讹紝閬囧埌涓嬪垪瀛楃涔嬩竴鍗崇粓姝?
    return std::isspace(ch) || ch == ',' || ch == ';' || ch == ':' || ch == '.' || ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '{' || ch == '}' || ch == '<' || ch == '>';
}

std::vector<NXGUICodeEditor::TextKeyword> NXGUICodeEditor::ExtractKeywords(const TextString& text)
{
    std::vector<NXGUICodeEditor::TextKeyword> words;
    std::string word;
    int i;
    for (i = 0; i < text.length(); i++)
    {
        const unsigned char& c = text[i];
        // 瀵逛簬瀛楁瘝鎴栨暟瀛楃殑瀛楃锛屽皢鍏舵坊鍔犲埌褰撳墠鍗曡瘝
        if (std::isalnum(c) || c == '_') word += c;
        else if (!word.empty())
        {
            // 瀵逛簬闈炲瓧姣嶆垨鏁板瓧鐨勫瓧绗︼紝濡傛灉褰撳墠鍗曡瘝涓嶄负绌猴紝鍒欐坊鍔犲埌words鍒楄〃涓紝骞舵竻绌哄綋鍓嶅崟璇?
            words.push_back({ word, i - (int)word.length() });
            word.clear();
        }

        // 娉ㄩ噴妫?娴?
        if (c == '/' && i + 1 < text.length())
        {
            if (text[i + 1] == '/')
            {
                words.push_back({ "//", i, 0 });
                word.clear();
                return words;
            }
        }
    }

    // 濡傛灉鏈?鍚庝竴涓崟璇嶆病鏈夎娣诲姞鍒皐ords鍒楄〃涓紝鍒欓渶瑕佸湪寰幆缁撴潫鍚庡啀娣诲姞涓?娆?
    if (!word.empty()) words.push_back({ word, i - (int)word.length() });
    return words;
}

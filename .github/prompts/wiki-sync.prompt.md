---
description: "一键完整同步：审查所有 wiki 文档与代码的差异，然后自动更新过时的文档。"
mode: "agent"
tools: ["semantic_search", "grep_search", "file_search", "read_file", "list_dir", "replace_string_in_file", "create_file", "multi_replace_string_in_file", "run_in_terminal"]
---

# Wiki 完整同步任务

你是 NIX 项目的文档维护系统。执行完整的 wiki 同步流程：先审查差异，再自动更新。

## 编码规范

所有创建或修改的 md 文件必须为 **UTF-8（无 BOM）** 编码。
使用 `create_file` 写入**新文件**后，**必须**在终端执行以下 PowerShell 命令验证并修复编码：

```powershell
$f = "文件路径"; $b = [IO.File]::ReadAllBytes($f); try { [Text.UTF8Encoding]::new($false,$true).GetString($b) | Out-Null } catch { $t = [Text.Encoding]::GetEncoding("GB2312").GetString($b); [IO.File]::WriteAllBytes($f, [Text.UTF8Encoding]::new($false).GetBytes($t)); Write-Output "Fixed: $f" }
```

注：`replace_string_in_file` 编辑已有 UTF-8 文件时会保留编码，无需额外处理。
建议在任务结束前，对所有本次修改的文件执行一次批量编码检查。

## 完整流程

### 阶段一：审查（Audit）

1. 读取 `NIX Wiki/_index.md`
2. 对每篇已有 wiki 文档，读取其内容和关联代码的头文件
3. 比对文档与代码的结构性差异（类名、继承、函数签名、核心流程）
4. 分类：
   - **current** — 无需更新
   - **needs-update** — 有差异，可自动更新
   - **outdated** — 严重过时或引用已删代码
5. 将审查结果写入 `NIX Wiki/_audit_report.md`

### 阶段二：更新（Update）

对所有标记为 needs-update 或 outdated 的文档，按优先级自动更新：

1. 读取关联代码
2. 更新 wiki 文档内容（保留原始风格，修正不准确处，补充新内容）
3. 更新 `_index.md` 中的同步日期和状态
4. 在 `_changelog.md` 追加变更记录

### 阶段三：覆盖度检查

1. 扫描代码中所有主要子系统（通过头文件）
2. 与 `_index.md` 的"未覆盖子系统"表对比
3. 如果发现新的未覆盖子系统，添加到列表中
4. 不自动为未覆盖子系统创建文档（除非用户明确要求）

### 阶段四：报告

输出一个简洁的同步摘要：
- 审查了多少篇文档
- 更新了多少篇
- 发现多少新的未覆盖子系统
- 有哪些文档仍有不确定之处（标记了 TODO）

## 重要规则

- **绝不修改 C++/HLSL 代码**，只修改 `NIX Wiki/` 下的文件
- 保留 wiki 文档的原始组织风格
- 不确定的内容用 `<!-- TODO: 待确认 -->` 标记
- 每次更新伴随 `_changelog.md` 记录
- 不自动创建新文档，只更新已有文档和索引
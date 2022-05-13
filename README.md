# Nix

自己写的 DX11 渲染器。

目前支持的功能：

- [x] FBX 模型导入（目前仅支持静态模型，而且可能有点问题）
- [x] PBR
- [x] IBL光照
  - [x] Diffuse IBL：三阶SH近似
  - [x] Specular IBL：PreFilter Map
- [x] Depth Peeling

- [ ] Shadow Map （折腾中...）

在计划中，但还没开始做的：
- 渲染
  - TAA
  - SSAO 升级
  - SSR
- UI
  - 材质界面增加 "Unique"（或者detach更贴切?）

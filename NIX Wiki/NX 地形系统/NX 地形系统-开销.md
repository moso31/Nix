此模块用于地形系统渲染。
核心技术基本就是FC5的那套论文。

**优势是显存和性能。显存占用非常低。对16kmx16km的大世界级别地形，显存可控制在200M以内；性能大小<1ms的开销。并且二者都有进一步的优化空间。**

# 显存开销

## 纹理资源

| 名称                       | 尺寸                   | 格式             | 大小            |
| ------------------------ | -------------------- | -------------- | ------------- |
| **Atlas 纹理**             |                      |                |               |
| HeightMapAtlas           | 65x65x1024           | R16_UNORM      | 16.13 MB      |
| SplatMapAtlas            | 65x65x1024           | R8_UNORM       | 16.06 MB      |
| NormalMapAtlas           | 65x65x1024           | R8G8B8A8_UNORM | 32.26 MB      |
| Sector2NodeID            | 256x256, 6mips       | R16_UINT       | 0.19 MB       |
| **Atlas 小计**             |                      |                | **64.64 MB**  |
| **GBuffer 材质纹理**         |                      |                |               |
| txTerrainBaseColor       | 1024x1024x49, 11mips | BC7_UNORM_SRGB | 65.49 MB      |
| txTerrainNormalMap       | 1024x1024x49, 11mips | BC7_UNORM      | 65.49 MB      |
| **GBuffer 小计**           |                      |                | **130.98 MB** |
| **临时加载纹理（每帧最多4个）**       |                      |                |               |
| Terrain_HeightMap_0~3 x4 | 65x65                | R16_UNORM      | 0.06 MB       |
| Terrain_SplatMap_0~3 x4  | 65x65                | R8_UNORM       | 0.06 MB       |
| Terrain_NormalMap_0~3 x4 | 65x65                | BC3_UNORM      | 0.03 MB       |
| **临时小计**                 |                      |                | **~0.16 MB**  |
## Buffer 资源

| 名称                    | Stride | Count | 大小           |
| --------------------- | ------ | ----- | ------------ |
| **Ping-pong Buffers** |        |       |              |
| NodeId Array A x3     | 4      | 1024  | 12 KB        |
| NodeId Array B x3     | 4      | 1024  | 12 KB        |
| NodeId Array Final x3 | 4      | 1024  | 12 KB        |
| Indirect Args         | 12     | 1     | ~36 bytes    |
| **Patcher Buffers**   |        |       |              |
| Patcher Buffer x3     | 32     | 32768 | 3.0 MB       |
| Draw Index Args x3    | 20     | 1     | ~60 bytes    |
| Draw Index Args Zero  | 20     | 1     | 20 bytes     |
| Counter Buffers       | 4      | 各1    | ~数十 bytes    |
| **Buffer 小计**         |        |       | **~3.04 MB** |
## 总计

|项目|大小|
|---|---|
|Atlas 纹理|64.64 MB|
|GBuffer 材质纹理|130.98 MB|
|GPU Buffers|3.04 MB|
|临时加载纹理（峰值）|0.16 MB|
|**总显存开销**|**~198.8 MB**|
**说明**：
- GBuffer 材质纹理 (`txTerrainBaseColor`, `txTerrainNormalMap`) 是 49 层的 Texture2DArray，用于地形着色时的材质混合
- 临时加载纹理是流式加载过程中的中间纹理，加载完成后数据被拷贝到 Atlas，纹理本身会被释放
- Buffer 存在 Frame Resource（x3），用于 GPU/CPU 帧间同步

# 开销评估

以目前结果而言，一般认为能控制在<0.25ms量级；
一帧RenderDoc的截图如下。截帧数据显示约0.5ms（120帧），但考虑到实际运行时的开销高于240帧

![[Pasted image 20260109114537.png|489x632]]
# 地形效果

![[Pasted image 20260109100802.png|910x409]]


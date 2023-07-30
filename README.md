# Nix

Selfmade DX11 renderer.

## What it can do
- [x] FBX import（For static models only and still have many bugs fot now）
- [x] PBR
- [x] IBL lighting
  - [x] Diffuse IBL (Spherical Harmonics)
  - [x] Specular IBL：PreFilter Map
- [x] Depth Peeling
- [x] Shadow Map (PCF only)
- [x] A very simple Tone-Mapping
- [x] Programmable material
  - [x] Used this HLSL code editor: https://github.com/moso31/DonoText

## Intending...
- [ ] SSS
  - [ ] Screen-Space SSSS
  - [ ] Separable SSS
  - [ ] Burley SSS
  - [ ] PreIntegrated SSS

- [ ] Anti-Aliasing(TAA SMAA FXAA...)
- [ ] Screen Space Ambient Occlusion(HBAO GTAO...)
- [ ] SSR
- [ ] Post-Processing

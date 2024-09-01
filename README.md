# Nix

Selfmade DX12 renderer.

## Features
### Standard PBR
> Deferrer pipeline + Diffuse IBL (Spherical Harmonics) + PreFilter Map
### Depth Peeling
### A very simple Tone-Mapping
### Cascade Shadow Map (PCF only)
![image](https://github.com/moso31/Nix/assets/15684115/f3b9d70c-ebb5-4c7e-aa0f-339e62c78115)
### Programmable material + HLSL code editor: https://github.com/moso31/DonoText
![image](https://github.com/moso31/Nix/assets/15684115/70bf7f43-61eb-473a-8b7a-5a3fad1230b6)
### Burley SSS + approx SSS BTDF
![image](https://github.com/moso31/Nix/assets/15684115/e08205c6-4929-4d25-8019-bcbb7c6a278e)
![transition_animation](https://github.com/moso31/Nix/assets/15684115/d13bae94-fe1c-4c53-8d0a-be8ed776f075)
### XAllocator

XAllocator is a memory allocator used for Nix DX12 to allocate TextureResources/CBuffers/RTViews/DSViews/Descriptors memory.

Wiki: https://github.com/moso31/Nix/wiki/XAllocator (not finished yet...)

## Intending...
- [ ] Anti-Aliasing(TAA SMAA FXAA...)
- [ ] Screen Space Ambient Occlusion(HBAO GTAO...)
- [ ] SSR
- [ ] A better Post-Processing



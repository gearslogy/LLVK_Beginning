# LLVK_Beginning
Learn Vulkan from scratch
## license of assets
* None of the assets can be used commercially
* Except houdini works commercially

## SSBO RBD animation
Implement RBD animation with vulkan SSBO
![dual.png](screenshot/RBD_SSBO.png)

## RBD VAT

### scene v2
rbd_vat_v2.hip
![dual.png](screenshot/RBD_VAT2.gif)
![dual.png](screenshot/RBD_VAT3.png)

### scene v1
[rbd_vat.hip](build/bin/content/scene/rbdvat/rbd_vat.hip)
![dual.png](screenshot/rbd_vat.gif)
![dual.png](screenshot/RBD_VAT1.png)

![dual.png](screenshot/vat0.png)
![dual.png](screenshot/vat1.gif)

## Dive into Cascade ShadowMap
geometry shader to generate the depth render target
![dual.png](screenshot/csm_1.png)
![dual.png](screenshot/csm_2.gif)
with houdini visualize result: scene/csm/vulkan_csm_visualize.hip
![dual.png](screenshot/csm_3.gif)
![dual.png](screenshot/csm_4.gif)

## DualPass RenderHair
![dual.png](screenshot/dualpass.png)
![dual.png](screenshot/dualpass_rdc.png)
I rewrote a bake system for baking hair occ:
![dual.png](screenshot/dualpass_occ.png)
![dual.png](screenshot/dualpass_realtime_bk.png)


## IndirectDraw 
![instancev2.png](screenshot/indirectDraw_rd.png)
![instancev2.png](screenshot/indirectDraw.png)

indirect_draw vs instance 😊:
* indirect_draw with all 4K map: 80fps
* instance with all 2k map: 80fps



## Instance 
### updated renderer/instance/instance_v2


![instancev2.png](screenshot/instance_v2_02.png)
![instancev2.png](screenshot/instance_v2.png)




### terrain resource gen(houdini20.5):
terrain rendering:

![terrain.png](screenshot/terrain_rendering.png)

1. generate data from houdini

![terrain.png](screenshot/hou_terrain_build.png)

2. four layers:
* cliff1
* cliff2
* rocks
* grass

3: encoded terrain mask:

![shadow.png](build/bin/content/scene/instance/tex/terrain/terrain_masks/mask.png)

* R:cliff1
* G:cliff2
* B:small_rocks
* A:Grass(as background) fully white color



## shadow map
* opacity/foliage objects rendering
* opaque objects rendering

1:generate depth test:
![shadow.png](screenshot/shadow_map_gen_depth.png)
2:using depth and pcf:
![pcf.png](screenshot/pcf.png)

## deferred
![deferred.png](screenshot/deferred.png)
![deferred.png](screenshot/deferred_gen_attachments.png)

## vma memory management 
![vma.png](screenshot%2Fvma.png)
## dynamic ubo
![dynamicUBO_UseGLTF_BasicPBR.png](screenshot%2FdynamicUBO_UseGLTF_BasicPBR.png)
## ktx tex array
![texarray.png](screenshot/texarray.png)


## command tool
### tools\gltf_dump  : gltf info dump
* command interface for dump geometry infos(vertex attributes)
* Clearly determine which object you put first in the gltf primitive, so that you can determine the order of materials
* Note that all gltf objects are discrete triangles,

```
-------------primitive part:0-----------
attrib key:NORMAL value:3
attrib key:POSITION value:1
attrib key:TANGENT value:4
attrib key:TEXCOORD_0 value:2
0 2 1 2 0 3 1 5 4 5 1 2 4 7 6 7 4 5 3 8 2 8 3 9 2 10 5 10 2 8 5 11 7 11 5 10 9 12 8 12 9 13 8 14 10 14 8 12 10 15 11 15 10 14
-------------primitive part:5-----------
attrib key:NORMAL value:8
attrib key:POSITION value:6
attrib key:TANGENT value:9
attrib key:TEXCOORD_0 value:7
0 2 1 2 0 3

```
### exr dump
#### dump single part layer:
.\exr_dump.exe position.exr
```text
Not multi part exr: `chunkCount' attribute is not found in the header., roll back to single part exr reader: EXR version: 2
Number of channels: 4
Channel[0]: name = A, pixel_type = HALF, sampling = (1, 1)
Channel[1]: name = B, pixel_type = HALF, sampling = (1, 1)
Channel[2]: name = G, pixel_type = HALF, sampling = (1, 1)
Channel[3]: name = R, pixel_type = HALF, sampling = (1, 1)
first pixel value:1.75879 0.648926 12.7578 1
last pixel value:2.16992 1.61328 26.7812 1
```
#### dump multi part layer:
.\exr_dump.exe multi_ch.exr
```text
exr num parts: 5
C chunk_count:1024  num_channels:4
         Channel[0]: name = A, pixel_type = FLOAT, sampling = (1, 1)
         Channel[1]: name = B, pixel_type = FLOAT, sampling = (1, 1)
         Channel[2]: name = G, pixel_type = FLOAT, sampling = (1, 1)
         Channel[3]: name = R, pixel_type = FLOAT, sampling = (1, 1)
noise chunk_count:1024  num_channels:1
         Channel[0]: name = noise.Z, pixel_type = FLOAT, sampling = (1, 1)
displace chunk_count:1024  num_channels:1
         Channel[0]: name = displace.Z, pixel_type = FLOAT, sampling = (1, 1)
id chunk_count:1024  num_channels:1
         Channel[0]: name = id.Z, pixel_type = UINT, sampling = (1, 1)
vector chunk_count:1024  num_channels:3
         Channel[0]: name = vector.b, pixel_type = FLOAT, sampling = (1, 1)
         Channel[1]: name = vector.g, pixel_type = FLOAT, sampling = (1, 1)
         Channel[2]: name = vector.r, pixel_type = FLOAT, sampling = (1, 1)
Loaded 5 part images, now dump image info use ExrImage
        image name:C image num tiles:0 width: 1024 height:1024 num chans:4
                first pixel value:-0.0527842 0.0313708 -0.0606085 1
                last pixel value:-0.188693 0.0616372 -0.0752638 1
        image name:noise image num tiles:0 width: 1024 height:1024 num chans:1
        image name:displace image num tiles:0 width: 1024 height:1024 num chans:1
        image name:id image num tiles:0 width: 1024 height:1024 num chans:1
        image name:vector image num tiles:0 width: 1024 height:1024 num chans:3
```



## REF
```html
https://github.com/SaschaWillems/Vulkan
https://docs.vulkan.org/tutorial/latest/00_Introduction.html
https://docs.vulkan.org/guide/latest/vertex_input_data_processing.html
```

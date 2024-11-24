# LLVK_Beginning
Learn Vulkan from scratch
## license of assets
* None of the assets can be used commercially
* Except houdini works commercially

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

## REF
```html
https://github.com/SaschaWillems/Vulkan
https://docs.vulkan.org/tutorial/latest/00_Introduction.html
https://docs.vulkan.org/guide/latest/vertex_input_data_processing.html
```

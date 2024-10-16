# LLVK_Beginning
Learn Vulkan from scratch

## instance 

### terrain resource gen(houdini20.5):
1. terrain from houdini:

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

## REF
```html
https://github.com/SaschaWillems/Vulkan
https://docs.vulkan.org/tutorial/latest/00_Introduction.html
https://docs.vulkan.org/guide/latest/vertex_input_data_processing.html
```

## instance_v1 resources gen:

### instance_v1 rendering terrain only
### how to generate basic .png files
scene/instance/tex/generic_texture_gen.hip
1. gpu_albedo
2. gpu_ordp
3. gpu_n

### terrain v1 textures:
generate_ktx2.bat
1. gpu_albedo_2darray.ktx2
2. gpu_ordp_2darray.ktx2
3. gpu_n_2darray.ktx2

### terrain mask generation
from scene/instance/instance.hip
encoded mask:
![shadow.png](terrain_masks/mask.png)

R:cliff1
G:cliff2
B:small_rocks
A:Grass(as background) fully white color




## instance_v2 resources gen:
### generate terrain textures:
generate_terrain_v2_ktx2.bat

* difference with v1: v2 use 5 layer, v1 use 4 layer.
* v2 add new road texture

### terrain mask generation
from scene/instance/instance_v2.hip
R:cliff1
G:cliff2
B:small_rocks
A:road mask



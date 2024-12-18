
:: albedo  layer: cliff1 cliff2 small_rocks grass
:: ordp    layer: same as above
:: normal  layer: same as above


set TERRAIN_COLOR_OPTIONS=--format=R8G8B8A8_SRGB --generate-mipmap --encode=uastc --layers=5
set TERRAIN_ORDP_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --layers=5

:: albedo
ktx create  %TERRAIN_COLOR_OPTIONS% ^
cliff1/gpu_albedo.png ^
cliff2/gpu_albedo.png ^
small_rocks/gpu_albedo.png ^
road/gpu_albedo.png ^
grass/gpu_albedo.png ^
gpu_albedo_2darray_v2.ktx2

:: ordp
ktx create %TERRAIN_ORDP_OPTIONS% ^
cliff1/gpu_ordp.png ^
cliff2/gpu_ordp.png ^
small_rocks/gpu_ordp.png ^
road/gpu_ordp.png ^
grass/gpu_ordp.png ^
gpu_ordp_2darray_v2.ktx2

:: normal
ktx create %TERRAIN_ORDP_OPTIONS% ^
cliff1/normal.jpg ^
cliff2/normal.jpg ^
small_rocks/normal.jpg ^
road/normal.jpg ^
grass/normal.jpg ^
gpu_n_2darray_v2.ktx2

:: mask
ktx create --format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc terrain_masks/mask_v2.png gpu_terrain_mask_v2.ktx2
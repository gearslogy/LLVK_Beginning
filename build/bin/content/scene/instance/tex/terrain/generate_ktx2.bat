
:: albedo  layer: cliff1 cliff2 small_rocks grass
:: ordp    layer: same as above
:: normal  layer: same as above


set TERRAIN_COLOR_OPTIONS=--format=R8G8B8A8_SRGB --generate-mipmap --encode=uastc --layers=4
set TERRAIN_ORDP_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --layers=4

:: albedo
ktx create  %TERRAIN_COLOR_OPTIONS% ^
cliff1/gpu_albedo.png ^
cliff2/gpu_albedo.png ^
small_rocks/gpu_albedo.png ^
grass/gpu_albedo.png ^
gpu_albedo_2darray.ktx2

:: ordp
ktx create %TERRAIN_ORDP_OPTIONS% ^
cliff1/gpu_ordp.png ^
cliff2/gpu_ordp.png ^
small_rocks/gpu_ordp.png ^
grass/gpu_ordp.png ^
gpu_ordp_2darray.ktx2

:: normal
ktx create %TERRAIN_ORDP_OPTIONS% ^
cliff1/normal.jpg ^
cliff2/normal.jpg ^
small_rocks/normal.jpg ^
grass/normal.jpg ^
gpu_n_2darray.ktx2

:: mask
ktx create --format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc terrain_masks/mask.png gpu_terrain_mask.ktx2
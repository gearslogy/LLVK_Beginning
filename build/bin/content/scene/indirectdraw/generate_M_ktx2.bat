set TERRAIN_COLOR_OPTIONS=--format=R8G8B8A8_SRGB --generate-mipmap --encode=uastc --layers=5
set TERRAIN_ORDP_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --layers=5



:: M
ktx create %TERRAIN_ORDP_OPTIONS% ^
tree_flower_grass/grass/mix.png ^
tree_flower_grass/flower/mix.png ^
tree_flower_grass/tree/leaves_albedo.png ^
tree_flower_grass/tree/branch_albedo.png ^
tree_flower_grass/tree/root_albedo.png ^
tex/gpu_M_array.ktx2


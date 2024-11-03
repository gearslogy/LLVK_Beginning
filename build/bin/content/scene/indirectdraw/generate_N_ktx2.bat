set TERRAIN_COLOR_OPTIONS=--format=R8G8B8A8_SRGB --generate-mipmap --encode=uastc --layers=5
set TERRAIN_ORDP_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --layers=5


ktx create %TERRAIN_ORDP_OPTIONS% ^
tree_flower_grass/grass/normal.png ^
tree_flower_grass/flower/normal.png ^
tree_flower_grass/tree/leaves_normal.png ^
tree_flower_grass/tree/branch_normal.png ^
tree_flower_grass/tree/root_normal.png ^
tex/gpu_N_array.ktx2

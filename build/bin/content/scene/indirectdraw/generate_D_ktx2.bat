set TERRAIN_COLOR_OPTIONS=--format=R8G8B8A8_SRGB --generate-mipmap --encode=uastc --layers=5
set TERRAIN_ORDP_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --layers=5

:: albedo
ktx create %TERRAIN_COLOR_OPTIONS% --assign-oetf=srgb ^
tree_flower_grass/grass/albedo.png ^
tree_flower_grass/flower/albedo.png ^
tree_flower_grass/tree/leaves_albedo.png ^
tree_flower_grass/tree/branch_albedo.png ^
tree_flower_grass/tree/root_albedo.png ^
tex/gpu_D_array.ktx2

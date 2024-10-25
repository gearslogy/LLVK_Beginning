
set TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc 

:: FLOWER
ktx create %TEX_OPTIONS% --assign-oetf=srgb flower/albedo.png flower/gpu_albedo.ktx2
ktx create %TEX_OPTIONS% --assign-oetf=linear  flower/normal.png flower/gpu_N.ktx2


:: GRASS
ktx create %TEX_OPTIONS% ^ 
grass/albedo.png grass/gpu_albedo.ktx2

ktx create %TEX_OPTIONS% ^ 
grass/normal.png grass/gpu_N.ktx2


::  tree-chunk
ktx create %TEX_OPTIONS% --assign-oetf=srgb tree/branch_albedo.png  tree/gpu_branch_albedo.ktx2
ktx create %TEX_OPTIONS% --assign-oetf=linear tree/branch_mix.png tree/gpu_branch_M.ktx2
ktx create %TEX_OPTIONS% --assign-oetf=linear  tree/branch_normal.png tree/gpu_branch_N.ktx2

:: tree-leaves
ktx create %TEX_OPTIONS% --assign-oetf=srgb tree/leaves_albedo.png  tree/gpu_leaves_albedo.ktx2
ktx create %TEX_OPTIONS% --assign-oetf=linear tree/leaves_mix.png  tree/gpu_leaves_M.ktx2
ktx create %TEX_OPTIONS%  --assign-oetf=linear tree/leaves_normal.png tree/gpu_leaves_N.ktx2

:: tree-root
ktx create %TEX_OPTIONS% --assign-oetf=srgb  tree/root_albedo.png tree/gpu_root_albedo.ktx2
ktx create %TEX_OPTIONS% --assign-oetf=linear tree/root_mix.png tree/gpu_root_M.ktx2
ktx create %TEX_OPTIONS% --assign-oetf=linear tree/root_normal.png tree/gpu_root_N.ktx2


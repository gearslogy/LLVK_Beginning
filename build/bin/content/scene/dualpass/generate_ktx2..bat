
set TEX_OPTIONS=--format=R8G8B8A8_SRGB --generate-mipmap --encode=uastc

ktx create %TEX_OPTIONS% head.png gpu_head_D.ktx2
ktx create %TEX_OPTIONS% hair.png gpu_hair_D.ktx2
ktx create %TEX_OPTIONS% grid.jpg gpu_grid_D.ktx2
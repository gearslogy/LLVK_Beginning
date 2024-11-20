
set TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc 


ktx create %TEX_OPTIONS% hair.png gpu_D.ktx2
ktx create %TEX_OPTIONS% grid.jpg gpu_grid_D.ktx2
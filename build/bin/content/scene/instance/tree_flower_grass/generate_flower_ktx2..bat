
set TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc 

:: FLOWER
ktx create %TEX_OPTIONS% --assign-oetf=srgb flower/albedo.png flower/gpu_albedo.ktx2
ktx create %TEX_OPTIONS% --assign-oetf=linear  flower/normal.png flower/gpu_N.ktx2

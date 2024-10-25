
set TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc 


ktx create %TEX_OPTIONS% --assign-oetf=srgb grass/albedo.png grass/gpu_albedo.ktx2

ktx create %TEX_OPTIONS% --assign-oetf=linear grass/normal.png grass/gpu_N.ktx2



set TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc 


ktx create %TEX_OPTIONS% tex.png gpu_D.ktx2

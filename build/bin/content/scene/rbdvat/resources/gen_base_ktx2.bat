set TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc

:: FLOWER
ktx create %TEX_OPTIONS% 39_MedBuilding/39_MedBuilding_BaseColor.jpg gpu_textures/39_MedBuilding_gpu_D.ktx2

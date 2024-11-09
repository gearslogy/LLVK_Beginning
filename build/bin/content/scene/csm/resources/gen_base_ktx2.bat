set TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc

:: FLOWER
ktx create %TEX_OPTIONS% 29_WatchTower/29_WatchTower_BaseColor.jpg gpu_textures/29_WatchTower_gpu_D.ktx2
ktx create %TEX_OPTIONS% 35_MedBuilding/35_MedBuilding_BaseColor.jpg gpu_textures/35_MedBuilding_gpu_D.ktx2
ktx create %TEX_OPTIONS% 36_MedBuilding/36_MedBuilding_BaseColor.jpg gpu_textures/36_MedBuilding_gpu_D.ktx2
ktx create %TEX_OPTIONS% 39_MedBuilding/39_MedBuilding_BaseColor.jpg gpu_textures/39_MedBuilding_gpu_D.ktx2
ktx create %TEX_OPTIONS% ground/olxjp_2K_Albedo.jpg gpu_textures/ground_gpu_D.ktx2
set TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc

:: V1
::ktx create %TEX_OPTIONS% 39_MedBuilding/39_MedBuilding_BaseColor_withOCC.jpg gpu_textures/39_MedBuilding_gpu_D.ktx2

:: rbd_vat_v2.hip
ktx create %TEX_OPTIONS% rbd_vat_v2_diff.jpg gpu_textures/39_MedBuilding_gpu_D.ktx2
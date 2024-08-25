::rock_forest_vjdqcba/vjdqcba_2K_Albedo.jpg
::rock_other_wgpsaf0/wgpsaf0_2K_Albedo.jpg
::rock_rock_wdemcbi/wdemcbi_2K_Albedo.jpg
::rock_cliff_wdeledb/wdeledb_2K_Albedo.jpg

::ktx create --format=R8G8B8_SRGB --generate-mipmap rock_forest_vjdqcba/vjdqcba_2K_Albedo.jpg diff_A.ktx2

ktx create --format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --uastc-quality=1 rock_forest_vjdqcba/vjdqcba_2K_Albedo.jpg diff_A.ktx2
ktx create --format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --uastc-quality=1 rock_other_wgpsaf0/wgpsaf0_2K_Albedo.jpg diff_B.ktx2
ktx create --format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --uastc-quality=1 rock_rock_wdemcbi/wdemcbi_2K_Albedo.jpg diff_C.ktx2
ktx create --format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --uastc-quality=1 rock_cliff_wdeledb/wdeledb_2K_Albedo.jpg diff_D.ktx2

ktx create --format=R8G8B8_SRGB --generate-mipmap --encode=uastc --layers=4 rock_forest_vjdqcba/vjdqcba_2K_Albedo.jpg rock_other_wgpsaf0/wgpsaf0_2K_Albedo.jpg rock_rock_wdemcbi/wdemcbi_2K_Albedo.jpg rock_cliff_wdeledb/wdeledb_2K_Albedo.jpg diff_array.ktx2
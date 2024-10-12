
set FOLIAGE_TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --layers=6

ktx create %FOLIAGE_TEX_OPTIONS% ^
 green_grass/albedo.jpg ^
 green_grass/normal.jpg ^
 green_grass/roughness.jpg ^
 green_grass/ao.jpg ^
 green_grass/opacity.jpg ^
 green_grass/translucency.jpg ^
 green_grass_texarray.ktx2


ktx create %FOLIAGE_TEX_OPTIONS% ^
yellow_grass/albedo.jpg ^
yellow_grass/normal.jpg ^
yellow_grass/roughness.jpg ^
yellow_grass/ao.jpg ^
yellow_grass/opacity.jpg ^
yellow_grass/translucency.jpg ^
yellow_grass_texarray.ktx2

ktx create %FOLIAGE_TEX_OPTIONS% ^
large_foliage/albedo.jpg ^
large_foliage/normal.jpg ^
large_foliage/roughness.jpg ^
large_foliage/ao.jpg ^
large_foliage/opacity.jpg ^
large_foliage/translucency.jpg ^
large_foliage_texarray.ktx2


set CLIFF_DIR=terrain/cliff
set ROCK_DIR=terrain/small_rocks
set GRASS_DIR=terrain/grass

set TERRAIN_TEX_OPTIONS=--format=R8G8B8A8_UNORM --generate-mipmap --encode=uastc --layers=5
ktx create  %TERRAIN_TEX_OPTIONS% ^
%CLIFF_DIR%/albedo.jpg ^
%CLIFF_DIR%/normal.jpg ^
%CLIFF_DIR%/roughness.jpg ^
%CLIFF_DIR%/displacement.jpg ^
%CLIFF_DIR%/ao.jpg ^
terrain_cliff_texarray.ktx2


ktx create %TERRAIN_TEX_OPTIONS% ^
%ROCK_DIR%/albedo.jpg ^
%ROCK_DIR%/normal.jpg ^
%ROCK_DIR%/roughness.jpg ^
%ROCK_DIR%/displacement.jpg ^
%ROCK_DIR%/ao.jpg ^
terrain_rock_texarray.ktx2


ktx create %TERRAIN_TEX_OPTIONS% ^
%GRASS_DIR%/albedo.jpg ^
%GRASS_DIR%/normal.jpg ^
%GRASS_DIR%/roughness.jpg ^
%GRASS_DIR%/displacement.jpg ^
%GRASS_DIR%/ao.jpg ^
terrain_grass_texarray.ktx2
cmake_minimum_required(VERSION 3.25)
project(sims2_independent_bootstrap LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Initial isolated analysis stage. No game-specific runtime has been imported.
add_subdirectory(tools/host)

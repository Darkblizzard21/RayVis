add_subdirectory(cli11)

add_subdirectory(core-configuration)
add_subdirectory(core-utils)

add_subdirectory(imgui)
set_target_properties(imgui PROPERTIES FOLDER "imported")

add_subdirectory(linalg)
set_target_properties(linalg PROPERTIES FOLDER "imported")


set(RDF_BUILD_INSTALL ON CACHE BOOL "Install amdrdf libraries" FORCE)
add_subdirectory(rdf)
set_target_properties(amdrdf PROPERTIES FOLDER "imported")
set_target_properties(zstd PROPERTIES FOLDER "imported")

add_subdirectory(json)
set_target_properties(json PROPERTIES FOLDER "imported")


add_subdirectory(glm)
set_target_properties(glm PROPERTIES FOLDER "imported")


add_subdirectory(spdlog)
set_target_properties(spdlog PROPERTIES FOLDER "imported")

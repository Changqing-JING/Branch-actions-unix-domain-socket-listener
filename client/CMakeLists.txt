project(listener)

aux_source_directory(. srclists)
add_library(${PROJECT_NAME} SHARED ${srclists})
target_link_libraries(${PROJECT_NAME} dl)
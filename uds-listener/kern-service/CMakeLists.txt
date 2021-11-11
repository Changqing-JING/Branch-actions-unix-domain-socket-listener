project(unix-domain-socket-receiver)

aux_source_directory(. srclists)
add_library(${PROJECT_NAME} ${srclists})
target_link_libraries(${PROJECT_NAME} PUBLIC pthread)

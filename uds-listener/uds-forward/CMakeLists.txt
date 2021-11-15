project(uds-forward)

aux_source_directory(. srclists)
add_executable(${PROJECT_NAME} ${srclists})
target_link_libraries(${PROJECT_NAME} PRIVATE unix-domain-socket-receiver)

project(unix-domain-socket-listener)

aux_source_directory(. srclists)
add_executable(${PROJECT_NAME} ${srclists})
target_sources(ECS
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/include
        FILES
        Column.h
        Table.h
        TableManager.h
)
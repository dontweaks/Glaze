target_sources(ECS
        PUBLIC
        FILE_SET HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/include
        FILES
        ComponentSparseSet.h
        SparseArray.h
        SparseIndex.h
        SparseSet.h
)
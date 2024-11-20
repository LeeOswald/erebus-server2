add_library(erebus-system SHARED
    ../../include/erebus/system/bool.hxx
    ../../include/erebus/system/erebus.hxx
    ../../include/erebus/system/format.hxx
    ../../include/erebus/system/private/null_logger.hxx
)

target_sources(erebus-system
	PRIVATE
		init.cxx
)

target_link_libraries(erebus-system
	PUBLIC
		fmt::fmt
        boost::boost
)
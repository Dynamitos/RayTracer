target_sources(RayTracer 
	PRIVATE 
		main.cpp
		Minimal.h
		ThreadPool.h
		ThreadPool.cpp
)

#add_subdirectory(gpu/)
if(APPLE)
	add_subdirectory(metal/)
endif()
add_subdirectory(cpu/)
add_subdirectory(scene/)
add_subdirectory(util/)

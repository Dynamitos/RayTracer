target_sources(RayTracer 
	PRIVATE 
		main.cpp
		Minimal.h
		ThreadPool.h
		ThreadPool.cpp)

add_subdirectory(scene/)
add_subdirectory(window/)
add_subdirectory(util/)

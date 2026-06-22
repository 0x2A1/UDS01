set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(MAIN_DIR ${CMAKE_CURRENT_LIST_DIR})

# ========== 1. 递归收集所有.c文件 ==========
function(get_c_files out_list search_root)
	set(tmp_c "")
	# 收集当前目录c文件
	file(GLOB cur_c "${search_root}/*.c")
	list(APPEND tmp_c ${cur_c})
	# 遍历子文件夹递归
	file(GLOB sub_folders LIST_DIRECTORIES true "${search_root}/*")
	foreach(folder ${sub_folders})
		if(IS_DIRECTORY ${folder})
			get_c_files(sub_c ${folder})
			list(APPEND tmp_c ${sub_c})
		endif()
	endforeach()
	# 过滤测试文件
	list(FILTER tmp_c EXCLUDE REGEX "test_|demo_|example_")
	set(${out_list} ${tmp_c} PARENT_SCOPE)
endfunction()

# 批量多目录收集c文件
set(ALL_SRCS "")
set(SRC_FOLDERS
	${MAIN_DIR}
	${MAIN_DIR}/App
	${MAIN_DIR}/Bsp/ESP32
	${MAIN_DIR}/Core
	${MAIN_DIR}/Drivers
	${MAIN_DIR}/HAL
	${MAIN_DIR}/Library
	${MAIN_DIR}/Middle
)
foreach(path ${SRC_FOLDERS})
	get_c_files(tmp ${path})
	list(APPEND ALL_SRCS ${tmp})
endforeach()

# ========== 2. 只收集文件夹 ==========
function(get_inc_folders out_list search_root)
	set(tmp_dirs "")
	list(APPEND tmp_dirs ${search_root})
	file(GLOB sub_folders LIST_DIRECTORIES true "${search_root}/*")
	foreach(folder ${sub_folders})
		if(IS_DIRECTORY ${folder})
			list(APPEND tmp_dirs ${folder})
			get_inc_folders(sub_dirs ${folder})
			list(APPEND tmp_dirs ${sub_dirs})
		endif()
	endforeach()
	set(${out_list} ${tmp_dirs} PARENT_SCOPE)
endfunction()

# 批量收集头文件目录
set(ALL_INC "")
set(INC_FOLDERS
	${MAIN_DIR}
	${MAIN_DIR}/App
	${MAIN_DIR}/Bsp/ESP32
	${MAIN_DIR}/Core
	${MAIN_DIR}/Drivers
	${MAIN_DIR}/HAL
	${MAIN_DIR}/Library
	${MAIN_DIR}/Middle
)
foreach(path ${INC_FOLDERS})
	get_inc_folders(tmp ${path})
	list(APPEND ALL_INC ${tmp})
endforeach()
# 去重
list(REMOVE_DUPLICATES ALL_INC)

# ========== 组件注册 ==========
idf_component_register(
	SRCS
	${ALL_SRCS}
	INCLUDE_DIRS
	${ALL_INC}
	REQUIRES
	esp_driver_gpio
)

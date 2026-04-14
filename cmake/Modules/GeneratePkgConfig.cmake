set(_GeneratePkGConfigDir "${CMAKE_CURRENT_LIST_DIR}/GeneratePkgConfig")

include(GNUInstallDirs)

function(_get_target_property_merging_configs _var_name _target_name _propert_name)
	get_property(prop_set TARGET ${_target_name} PROPERTY ${_propert_name} SET)
	if(prop_set)
		get_property(vals TARGET ${_target_name} PROPERTY ${_propert_name})
	else()
		if(CMAKE_BUILD_TYPE)
			list(APPEND configs ${CMAKE_BUILD_TYPE})
		elseif(CMAKE_CONFIGURATION_TYPES)
			list(APPEND configs ${CMAKE_CONFIGURATION_TYPES})
		endif()
		foreach(cfg ${configs})
			string(TOUPPER "${cfg}" UPPERCFG)
			get_property(mapped_configs TARGET ${_target_name} PROPERTY "MAP_IMPORTED_CONFIG_${UPPERCFG}")
			if(mapped_configs)
				list(GET "${mapped_configs}" 0 target_cfg)
			else()
				set(target_cfg "${UPPERCFG}")
			endif()
			get_property(prop_set TARGET ${_target_name} PROPERTY ${_propert_name}_${target_cfg} SET)
			if(prop_set)
				get_property(val_for_cfg TARGET ${_target_name} PROPERTY ${_propert_name}_${target_cfg})
				list(APPEND vals "$<$<CONFIG:${cfg}>:${val_for_cfg}>")
				break()
			endif()
		endforeach()
		if(NOT prop_set)
			get_property(imported_cfgs TARGET ${_target_name} PROPERTY IMPORTED_CONFIGURATIONS)
			
			list(GET imported_cfgs 0 imported_config)
			get_property(vals TARGET ${_target_name} PROPERTY ${_propert_name}_${imported_config})
			
			string(REPLACE "$<$<CONFIG:${imported_config}>:" "$<1:" vals "${vals}")
		endif()
	endif()
	
	string(REPLACE "$<LINK_ONLY:" "$<1:" vals "${vals}")

	string(REPLACE "$<BUILD_INTERFACE:" "$<0:" vals "${vals}")
	string(REPLACE "$<INSTALL_INTERFACE:" "@CMAKE_INSTALL_PREFIX@/$<1:" vals "${vals}")
	set(${_var_name} "${vals}" PARENT_SCOPE)
endfunction()

function(_expand_targets _targets _libraries_var _include_dirs_var _compile_options_var _compile_definitions_var)
	set(_any_target_was_expanded True)
	set(_libs "${${_libraries_var}}")
	set(_includes "${${_include_dirs_var}}")
	set(_defs "${${_compile_definitions_var}}")
	set(_options "${${_compile_options_var}}")

	list(APPEND _libs "${_targets}")

	while(_any_target_was_expanded)
		set(_any_target_was_expanded False)
		set(_new_libs "")
		foreach (_dep ${_libs})
			if(TARGET ${_dep})
				set(_any_target_was_expanded True)

				get_target_property(_type ${_dep} TYPE)
				if("${_type}" STREQUAL "INTERFACE_LIBRARY")
					set(_imported_location "")
				else()
					_get_target_property_merging_configs(_imported_location ${_dep} IMPORTED_LOCATION)
				endif()

				_get_target_property_merging_configs(_iface_link_libraries ${_dep} INTERFACE_LINK_LIBRARIES)
				_get_target_property_merging_configs(_iface_include_dirs ${_dep} INTERFACE_INCLUDE_DIRECTORIES)
				_get_target_property_merging_configs(_iface_compile_options ${_dep} INTERFACE_COMPILE_OPTIONS)
				_get_target_property_merging_configs(_iface_definitions ${_dep} INTERFACE_COMPILE_DEFINITIONS)

				if(_imported_location)
					list(APPEND _new_libs "${_imported_location}")
				endif()

				if(_iface_link_libraries)
					list(APPEND _new_libs "${_iface_link_libraries}")
				endif()

				if(_iface_include_dirs)
					list(APPEND _includes "${_iface_include_dirs}")
				endif()

				if(_iface_compile_options)
					list(APPEND _options "${_iface_compile_options}")
				endif()

				if(_iface_definitions)
					list(APPEND _defs "${_iface_definitions}")
				endif()

				list(REMOVE_DUPLICATES _new_libs)
				list(REMOVE_DUPLICATES _includes)

				list(REMOVE_DUPLICATES _defs)
			else()
				list(APPEND _new_libs "${_dep}")
			endif()
		endforeach()
		set(_libs "${_new_libs}")
	endwhile()
	set(${_libraries_var} "${_libs}" PARENT_SCOPE)
	set(${_include_dirs_var} "${_includes}" PARENT_SCOPE)
	set(${_compile_options_var} "${_options}" PARENT_SCOPE)
	set(${_compile_definitions_var} "${_defs}" PARENT_SCOPE)
endfunction()

function(generate_and_install_pkg_config_file _target _packageName)
	_expand_targets(${_target}
		_interface_link_libraries _interface_include_dirs
		_interface_compile_options _interface_definitions)

	get_target_property(_output_name ${_target} OUTPUT_NAME)
	if(NOT _output_name)
		set(_output_name "${_target}")
	endif()

	set(_package_name "${_packageName}")

	foreach(d IN LISTS CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES)
		list(REMOVE_ITEM _interface_include_dirs "${d}")
	endforeach()

	set(_generate_target_dir "${CMAKE_CURRENT_BINARY_DIR}/${_target}-pkgconfig")
	set(_pkg_config_file_template_filename "${_GeneratePkGConfigDir}/pkg-config.cmake.in")

	if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.19")
		set(_target_arg TARGET ${_target})
	else()
		string(REPLACE "<COMPILE_LANG_AND_ID:CUDA,NVIDIA>" "<COMPILE_LANGUAGE:CUDA>" _interface_compile_options "${_interface_compile_options}")
	endif()

	configure_file("${_GeneratePkGConfigDir}/target-compile-settings.cmake.in"
		"${_generate_target_dir}/compile-settings.cmake" @ONLY)

	get_property(_isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
	if(NOT _isMultiConfig)
		set(_variables_file_name "${_generate_target_dir}/compile-settings-expanded.cmake")

		file(GENERATE OUTPUT "${_variables_file_name}" INPUT "${_generate_target_dir}/compile-settings.cmake" ${_target_arg})

		configure_file("${_GeneratePkGConfigDir}/generate-pkg-config.cmake.in"
			"${_generate_target_dir}/generate-pkg-config.cmake" @ONLY)

		install(SCRIPT "${_generate_target_dir}/generate-pkg-config.cmake")
	else()
		foreach(cfg IN LISTS CMAKE_CONFIGURATION_TYPES)
			set(_variables_file_name "${_generate_target_dir}/${cfg}/compile-settings-expanded.cmake")

			file(GENERATE OUTPUT "${_variables_file_name}" INPUT "${_generate_target_dir}/compile-settings.cmake" CONDITION "$<CONFIG:${cfg}>" ${_target_arg})

			configure_file("${_GeneratePkGConfigDir}/generate-pkg-config.cmake.in"
				"${_generate_target_dir}/${cfg}/generate-pkg-config.cmake" @ONLY)

			install(SCRIPT "${_generate_target_dir}/${cfg}/generate-pkg-config.cmake")
		endforeach()
	endif()
endfunction()
project('wahtwo', 'cpp', version: '0.1.0',
	meson_version: '>= 0.62.0',
	default_options: ['warning_level=2', 'werror=false', 'cpp_std=c++20'])

cxx = meson.get_compiler('cpp')

project_cpp_args = []
test_cpp_args = [
	'-Wno-volatile'
]

if get_option('buildtype') != 'plain'
	test_cpp_args += '-fstack-protector-strong'
endif

foreach arg: test_cpp_args
	if cxx.has_multi_arguments(arg)
		project_cpp_args += arg
	endif
endforeach

add_project_arguments(project_cpp_args, language: 'cpp')

subdir('src')

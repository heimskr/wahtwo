wahtwo_sources = []
wahtwo_deps = []
link_with = []
link_args = []

if build_machine.system() == 'darwin'
	wahtwo_sources += ['fsevents.cpp']
	link_args += ['-framework CoreServices']
elif build_machine.system() == 'linux'
	wahtwo_sources += ['inotify.cpp']
else
	error('Unsupported platform')
endif

inc_dirs = [
	include_directories('..' / 'include'),
]

library('wahtwo', wahtwo_sources,
	dependencies: wahtwo_deps,
	link_with: link_with,
	link_args: link_args,
	install: true,
	include_directories: [inc_dirs])

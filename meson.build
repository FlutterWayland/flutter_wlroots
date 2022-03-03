project('flutter wlroots', 'c', 'cpp')

cc = meson.get_compiler('c')

if get_option('flutter_compositor')
  add_project_arguments('-DFLUTTER_COMPOSITOR', language : 'c')
endif

embedder_dep = subproject('flutter_embedder').get_variable('embedder_dep')

wlroots_dep = subproject('wlroots', default_options: 'examples=false').get_variable('wlroots')

threads_dep = dependency('threads')

#wlroots_dep = dependency('wlroots')
wayland_server_dep = dependency('wayland-server')

glesv2_dep = dependency('glesv2')
egl_dep = dependency('egl')

fwr_src = []

fwr_deps = [
  embedder_dep,
  wlroots_dep,
  wayland_server_dep,
  glesv2_dep,
  egl_dep,
  threads_dep,
]

subdir('src')

fwr_inc = include_directories('include')

lib_fwr = library(
  'flutter_wlroots',
  fwr_src,
  dependencies : fwr_deps,
  include_directories : [fwr_inc],
)

flutter_wlroots = declare_dependency(
  link_with : lib_fwr,
  dependencies : fwr_deps,
  include_directories : fwr_inc,
)
meson.override_dependency('flutter_wlroots', flutter_wlroots)

if get_option('examples')
  subdir('example')
endif

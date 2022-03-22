project('flutter embedder', 'c')

# The git commit hash of the flutter engine we are to use.
#embedder_commit = '1ec3a58d7d9ca291270ddd7dd5d5b38bc39ee8b8'
#embedder_commit = 'e3559935720ea88dfcdf9079c394ffdb5146ceab'
#embedder_commit = 'd3ea636dc5d16b56819f3266241e1f708979c233'
embedder_commit = '6576bd428ae1f9556924d06211b77a540b5a3d5a'
# The sha256 of the built flutter embedder archive from the google buildbot.
# https://storage.googleapis.com/flutter_infra_release/flutter/<COMMIT>/linux-x64/linux-x64-embedder
#embedder_sha256 = '54e1c9e7e5674bfade09b423a0173fd334068396c6735fd72093442d3f1e595a'
#embedder_sha256 = '1fec41ad6767a5cb4c159355f01eedc6fdfb0dd209bb45c84337d8c058be41d0'
#embedder_sha256 = '4275f19c5c3e1b7a513cb913c7ce16f6809d5fa30a75c7ef8a974f14af8f43ee'
embedder_sha256 = '26269e48980acc18bc9a8fef6fd619b3d344096b171a4547e70c6e40cd3b84b8'

embedder_archive_url = 'https://storage.googleapis.com/flutter_infra_release/flutter/' + embedder_commit + '/linux-x64/linux-x64-embedder'
embedder_archive_path = 'linux-x64-embedder.zip'
embedder_path = 'linux-x64-embedder'

run_command('python', 'meson_file_download.py', embedder_archive_url, embedder_archive_path, check: true)
run_command('python', 'meson_file_extract.py', embedder_archive_path, embedder_path, check: true)

cc = meson.get_compiler('c')
embedder_dep = declare_dependency(
  dependencies : cc.find_library('flutter_engine', dirs : meson.current_source_dir()),
  include_directories : include_directories('.'))

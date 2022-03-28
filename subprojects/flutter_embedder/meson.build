project('flutter embedder', 'c')

# The git commit hash of the flutter engine we are to use.
embedder_commit = get_option('engine_commit')
# The sha256 of the built flutter embedder archive from the google buildbot.
# https://storage.googleapis.com/flutter_infra_release/flutter/<COMMIT>/linux-x64/linux-x64-embedder
embedder_sha256 = get_option('engine_embedder_hash')

embedder_archive_url = 'https://storage.googleapis.com/flutter_infra_release/flutter/' + embedder_commit + '/linux-x64/linux-x64-embedder'
embedder_archive_path = 'linux-x64-embedder.zip'
embedder_path = 'linux-x64-embedder'

if embedder_sha256 != ''
  run_command('python', 'meson_file_download.py', embedder_archive_url, embedder_archive_path, '-hash', 'sha256', embedder_sha256, check: true)
else
  run_command('python', 'meson_file_download.py', embedder_archive_url, embedder_archive_path, check: true)
endif

run_command('python', 'meson_file_extract.py', embedder_archive_path, embedder_path, check: true)

cc = meson.get_compiler('c')
embedder_dep = declare_dependency(
  dependencies : cc.find_library('flutter_engine', dirs : meson.current_source_dir()),
  include_directories : include_directories('.'))

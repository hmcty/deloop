project('bizzy', ['c'])

cc = meson.get_compiler('c')
jack_dep = dependency('jack')
m_dep = cc.find_library('m', required: false)
executable('bizzy', 'src/main.c', dependencies: [jack_dep, m_dep])

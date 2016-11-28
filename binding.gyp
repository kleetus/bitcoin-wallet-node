{
  'target_defaults':
    {
      'cflags_cc': ['-fexceptions']
    },
  'targets': [
    {
      'target_name': 'wallet',
      'type': 'loadable_module',
      'variables': {
        'deps': '<!(./build_scripts/bdb.sh dep)',
      },
      'sources': [
        'src/wallet.cc'
      ],
      'dependencies': [],
      'cflags': ['-Wall', '-std=c++11'],
      'include_dirs': [
        './dependencies/db-4.8.30.NC/build_unix',
        './dependencies/json/src',
        '<!(node -e "require(\'nan\')")',
        '<!(node -e "require(\'streaming-worker-sdk\')")'
      ],
      'conditions': [
        ['OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
            'MACOSX_DEPLOYMENT_TARGET': '10.7'
          },
          'conditions': [
            ['clang==1', {
              'xcode_settings': {
                'CLANG_CXX_LANGUAGE_STANDARD': 'gnu++0x',
                'CLANG_CXX_LIBRARY': 'libc++'
              },
            }]
          ]
        }],
        [ 'OS in "linux freebsd openbsd solaris android aix"',
          {
            'cflags_cc': [ '-fexceptions', '-std=gnu++0x' ]
          }
        ]
      ],
      'link_settings': {
        'libraries': [
          '-lpthread',
          '<!(build_scripts/bdb.sh lib)'
        ],
        'library_dirs': [
        ]
      }
    }
  ]
}

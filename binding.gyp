{
  'target_defaults':
    {
      'cflags_cc': ['-fexceptions']
    },
  'targets': [
    {
      'target_name': 'wallet',
      'type': 'loadable_module',
      'sources': [
        'src/wallet.cc'
      ],
      'cflags': ['-Wall', '-std=c++11'],
      'include_dirs': [
        './dependencies/db-4.8.30.NC/build_unix',
        './dependencies/json/src',
        "<!(node -e \"require('nan')\")",
        "<!(node -e \"require('streaming-worker-sdk')\")"
      ],
      'conditions': [
        ['<!(./build_scripts/bdb.sh)=="lib"', {
          'link_settings': {
            'libraries': [
              '-lpthread',
              '-ldb_cxx-4.8'
            ]
          }
        },
        { 'dependencies': ['./dependencies/db-4.8.30.NC/bdb.gyp:bdb'] }
        ],
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
        ],
        'library_dirs': [
        ]
      }
    }
  ]
}

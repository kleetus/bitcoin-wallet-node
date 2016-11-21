{
  'targets': [
    {
      'target_name': 'wallet',
      'type': 'loadable_module',
      'sources': [
        'src/wallet.cc'
      ],
      'include_dirs': [
        "./dependencies/db-4.8.30.NC/build_unix",
        "<!(node -e \"require('nan')\")"
      ],
      'link_settings': {
        'libraries': [
          '-ldb_cxx-4.8'
        ],
        'library_dirs': [
          '/lib64'
        ]
      }
    }
  ]
}

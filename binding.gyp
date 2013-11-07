{
  "targets": [
    {
      "target_name": "dialer",
      "sources": [ "dialer.cc" ],
      'include_dirs': [
  	'/home/ashish/test/node/dialer',
	],
      "link_settings": {
          'libraries': [
              '-lPJSIP', '-L/home/ashish/test/node/dialer',
			  '-lpjsua-x86_64-unknown-linux-gnu',
			  '-lpjsip-ua-x86_64-unknown-linux-gnu',
			  '-lpjsip-simple-x86_64-unknown-linux-gnu',
			  '-lpjsip-x86_64-unknown-linux-gnu',
			  '-lpjmedia-codec-x86_64-unknown-linux-gnu', 
			  '-lpjmedia-audiodev-x86_64-unknown-linux-gnu',
			  '-lpjnath-x86_64-unknown-linux-gnu', 
			  '-lpjlib-util-x86_64-unknown-linux-gnu', 
			  '-lpjmedia-x86_64-unknown-linux-gnu', 
			  '-lpjsdp-x86_64-unknown-linux-gnu',
			  '-lpjmedia-videodev-x86_64-unknown-linux-gnu', 
			  '-lmilenage-x86_64-unknown-linux-gnu',
              '-lsrtp-x86_64-unknown-linux-gnu', 
			  '-lresample-x86_64-unknown-linux-gnu',
			  '-lgsmcodec-x86_64-unknown-linux-gnu',
			  '-lspeex-x86_64-unknown-linux-gnu',
			  '-lilbccodec-x86_64-unknown-linux-gnu',
			  '-lg7221codec-x86_64-unknown-linux-gnu',
			  '-lportaudio-x86_64-unknown-linux-gnu',
              '-lpj-x86_64-unknown-linux-gnu', 
			  '-lm', '-lnsl', '-lrt', '-lpthread', '-lasound',
			  '-L/home/ashish/Downloads/pjproject-2.1.0/pjlib/lib',
			  '-L/home/ashish/Downloads/pjproject-2.1.0/pjlib-util/lib',
			  '-L/home/ashish/Downloads/pjproject-2.1.0/pjnath/lib',
			  '-L/home/ashish/Downloads/pjproject-2.1.0/pjmedia/lib',
			  '-L/home/ashish/Downloads/pjproject-2.1.0/pjsip/lib',
			  '-L/home/ashish/Downloads/pjproject-2.1.0/third_party/lib'
          ]
      }
    }
  ]
}

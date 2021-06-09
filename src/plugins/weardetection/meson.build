project('GEO-Fencing', 'c',
  version : '1.0.0',
  default_options : ['warning_level=3'])

add_languages('cpp', required : true)

cc = meson.get_compiler('c')

gst = dependency('gstreamer-1.0')
gst_base = dependency('gstreamer-base-1.0')
#gst_video = declare_dependency(
#    dependencies : cc.find_library('gstvideo-1.0'),
#    include_directories : ['/usr/include/gstreamer-1.0'],
#  )

eva_root_dir = get_option('eva_root')
inc_gst_video = ''
lib_gst_video = []
if target_machine.system() == 'linux'
	lib_gst_video += cc.find_library('gstvideo-1.0', required : true)
	inc_gst_video = include_directories('/usr/include/gstreamer-1.0')
elif target_machine.system() == 'windows'
	lib_gst_video += cc.find_library('gstvideo-1.0', required : true, dirs: 'C:\ADLINK\gstreamer\lib')
	inc_gst_video = include_directories(join_paths(eva_root_dir, '..', 'gstreamer', 'include', 'gstreamer-1.0'))
endif

gst_video = declare_dependency(
	dependencies : lib_gst_video,
	include_directories : inc_gst_video,
	)
  
## Declare OpenCV4 dependency
#opencv_dir = '/usr/local'
opencv_dir = get_option('opencv_dir')
libs_cv = []
req_cv_libs = ['core', 'imgproc', 'video', 'videoio', 'imgcodecs', 'highgui']
inc_cv = ''

if target_machine.system() == 'linux'
  foreach lib_name : req_cv_libs
    libs_cv += cc.find_library('opencv_' + lib_name,
                   required : true,
                   dirs : [join_paths(opencv_dir, 'lib'),
                   join_paths(opencv_dir, 'lib', '@0@-linux-gnu'.format(target_machine.cpu()))])    
  endforeach
  inc_cv = include_directories(join_paths(opencv_dir, 'include', 'opencv4'))
elif target_machine.system() == 'windows'
  foreach lib_name : req_cv_libs
    libs_cv += cc.find_library('opencv_' + lib_name + (get_option('buildtype') == 'release' ? '450' : '450d'),
                   required : true,
                   dirs : [join_paths(opencv_dir, 'lib')])
  endforeach
  inc_cv = include_directories(join_paths(opencv_dir, 'include'))
endif

cv_dep = declare_dependency(
  dependencies : libs_cv,
  include_directories: inc_cv,
)
#### End to declare OpenCV4 dependency

## add gstadmeta.so
eva_root_dir = get_option('eva_root')
libs_meta = []
libs_meta += cc.find_library('gstadmeta',
                required : true,
                dirs : [join_paths(eva_root_dir, 'lib')])


inc_meta = include_directories(join_paths(eva_root_dir, 'include'))

gstmeta_dep = declare_dependency(
  dependencies : libs_meta,
  include_directories: inc_meta,
)
#### End to declare gstmeta dependency

library('weardetection',
    'gstweardetection.cpp',
    dependencies : [gst, gst_base, gst_video, cv_dep, gstmeta_dep],
  )

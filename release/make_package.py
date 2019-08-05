#!/usr/bin/env python

from zipfile import ZipFile, ZIP_DEFLATED
from helpers import *

navigate_to_project_dir()

version_str = get_file_text(os.path.join('release', 'version.txt'))
printc('Create redistributable package version {}'.format(version_str), Colors.BOLD)

if IS_WINDOWS:
  # when run with -v, windeployqt returns 1 and prints long help message,
  # so don't print stdout and don't check return code
  check_qt_path(cmd = 'windeployqt -v', print_stdout = False, check_return_code = False)
if IS_MACOS:
  check_qt_path(cmd = 'macdeployqt -v', print_stdout = False, check_return_code = False)

create_dir_if_none(OUT_DIR)
os.chdir(OUT_DIR)

recreate_dir_if_exists(REDIST_DIR)
os.chdir(REDIST_DIR)

package_name = PROJECT_NAME + '-' + version_str


def make_package_for_windows():
  print_header('Run windeployqt...')
  execute('windeployqt ..\\..\\bin\\{} --dir . --no-translations --no-system-d3d-compiler --no-opengl-sw'.format(PROJECT_EXE))

  print_header('Clean some excessive files...')
  remove_files(['libEGL.dll', 'libGLESV2.dll'])
  remove_files(['sqldrivers\\qsqlmysql.dll',
                'sqldrivers\\qsqlodbc.dll',
                'sqldrivers\\qsqlpsql.dll'])
  remove_files(['imageformats\\qicns.dll',
                'imageformats\\qtga.dll',
                'imageformats\\qtiff.dll',
                'imageformats\\qwbmp.dll',
                'imageformats\\qwebp.dll'])

  print_header('Copy project files...')
  shutil.copyfile('..\\..\\bin\\' + PROJECT_EXE, PROJECT_EXE)

  print_header('Pack files to zip...')
  global package_name
  package_name = '{}-win-x{}.zip'.format(package_name, get_exe_bits(PROJECT_EXE))
  with ZipFile('..\\' + package_name, mode='w', compression=ZIP_DEFLATED) as zip:
     for dirname, subdirs, filenames in os.walk('.'):
        for filename in filenames:
          zip.write(os.path.join(dirname, filename))


def make_package_for_linux():
  pass


def make_package_for_macos():
  print_header('Copy application bundle...')
  remove_dir(PROJECT_EXE)
  shutil.copytree('../../bin/' + PROJECT_EXE, PROJECT_EXE)

  print_header('Run macdeployqt...')
  execute('macdeployqt {}'.format(PROJECT_EXE))
  
  print_header('Clean some excessive files...')
  remove_files([PROJECT_EXE + '/Contents/PlugIns/sqldrivers/libqsqlmysql.dylib',
                PROJECT_EXE + '/Contents/PlugIns/sqldrivers/libqsqlpsql.dylib'])
  remove_files([PROJECT_EXE + '/Contents/PlugIns/imageformats/libqico.dylib',
                PROJECT_EXE + '/Contents/PlugIns/imageformats/libqtga.dylib',
                PROJECT_EXE + '/Contents/PlugIns/imageformats/libqtiff.dylib',
                PROJECT_EXE + '/Contents/PlugIns/imageformats/libqwbmp.dylib',
                PROJECT_EXE + '/Contents/PlugIns/imageformats/libqwebp.dylib'])
           
  print_header('Pack application bundle to dmg...')
  global package_name
  package_name = package_name + '.dmg'
  remove_files(['tmp.dmg', '../' + package_name])
  execute('hdiutil create tmp.dmg -ov -volname {} -fs HFS+ -srcfolder {}'.format(PROJECT_NAME, PROJECT_EXE)) 
  execute('hdiutil convert tmp.dmg -format UDZO -o ../{}'.format(package_name))


if IS_WINDOWS:
  make_package_for_windows()
elif IS_LINUX:
  make_package_for_linux()
elif IS_MACOS:
  make_package_for_macos()

print('\nPackage created: {}'.format(package_name))
printc('Done\n', Colors.OKGREEN)

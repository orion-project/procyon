#!/usr/bin/env python

from zipfile import ZipFile
from helpers import *

navigate_to_project_dir()

version_str = get_file_text(os.path.join('release', 'version.txt'))
printc('Create redistributable package version {}'.format(version_str), Colors.BOLD)

if IS_WINDOWS:
  # when run with -v, windeployqt returns 1 and prints long help message,
  # so don't print stdout and don't check return code
  check_qt_path(cmd = 'windeployqt -v', print_stdout = False, check_return_code = False)

create_dir_if_none(OUT_DIR)
os.chdir(OUT_DIR)

recreate_dir_if_exists(REDIST_DIR)
os.chdir(REDIST_DIR)


def make_package_for_windows():
  print_header('Run windeployqt...')
  execute('windeployqt ..\\..\\bin\procyon.exe --dir . --no-translations --no-system-d3d-compiler --no-opengl-sw --qmldir ..\\..\\src')

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
  shutil.copyfile('..\\..\\bin\\procyon.exe', 'procyon.exe')

  print_header('Pack files to zip...')
  package_name = 'procyon-{}-win-x{}.zip'.format(version_str, get_exe_bits('procyon.exe'))
  print(package_name)
  with ZipFile('..\\' + package_name, 'w') as zip:
     for dirname, subdirs, filenames in os.walk('.'):
        for filename in filenames:
          zip.write(os.path.join(dirname, filename))


def make_package_for_linux():
  pass


def make_package_for_macos():
  pass


if IS_WINDOWS:
  make_package_for_windows()
elif IS_LINUX:
  make_package_for_linux()
elif IS_MACOS:
  make_package_for_macos()

printc('\nDone\n', Colors.OKGREEN)

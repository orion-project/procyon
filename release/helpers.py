from __future__ import print_function

import os
import subprocess
import platform
import shutil
import struct
import errno

IS_WINDOWS = False
IS_LINUX = False
IS_MACOS = False
p = platform.system()
if p == 'Windows':
  IS_WINDOWS = True
elif p == 'Linux':
  IS_LINUX = True
elif p == 'Darwin':
  IS_MACOS = True
else:
  print('ERROR: Unknown platform {}'.format(p))
  exit()


PROJECT_FILE = 'procyon.pro'
OUT_DIR = 'out'
BUILD_DIR = 'build'
REDIST_DIR = 'redist'

class Colors:
  HEADER = '\033[95m'
  OKBLUE = '\033[94m'
  OKGREEN = '\033[92m'
  WARNING = '\033[93m'
  FAIL = '\033[91m'
  ENDC = '\033[0m'
  BOLD = '\033[1m'
  UNDERLINE = '\033[4m'


def printc(txt, color):
  if IS_WINDOWS:
    print(txt)
  else:
    print(color + txt + Colors.ENDC)


def print_error_and_exit(txt):
  printc('ERROR: ' + txt, Colors.FAIL)
  exit()


def print_header(txt):
  print('')
  if IS_WINDOWS:
    print('***** ' + txt)
  else:
    printc(txt, Colors.HEADER)


def execute(cmd, print_stdout = True, check_return_code = True):
  # It works in Python 3.5 but Python 2.7 can't find command with parameters
  # given as a string (e.g.: 'qmake -v'), it should be splitted to array (['qmake', '-v'])
  p = subprocess.Popen(cmd.split(' '), stdout=subprocess.PIPE, stderr=subprocess.STDOUT, universal_newlines=True)
  for line in iter(p.stdout.readline, ''):
    if print_stdout:
      print(line, end='')
  p.stdout.close()
  return_code = p.wait()
  if return_code and check_return_code:
    raise subprocess.CalledProcessError(return_code, cmd)


def navigate_to_project_dir():
  curdir = os.path.dirname(os.path.realpath(__file__))
  os.chdir(os.path.join(curdir, '..'))
  if not os.path.isfile(PROJECT_FILE):
    print_error_and_exit((
      'Project file {} not found in the current directory.\n' +
      'This script should be run from the project directory.'
    ).format(PROJECT_FILE))


def check_qt_path(cmd = 'qmake -v', print_stdout = True, check_return_code = True):
  print_header('Check if Qt is in PATH...')

  def get_qt_path_example():
    if IS_WINDOWS:
      return 'set PATH=c:\\Qt\\5.12.0\\mingw73_64\\bin;%PATH%'
    if IS_LINUX:
      return 'export PATH=/home/user/Qt/5.10.0/gcc_64/bin:$PATH$'
    if IS_MACOS:
      return 'export PATH=/Users/user/Qt/5.10.0/clang_64/bin:$PATH$'

  try:
    execute(cmd, print_stdout, check_return_code)
    print()
  except OSError as e:
    if e.errno == errno.ENOENT:
      printc('ERROR: qmake not found in PATH', Colors.FAIL)
      print('Find Qt installation and update your PATH like:')
      printc(get_qt_path_example(), Colors.BOLD)
      exit()
    else:
      raise e


def check_make_path():
  if not IS_WINDOWS:
    return
  try:
    execute('mingw32-make --version')
    print()
  except OSError as e:
    if e.errno == errno.ENOENT:
      printc('ERROR: mingw32-make not found in PATH', Colors.FAIL)
      print('Ensure you have MinGW installed and update PATH like:')
      printc('set PATH=c:\\Qt\\Tools\\mingw730_64\\bin;%PATH%', Colors.BOLD)
      exit()
    else:
      raise e


def create_dir_if_none(dirname):
  print_header('Create "{}" dir if none...'.format(dirname))
  if not os.path.exists(dirname):
    os.mkdir(dirname)
  else:
    print('Already there')


def recreate_dir_if_exists(dirname):
  print_header('Recreate "{}" directory...'.format(dirname))
  if os.path.exists(dirname):
    shutil.rmtree(dirname)
  os.mkdir(dirname)


def remove_files(filenames):
  for filename in filenames:
    if os.path.exists(filename):
      os.remove(filename)


def get_file_text(file_name):
    with open(file_name, 'r') as f:
        return f.read()


def set_file_text(file_name, text):
    with open(file_name, 'w') as f:
        f.write(text)


def get_exe_bits(file_name):
  with open(file_name, 'rb') as f:
    if f.read(2) != b'MZ':
      raise Exception('Invalid exe file {}: invalid MZ signature'.format(file_name))
    f.seek(0x3A, 1) # seek to e_lfanew
    f.seek(struct.unpack('=L', f.read(4))[0], 0) # //seek to the start of the NT header.
    if f.read(4) != b'PE\0\0':
      raise Exception('Invalid exe file {}: invalid PE signature'.format(file_name))
    f.seek(20, 1) # seek past the file header
    arch = f.read(2)
    if arch ==  b'\x0b\x01':
      return 32
    elif arch == b'\x0b\x02':
      return 64
    else:
      raise Exception('Invalid exe file {}: unknown magic number {}'.format(file_name, str(arch)))

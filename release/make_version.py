#!/usr/bin/env python
#
# Script updates version information in version.pri and version.rc,
# and creates version.txt file contaning version string "X.Y.Z-codename"
# that can be read by another scripts.
#
# Target version should be given via script parameter:
#    ./make_version.py 2.0.2-alpha2
#

import os
import re
import sys
import datetime
import helpers

# Get version string from command line
if len(sys.argv) < 2:
  helpers.print_error_and_exit('No version string is given')

helpers.navigate_to_project_dir()
os.chdir('release')

version_parts = sys.argv[1].split('.')
if len(version_parts) != 3:
  helpers.print_error_and_exit('Invalid version string')

version_major = version_parts[0]
version_minor = version_parts[1]
version_patch = version_parts[2]
codename = ''

patch_parts = version_patch.split('-')
if len(patch_parts) > 1:
  version_patch = patch_parts[0]
  codename = patch_parts[1]

# We have version now
version_str = '%s.%s.%s' % (version_major, version_minor, version_patch)
if codename:
  version_str = version_str + '-' + codename
print('Version: %s' % version_str)

year = str(datetime.datetime.now().year)

# Update version.txt
print('Updating version.txt')
helpers.set_file_text('version.txt', version_str)

# Update version.pri
print('Updating version.pri')
file_name = 'version.pri'
text = helpers.get_file_text(file_name)

def replace(key, value):
  global text
  text = re.sub('^' + key + '=.*$', key + '=' + value, text, 1, re.MULTILINE)

replace('APP_VER_MAJOR', version_major)
replace('APP_VER_MINOR', version_minor)
replace('APP_VER_PATCH', version_patch)
replace('APP_VER_CODENAME', codename)
replace('APP_VER_YEAR', year)
replace('APP_VER', version_str)
helpers.set_file_text(file_name, text)

# Update version.rc
print('Updating version.rc')
text = helpers.get_file_text('version.rc.template')
text = text.replace('{v1}', version_major)
text = text.replace('{v2}', version_minor)
text = text.replace('{v3}', version_patch)
text = text.replace('{v4}', '0')
text = text.replace('{year}', year)
text = text.replace('{codename}', codename)
helpers.set_file_text('version.rc', text)

print('OK')

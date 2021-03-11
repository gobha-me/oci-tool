#!/bin/python
import os
print("Beginning python script to determine version and release")
# Get Env vars of build from OS
CI_TAG = os.getenv('CI_COMMIT_TAG')
CI_BRANCH = os.getenv('CI_COMMIT_BRANCH')
CI_SHSHA = os.getenv('CI_COMMIT_SHORT_SHA')
# Little if-else to ensure we have valid version and release
if CI_BRANCH == "master":
    try:
        version = CI_TAG
        release = "rel"
    except:
        version = "0.0."+CI_SHSHA
        release = "beta"
else:
    version = "0.0."+CI_SHSHA
    release = "devel"
print(version, release)
# Output version and release to os env
os.environ['RPM_VERSION'] = 'version'
os.environ['RPM_RELEASE'] = 'release'
# Read file into mem
with open('/home/gitlabRunner/rpmbuild/SPECS/oci-sync-TEMPLATE.spec', 'r') as file :
    filedata = file.read()
# Replace the target string
filedata = filedata.replace('RPM_VERSION', version )
filedata = filedata.replace('RPM_RELEASE', release )
with open('/home/gitlabRunner/rpmbuild/SPECS/oci-sync-TEMPLATE.spec', 'w') as file:
    file.write(filedata)
# Create properly-named directory for build
if not os.path.exists("/home/gitlabRunner/oci-sync-"+RPM_VERSION):
    os.makedirs("/home/gitlabRunner/oci-sync-"+RPM_VERSION)
# Collect files for the rpmbuild TODO: Make this a function
shutil.copyfile(/home/gitlabRunner/gitPull/oci-tool/LICENSE, "/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/", *, follow_symlinks=True)
shutil.copymode(/home/gitlabRunner/gitPull/oci-tool/LICENSE, "/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/", *, follow_symlinks=True)
shutil.copyfile(/home/gitlabRunner/build_files/build/bin/oci-sync, "/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/oci-sync.bin", *, follow_symlinks=True)
shutil.copymode(/home/gitlabRunner/build_files/build/bin/oci-sync, "/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/oci-sync.bin", *, follow_symlinks=True)
shutil.copyfile(/home/gitlabRunner/gitPull/oci-tool/rpmbuild/oci-sync.man, "/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/", *, follow_symlinks=True)
shutil.copymode(/home/gitlabRunner/gitPull/oci-tool/rpmbuild/oci-sync.man, "/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/", *, follow_symlinks=True)
os.system('rpmdev-setuptree')
# Collect more files for the rpmbuild TODO: No, really, make this a function
- cp /home/gitlabRunner/gitPull/oci-tool/LICENSE ~/rpmbuild/SOURCES/
- cp /home/gitlabRunner/build_files/build/bin/oci-sync ~/rpmbuild/SOURCES/oci-sync.bin
- cp /home/gitlabRunner/gitPull/oci-tool/rpmbuild/oci-sync.man ~/rpmbuild/SOURCES/
- tar -czf ~/oci-sync ./oci-sync-$RPM_VERSION
    # Ensure all files are in the right place for the rpmbuild
- mv ~/oci-sync ~/rpmbuild/SOURCES/
- cd /home/gitlabRunner/rpmbuild/SOURCES
- gunzip -c oci-sync | tar xvf -
- cp /home/gitlabRunner/gitPull/oci-tool/rpmbuild/oci-sync-TEMPLATE.spec ~/rpmbuild/SPECS/
# rpmbuild
- cd ~/rpmbuild/SPECS/
- rpmbuild -ba oci-sync-TEMPLATE.spec
- rm -rf /home/gitlabRunner/oci-sync-$RPM_VERSION
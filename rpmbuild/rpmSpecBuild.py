#!/usr/python
import os
import shutil
print("Beginning python script to determine version and release")
# Get Env vars of build from OS
CI_TAG = os.getenv('CI_COMMIT_TAG')
CI_BRANCH = os.getenv('CI_COMMIT_BRANCH')
CI_SHSHA = os.getenv('CI_COMMIT_SHORT_SHA')
def funcFCopyAndPermissions( inputF, outputF, a="*", followS="follow_symlinks=True"):
    shutil.copyfile(inputF, outputF)
    shutil.copymode(inputF, outputF)
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
RPM_VERSION = version
RPM_RELEASE = release
# Create properly-named directory for build staging
if not os.path.exists("/home/gitlabRunner/oci-sync-"+RPM_VERSION):
    os.makedirs("/home/gitlabRunner/oci-sync-"+RPM_VERSION)
# Collect files for the rpmbuild
funcFCopyAndPermissions( "/home/gitlabRunner/gitPull/oci-tool/LICENSE", "/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/LICENSE")
funcFCopyAndPermissions( "/home/gitlabRunner/build_files/build/bin/oci-sync","/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/oci-sync.bin")
funcFCopyAndPermissions( "/home/gitlabRunner/gitPull/oci-tool/rpmbuild/oci-sync.man", "/home/gitlabRunner/oci-sync-"+RPM_VERSION+"/oci-sync.man")
# Make call out to os to execute rpmdev-setuptree
os.system('rpmdev-setuptree')
# Collect more files for the rpmbuild
funcFCopyAndPermissions( "/home/gitlabRunner/gitPull/oci-tool/LICENSE", "/home/gitlabRunner/rpmbuild/SOURCES/LICENSE")
funcFCopyAndPermissions( "/home/gitlabRunner/build_files/build/bin/oci-sync", "/home/gitlabRunner/rpmbuild/SOURCES/oci-sync.bin")
funcFCopyAndPermissions( "/home/gitlabRunner/gitPull/oci-tool/rpmbuild/oci-sync.man", "/home/gitlabRunner/rpmbuild/SOURCES/oci-sync.man")
funcFCopyAndPermissions( "/home/gitlabRunner/gitPull/oci-tool/rpmbuild/oci-sync-TEMPLATE.spec", "/home/gitlabRunner/rpmbuild/SPECS/oci-sync-TEMPLATE.spec")
# Read spec file into mem and edit the version and release
with open('/home/gitlabRunner/rpmbuild/SPECS/oci-sync-TEMPLATE.spec', 'r') as file :
    filedata = file.read()
    # Replace the target string
    filedata = filedata.replace('RPM_VERSION', version )
    filedata = filedata.replace('RPM_RELEASE', release )
# Write back changes to file
with open('/home/gitlabRunner/rpmbuild/SPECS/oci-sync-TEMPLATE.spec', 'w') as file:
    file.write(filedata)
os.system('tar -czf /home/gitlabRunner/oci-sync -C /home/gitlabRunner ./oci-sync-'+RPM_VERSION)
funcFCopyAndPermissions( "/home/gitlabRunner/oci-sync", "/home/gitlabRunner/rpmbuild/SOURCES/oci-sync")
os.system('gunzip -c /home/gitlabRunner/rpmbuild/SOURCES/oci-sync | tar xvf - -C /home/gitlabRunner/rpmbuild/SOURCES/')
os.system('rpmbuild -ba /home/gitlabRunner/rpmbuild/SPECS/oci-sync-TEMPLATE.spec')

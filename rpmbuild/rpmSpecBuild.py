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
        version = CI_SHSHA
        release = "beta"
else:
    version = CI_SHSHA
    release = "devel"
print(version, release)
# Output version and release to os env
os.environ['RPM_VERSION'] = 'version'
os.environ['RPM_RELEASE'] = 'release'
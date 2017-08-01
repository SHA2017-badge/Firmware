We warmly welcome contributions. New versions of the badge firmware are
distributed periodically to all users on the field through OTA updates. While
availability of OTA updates is prominently broadcasted and they are easy for
users to install, they are not mandatory.

All contributions must be licensed under Apache License 2.0 unless explicitly
stated otherwise.

# Testing

Both branches and PR's are tested on
[Travis](https://travis-ci.org/SHA2017-badge). Note that due to the nature of
the project there travis only compiles the project, there is no extensive test
suite - so careful reviewing and testing remains vitally important.

# Reviewing

All pull requests must be reviewed by at least 1 team member before merging,
and preferably tested on a real badge.

# Working with submodules

We make extensive use of submodules. While this makes it much easier for
participants to check out and build a consistent version of the software, it
introduces some room for error when accepting contributions. To minimize this
risk we use the following workflow when contributing changes that touch
submodules:

* create a branch in the submodule
  * make changes
  * push and PR to `master` of this submodule
  * merge after reviewing the PR
* then only after that is merged: 
  * create a branch in the main module
  * change the submodule reference to point to the new submodule master
  * make any other changes to the main project as needed
  * push and PR to `master` of the main module
  * merge after Travis is green and the PR is reviewed

There are 2 special cases to this workflow:

1 if the PR to `master` contains no other changes except the updated reference
to the submodule, it can be merged without further review as soon as Travis is green. After all, the change has been reviewed when merging the PR to the submodule.
2 For complex changes it may be useful to temporarily point a branch in the main module to a branch of the submodule. This should be prominently mentioned in any PR though, and these should be merged in the following order:

* merge the PR in the submodule
* update the PR in the main module to point to the (new) master of the submodule
* merge the PR in the main module

This may seem convoluted, but helps avoid mistakes with pointers to unmerged
branches of submodules or pointers to branches of submodules that are merged
but don't have all (other) commits that are on the submodule master.

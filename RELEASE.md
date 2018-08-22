# Release

Steps for releasing a new version of Pointcloud.

Add a new section to the [NEWS](NEWS) file, listing all the changes associated with the new release.

Change the version number in the [Version.config](Version.config) and
[pgsql/expected/pointcloud.out](pgsql/expected/pointcloud.out) files.

Update the value of `UPGRADABLE` in [pgsql/Makefile.in](pgsql/Makefile.in). This variable defines
the versions from which a database can be upgraded to the new Pointcloud version.

Create a PR with these changes.

When the PR is merged create a tag for the new release and push it to GitHub:

```bash
$ git tag -a vx.y.z -m 'version x.y.z'
$ git push origin vx.y.z
```

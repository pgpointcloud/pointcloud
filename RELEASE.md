# Release

Steps for releasing a new version of Pointcloud.

Add a new section to the [NEWS](NEWS) file, listing all the changes associated with the new release.

Change the version number in the [Version.config](Version.config) and
[pgsql/expected/pointcloud.out](pgsql/expected/pointcloud.out) files.

Create a PR with these changes. When the PR is merged create a tag for the new release and push
it to GitHub:

```bash
$ git tag -a x.y.z -m 'version x.y.z'
$ git push origin x.y.z
```

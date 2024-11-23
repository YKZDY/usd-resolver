# Developing

## Building

- clone, cd into repo root
- Linux:
  - If you don't have docker installed:
    - `./setup.sh`
  - `./build.sh`
- Windows:
  - `build.bat`

A build of `usd-resolver` is dependent on three things:

1. A `usd-flavor` which is a name that indicates a particular build of USD
2. The `usd-ver` which is the version of USD to build, i.e 24.05, 24.08, etc.
3. Finally, the `python-ver` which is the Python version USD was built with, i.e 3.10, 3.11, etc.

Available USD flavors + versions are handled via `repo_usd` which can be found with:

- Linux:
  - `./repo.sh usd --help`
- Windows:
  - `.\repo.bat usd --help`

For instance, if you would like to build `usd-resolver` against a specific USD flavor + version you can run the follow:

- Linux:
  - `./build.sh --usd-flavor usd --usd-ver 24.05 --python-ver 3.11`
- Windows:
  - `.\build.bat --usd-flavor usd --usd-ver 24.05 --python-ver 3.11`

If you have a local build of USD that you would like to build against you can use the following:

- Linux:
  - ` ./build.sh --local-path /some/path/to/usd --no-docker`
- Windows:
  - `.\build.bat --local-path C:\some\path\to\usd`

> On Linux, the --no-docker flag is needed as `repo_build` uses docker by default and the path set by `--local-path` will not be mounted. If you do want to use docker with your local build of USD you can manually mount it in `repo.toml` with:

```
[repo_build.docker]

extra_volumes = ["/some/path/to/usd"]
```

> If you would like to build `usd-resolver` in your own docker container, the default image used by `repo_build` can be manually overriden in `repo.toml`:

```
[repo_build.docker]

image_url = "DOCKER_IMAGE:TAG"
```

> Or via environment variable:

- Linux:
  - `export OV_LINBUILD_IMAGE=${DOCKER_IMAGE:TAG}`


## Branches & Merge Requests

The main branch is named "main". Team City automatically builds all changes from this branch and publishes the artifacts to packman.

Do not submit anything directly to the "main" branch. Submit to a branch, then open a merge request. Prefer branches over forks.

Branches must be named either "feature/OMPE-1234" or "bugfix/OMPE-1234" where "OMPE-1234" is the Jira ticket. All changes *must* have an
associated Jira ticket.

## Testing

Tests are run using `repo_test` by simply calling `./test.sh` on Linux or `.\test.bat` on Windows

## Versioning

The build scripts read the first line from CHANGELOG.md to determine the version number

The version number must be 3 numbers seperated by periods:
{major}.{release}.{hotfix}

The major number must change if we make non-backwards-compatible changes.
This is any change that causes previously-working code to no longer work.
Please avoid making these kinds of changes.

The release number changes with every release (see below).

The hotfix number changes with every hotfix (see below).

## Releases

We release a new version of the plugin periodically.
We used to produce a new version number with every change, but it became extremely difficult to keep track of the version numbers.
Releasing a new version consists of tagging the current main branch in git, then updating CHANGELOG.md with a new minor number.

## Hot Fixes

If a fix needs to be back-ported to an older release:
1. Create a branch from the original release tag named "hotfix/{major}.{release}"
2. Make your changes and increase the "hotfix" number in CHANGELOG.md
3. Create a branch named "bugfix/OM-1234-hotfix" and create a merge request into the hotfix branch.
4. Merge once approved & pipelines pass

The build number will look like:
{major}.{release}.{hotfix}-hotfix.{build}+{builder}.{githash}

For example:
1.34.1-hotfix.2508+tc.fefefefefefefe

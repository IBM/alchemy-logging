# ADR: Releases

**Author**: Gabe Goodhart
**Date**: 8/13/2021

<details>
<summary>
TABLE OF CONTENTS
</summary>

- [Objective](#objective)
- [Motivation](#motivation)
- [User Benefit](#user-benefit)
- [Design Proposal](#design-proposal)
  - [Tagging Releases](#tagging-releases)
  - [Semantic Versioning](#semantic-versioning)
  - [Version Publication](#version-publication)
  - [Alternatives Considered](#alternatives-considered)
  - [Dependencies](#dependencies)
  - [Engineering Impact](#engineering-impact)
  - [Best Practices](#best-practices)
  - [User Impact](#user-impact)
- [Questions and Discussion Topics](#questions-and-discussion-topics)

</details>

## Objective

This ADR establishes the release process for all implementations of the Alchemy Logging framework within this repo. The intent is to have a repeatable process for each language that conforms to the standards of that language while playing nicely in the monorepo.

## Motivation

In order for the various language-specific implementations to be consumed natively, they must each be available via standard package management solutions for the given languages. Publishing these releases must be repeatable and consistent so that changes are rolled out in an orderly fashion and conform to versioning standards within the given language's package community.

## User Benefit

Users will be able to consume `alog` via their standard package managers! They will also be able to rely on the semantic meaning of the releases as they would expect.

## Design Proposal

### Tagging Releases

* Each [github release](https://github.com/IBM/alchemy-logging/releases) on this repo will be scoped to a single language implementation. Scoping will be accomplished via the name of the `git tag`.
* This scoping will be managed by a [top level `release.sh` script](https://github.com/IBM/alchemy-logging/blob/main/ci/release.sh) in order to dispatch to the correct language's release process.
* A single [github workflow](https://github.com/IBM/alchemy-logging/blob/main/.github/workflows/release.yml) will handle all releases and run the top-level `release.sh` script
* The scoping rules will be as follows:
  * For `python`, `typescript`, and `c++`, and any future implementation that allows it, the tag format will be `[prefix]-[semantic version]`. The prefixes are `py`, `ts`, and `cpp`.
  * For `go`, the `go.mod` package manager requires [tags of a specific format](https://blog.golang.org/publishing-go-modules#TOC_3.). Specifically, the tag [must have a prefix](https://golang.org/ref/mod#vcs-version) pointing to the location of the `go.mod` file. We will follow this convention in `alchemy-logging`.

### Semantic Versioning

Each implementation will follow standard [semantic versioning](https://semver.org/) practices. Where appropriate, prerelease metadata can be attached to the `patch` version according to the standards of the given language.

### Version Publication

The following publication targets will be used:

* `python`: The python package will be [hosted on pypi](https://pypi.org/project/alchemy-logging/)
  * **NOTE**: There is a naming collision with the [existing `alog` package](https://pypi.org/project/alog/), so the project name is `alchemy-logging` while the imported module is `alog`.
* `typescript`: The javascript/typescript package will be [hosted on npm](https://www.npmjs.com/package/alchemy-logging)
* `go`: The `go` package will use [the `go.mod` standard](https://blog.golang.org/using-go-modules) and be hosted directly from this github repository.
* `c++`: The `c++` package will use [the `cmake` CPM project](https://github.com/cpm-cmake/CPM.cmake)
  * Additionally, users may build and install directly from source using standard `cmake`

### Alternatives Considered

* We could version the entire repo with a single version number
  * **PROS**:
    * This would make for a much simpler landscape of `git tags`
    * It would enforce a tighter coupling between the implementation versions
  * **CONS**:
    * It would either result in many unnecessary version bumps to versions in languages that have not changed or in skipped version values if only publishing when a given language's implementation has changed
* We could split up the monorepo and implement individual repos for each language implementation
  * **PROS**:
    * Each implementation would feel more native to the respective language
    * Versioning in each independent repo would feel standard
  * **CONS**:
    * It would introduce difficult maintainence issues in keeping implementations standardized
    * It would minimize the primary value proposition of the framework which is that it is a single framework implemented across multiple languages

### Dependencies

* This proposal depends on using `Github Actions` to implement the release workflow

### Engineering Impact

* The responsibility for executing the releases will fall on the codeowners

### Best Practices

* When contributing a change to the project, the Pull Request must indicate which implementation(s) it impacts and what level of semantic version change it requires.

### User Impact

* This ADR will allow users to consume `alog` the way they want
* It will (or has been) rolled out on a per-language basis

## Questions and Discussion Topics

* For the `c++` implementation, we could publish an `.rpm` and/or a `.deb` for various target architecture. Should we? The library is lightweight, but building it is a pain with the boost dependency unless we can solve that and make it lightweight.
* For `typescript`, should we consider also hosting it on [githup package repository](https://github.com/features/packages)?
* For `go`, it _may_ be possible to use [replace directives](https://golang.org/ref/mod#go-mod-file-replace) or some other mechanism to avoid the need for the `/src/go` suffix when importing the module. Is it worth investigation?
* For `python`, we could fully switch the module to be `alchemy_logging`, thus avoiding the collision with [existing `alog` package](https://pypi.org/project/alog/). This would break internal code that imports `alog`, but would likely be a friendlier stance in the community. Is that worth doing?

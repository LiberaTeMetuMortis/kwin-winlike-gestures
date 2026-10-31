#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 LiberaTeMetuMortis <78408599+LiberaTeMetuMortis@users.noreply.github.com>
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Make a release:  ./release.sh 0.2.0
#
# Builds and tests, turns the commit messages since the last v* tag into a
# packaging/kwin-winlike-gestures.changes entry (shown to you first), commits
# it, tags vX.Y.Z and pushes both. GitHub Actions then builds the tag and, if
# it passes, tells OBS to publish the new version.
set -euo pipefail

cd "$(dirname "$0")"

version=${1:-}
changes=packaging/kwin-winlike-gestures.changes

die() { echo "release: $*" >&2; exit 1; }

[[ $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || die "usage: ./release.sh X.Y.Z   (e.g. ./release.sh 0.2.0)"
tag=v$version

echo "==> Checking the repository"
[[ $(git branch --show-current) == main ]] || die "switch to the main branch first"
[[ -z $(git status --porcelain) ]] || die "commit or stash your changes first (git status)"
git fetch -q --tags origin
git merge-base --is-ancestor origin/main HEAD || die "main is behind GitHub; run git pull first"
git rev-parse -q --verify "refs/tags/$tag" >/dev/null && die "tag $tag already exists"

last_tag=$(git describe --tags --abbrev=0 --match 'v*' 2>/dev/null || true)
[[ -n $last_tag ]] || die "no previous v* tag found"
if [[ $(printf '%s\n%s\n' "${last_tag#v}" "$version" | sort -V | tail -n1) != "$version" || ${last_tag#v} == "$version" ]]; then
    die "$version is not newer than the last release ($last_tag)"
fi

mapfile -t subjects < <(git log --no-merges --reverse --format='%s' "$last_tag..HEAD")
(( ${#subjects[@]} > 0 )) || die "no commits since $last_tag, nothing to release"

echo "==> Building and testing"
if ! configure_log=$(cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=RelWithDebInfo 2>&1); then
    echo "$configure_log" >&2
    die "configure failed"
fi
cmake --build build -j"$(nproc)" >/dev/null || die "build failed (run cmake --build build to see why)"
ctest --test-dir build --output-on-failure >/dev/null || die "tests failed (run ctest --test-dir build --output-on-failure)"

# openSUSE .changes format: 67 dashes, "date - author", blank line, entries.
entry=$(mktemp)
trap 'rm -f "$entry"' EXIT
{
    printf -- '-------------------------------------------------------------------\n'
    printf '%s - %s <%s>\n\n' "$(LC_ALL=C date -u '+%a %b %d %H:%M:%S UTC %Y')" \
        "$(git config user.name)" "$(git config user.email)"
    printf -- '- Update to version %s:\n' "$version"
    for s in "${subjects[@]}"; do
        printf '  * %s\n' "$s"
    done
    printf '\n'
} > "$entry"

echo
echo "Changelog entry for $tag (from the commits since $last_tag):"
echo
cat "$entry"
while true; do
    read -r -p "Release $tag with this entry? [y]es / [e]dit / [n]o: " answer
    case $answer in
        y|Y|yes) break ;;
        e|E|edit) "${VISUAL:-${EDITOR:-nano}}" "$entry" ;;
        *) die "cancelled, nothing changed" ;;
    esac
    echo; cat "$entry"
done

cat "$entry" "$changes" > "$changes.new" && mv "$changes.new" "$changes"
git add "$changes"
git commit -q -m "Release $version"
git tag -a "$tag" -m "kwin-winlike-gestures $version"

echo "==> Pushing main and $tag"
# Atomic: GitHub gets both or neither, so the tag is always the tip of main.
git push --atomic origin main "$tag"

repo=$(git remote get-url origin | sed -E 's#^git@github.com:#https://github.com/#; s#\.git$##')
echo
echo "Released $tag. Next, automatically:"
echo "  1. GitHub builds and tests it:  $repo/actions"
echo "  2. If that passes, OBS publishes it:"
echo "     https://build.opensuse.org/package/show/home:MetuMortis/kwin-winlike-gestures"

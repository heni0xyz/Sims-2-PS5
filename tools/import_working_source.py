# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Export committed development trees into a fresh source-only staging folder.

No Git history, submodule fetch, compiled title, owner data or network upload.
Complete local SDK source is vendored so unpublished Git SHAs are provenance,
not dependencies that another developer must fetch.
"""
import argparse
import hashlib
import importlib.util
import io
import json
import posixpath
from pathlib import Path
import subprocess
import tarfile


def source_bytes(archive, member, seen=None):
    seen = set() if seen is None else seen
    if member.name in seen or len(seen) >= 32:
        raise ValueError('Cyclic source link: '+member.name)
    seen.add(member.name)
    if member.isfile():
        return archive.extractfile(member).read()
    if not (member.issym() or member.islnk()):
        raise ValueError('Special source entry: '+member.name)
    target = posixpath.normpath(posixpath.join(posixpath.dirname(member.name), member.linkname) if member.issym() else member.linkname)
    if target.startswith(('/', '../')) or target == '..' or ':' in target or '\\' in target:
        raise ValueError('Source link escapes archive: '+member.name)
    return source_bytes(archive, archive.getmember(target), seen)


def preserve_source_bytes(output, records):
    # Nested attribute files override their parents. Preserve archived bytes in
    # every component, including mixed-line-ending upstream source on Windows.
    for record in records.values():
        record['export_overrides'] = {}
        attributes = {n for n in record['files'] if Path(n).name == '.gitattributes'} | {'.gitattributes'}
        for name in sorted(attributes):
            path = output/record['archive_prefix']/name
            original = record['files'].get(name)
            data = path.read_bytes() if path.exists() else b''
            data += b'\n# Source snapshot: preserve exact bytes across checkouts.\n* -text\n'
            path.write_bytes(data)
            record['export_overrides'][name] = dict(source_sha256=original['sha256'] if original else None,
                reason='Git attributes only; preserve source bytes across Windows checkouts.')
            record['files'][name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), mode=0o644)


def export(project, installer, game_foundation, native_sdk, output):
    spec = importlib.util.spec_from_file_location('export_policy', project/'tools/prepare_source_release.py')
    policy = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(policy)
    components = {
        'project': (project, 'recomp'),
        'sdk': (project/'sdk', 'recomp/sdk'),
        'runtime-sdk': (project/'runtime-sdk', 'recomp/runtime-sdk'),
        'ps5-installer': (installer, 'PS5_Sims2Installer'),
        'ps5-foundation': (game_foundation, 'PS5_Sims2Independent'),
        'native-sdk': (native_sdk, 'vendor/PS5_PayloadSDK'),
    }
    output.mkdir(parents=True, exist_ok=False)
    records = {}
    for name, (repo, prefix) in components.items():
        def git(*args):
            return subprocess.check_output(['git', '-C', str(repo), *args], stderr=subprocess.PIPE)
        if git('status', '--porcelain', '--untracked-files=no').strip():
            raise ValueError('Commit tracked source changes before export: '+name)
        revision = git('rev-parse', 'HEAD').decode().strip()
        record = dict(revision=revision, archive_prefix=prefix, files={}, omitted=[], flattened_links={})
        # Git archive otherwise applies this Windows host's core.autocrlf and
        # changes archived bytes despite the same source commit.
        archive_bytes = git('-c', 'core.autocrlf=false', 'archive', '--format=tar', 'HEAD')
        with tarfile.open(fileobj=io.BytesIO(archive_bytes)) as archive:
            for member in archive:
                if member.isdir():
                    continue
                if not member.isfile() and name != 'native-sdk':
                    raise ValueError('Special source entry: '+member.name)
                if member.name == '.gitmodules' or member.name.startswith('ps5/screenshots/'):
                    record['omitted'].append(member.name)
                    continue
                data = source_bytes(archive, member)
                if not member.isfile():
                    record['flattened_links'][member.name] = member.linkname
                policy_name = member.name
                # FreeBSD's public implementation headers use "private" as a
                # source namespace; it is not the owner's private/ game folder.
                if name == 'native-sdk' and policy_name.startswith('include/freebsd/private/'):
                    policy_name = policy_name.replace('/private/', '/sdk-internal/', 1)
                policy.inspect(policy_name, data)
                path = output/prefix/member.name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
                mode = 0o755 if member.isfile() and member.mode & 0o111 else 0o644
                record['files'][member.name] = dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest(), mode=mode)
        records[name] = record
        print(name, revision, len(record['files']), flush=True)
    lock = json.loads((output/'recomp/repositories.lock.json').read_text())
    if lock['sdk']['revision'] != records['sdk']['revision'] or lock['runtime_sdk']['revision'] != records['runtime-sdk']['revision']:
        raise ValueError('SDK source pin differs from committed tree')
    preserve_source_bytes(output, records)
    origin = dict(schema=2, components={n:{k:v for k,v in r.items() if k!='files'} for n,r in records.items()},
                  layout='Complete SDK source trees included; no unpublished commit fetch is required.',
                  launcher_candidate='8db5af5a5dd85551', native_game_code_included=False)
    (output/'recomp/source-origin.json').write_text(json.dumps(origin, indent=2)+'\n')
    (output/'source-component-manifest.json').write_text(json.dumps(dict(schema=1, components=records), indent=2)+'\n')
    return records


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    for name in ('project', 'installer', 'game-foundation', 'native-sdk', 'output'):
        p.add_argument('--'+name, type=Path, required=True)
    a = p.parse_args()
    export(a.project.resolve(), a.installer.resolve(), a.game_foundation.resolve(), a.native_sdk.resolve(), a.output.resolve())

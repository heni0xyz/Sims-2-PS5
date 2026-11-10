#!/usr/bin/env python3
# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
"""Read a tester's klog without treating missing capture/shutdown as success.

This parser does not contact or launch a console. Keep the original log private;
the compact JSON excludes unrelated kernel records and network addresses.
"""
import argparse
import json
from pathlib import Path
import re

FIELDS = re.compile(r'\b(build|run|event|test|result|code|status|sha256|valid|elapsed|verdict|restart_required|executing_linked_code|generating_game_source|compiling_game_unit|game_unit_count|preparing_native_title|game_code_linked|game_code_executed|units|calls|input|received|expected|regions_before|regions_after|bytes_before|bytes_after|files)=([^\s]+)')
CRASH = re.compile(r'A user thread receives a fatal signal|GPU (?:fault|hang)|VM fault|watchdog_timeout|VK_ERROR_DEVICE_LOST', re.I)


def analyze(text, expected_build=None, require_execution=False, require_generation=False, require_game_unit=False,
            expected_game_units=None, require_native_title=False):
    runs = []
    current = None
    # Segment by the application's worker-start record, not arbitrary kernel
    # process IDs. A log may contain several independent title launches.
    for number, line in enumerate(text.splitlines(), 1):
        if '[SIMS2][LINKPROBE]' not in line and '[SIMS2][COMPILERPROBE]' not in line:
            if current and CRASH.search(line):
                current['fatal_lines'].append(number)
            continue
        fields = dict(FIELDS.findall(line))
        event = fields.get('event')
        if event == 'worker_begin':
            current = dict(build_id=fields.get('build'), run_id=fields.get('run'),
                start_line=number, fatal_lines=[], inputs={}, links={}, outputs={},
                mode='compiler' if '[COMPILERPROBE]' in line else 'linker',
                resources={}, compilations={}, compiled_objects={}, compiler_verdict=None,
                io_verdict=None, verdict=None, shutdown=False, worker_joined=False,
                execution_required=require_execution or fields.get('executing_linked_code') == '1',
                execution_results={}, execution_ends={}, module_unloads={},
                execution_negative=False, execution_verdict=None,
                generation_required=require_generation or fields.get('generating_game_source') == '1',
                generation_files={}, generation_end=None, generation_verdict=None,
                generation_overwrite_refused=False, generation_xbe_verified=False,
                game_unit_required=require_game_unit or expected_game_units is not None or fields.get('compiling_game_unit') == '1',
                game_unit_count=fields.get('game_unit_count', '1'),
                game_unit_ends={}, game_unit_objects={}, game_unit_verdict=None,
                native_required=require_native_title or fields.get('preparing_native_title') == '1',
                native_public={}, native_objects={}, native_end=None, native_artifacts={}, native_verdict=None)
            runs.append(current)
        if not current: continue
        if fields.get('build') and fields['build'] != current['build_id']:
            current['fatal_lines'].append(number)
        if fields.get('run') and fields['run'] != current['run_id']:
            current['fatal_lines'].append(number)
        if CRASH.search(line): current['fatal_lines'].append(number)
        if event == 'input_verified':
            name = re.search(r'\bfile=(\S+)', line)
            if name: current['inputs'][name[1]] = fields.get('valid') == '1'
        elif event == 'link_end':
            current['links'][fields.get('test')] = dict(result=fields.get('result'), elapsed=fields.get('elapsed'))
        elif event == 'resource_verified':
            name = re.search(r'\bfile=(\S+)', line)
            if name: current['resources'][name[1]] = fields.get('valid') == '1'
        elif event == 'compile_end':
            current['compilations'][fields.get('test')] = dict(result=fields.get('result'), elapsed=fields.get('elapsed'))
        elif event == 'compiled_object_verified':
            name = re.search(r'\bpath=(\S+)', line)
            if name and fields.get('valid') == '1': current['compiled_objects'][name[1].rsplit('/', 1)[-1]] = fields.get('sha256')
        elif event == 'compiler_verdict': current['compiler_verdict'] = fields.get('status')
        elif event == 'io_verdict': current['io_verdict'] = fields.get('status')
        elif event == 'execution_result':
            current['execution_results'].setdefault(fields.get('test'), {})[fields.get('input')] = (
                fields.get('valid') == '1' and fields.get('received') == fields.get('expected'))
        elif event == 'execution_end':
            current['execution_ends'][fields.get('test')] = fields.get('valid') == '1' and fields.get('calls') == '16'
        elif event == 'module_unload': current['module_unloads'][fields.get('test')] = fields.get('result') == '0'
        elif event == 'execution_negative': current['execution_negative'] = fields.get('valid') == '1'
        elif event == 'execution_verdict': current['execution_verdict'] = fields
        elif event == 'generation_xbe_verified': current['generation_xbe_verified'] = fields.get('valid') == '1'
        elif event == 'generation_end': current['generation_end'] = fields.get('result')
        elif event == 'generation_file_verified':
            name = re.search(r'\bfile=(\S+)', line)
            if name:
                entries = current['generation_files'].setdefault(name[1], [])
                entries.append(fields.get('sha256') if fields.get('valid') == '1' else None)
        elif event == 'generation_overwrite_refused': current['generation_overwrite_refused'] = fields.get('valid') == '1'
        elif event == 'generation_verdict': current['generation_verdict'] = fields
        elif event == 'game_unit_end': current['game_unit_ends'][fields.get('test', 'x86_recomp.0.cpp')] = fields
        elif event == 'game_unit_object': current['game_unit_objects'][fields.get('test')] = fields
        elif event == 'game_unit_verdict': current['game_unit_verdict'] = fields
        elif event in ('native_diagnostic', 'lld_diagnostic') and current['native_required']:
            name = re.search(r'\bfile=(\S+)', line)
            if name and 'text=public_input_verified ' in line: current['native_public'][name[1]] = fields.get('sha256')
            if name and 'text=private_object_verified ' in line: current['native_objects'][name[1]] = fields.get('sha256')
        elif event == 'native_title_end': current['native_end'] = fields.get('result')
        elif event == 'native_artifact':
            if fields.get('valid') == '1': current['native_artifacts'][fields.get('test')] = fields.get('sha256')
        elif event == 'native_title_verdict': current['native_verdict'] = fields
        elif event == 'output_verified':
            current['outputs'][fields.get('test')] = fields.get('sha256')
        elif event == 'verdict': current['verdict'] = fields.get('status')
        elif event == 'worker_join': current['worker_joined'] = fields.get('result') == '0'
        elif event == 'shutdown': current['shutdown'] = True
        if '[ERROR]' in line: current['fatal_lines'].append(number)
    for run in runs:
        reasons = []
        if expected_build and run['build_id'] != expected_build: reasons.append('wrong_build')
        if run['fatal_lines']: reasons.append('fatal_or_error_records')
        if run['verdict'] != 'PASS': reasons.append('no_passing_application_verdict')
        if run['mode'] == 'linker':
            if set(run['inputs']) != {'entry.o', 'provider.o', 'missing.o'} or not all(run['inputs'].values()):
                reasons.append('inputs_not_verified')
        else:
            required_resources = {'compile_fixture.cpp', 'clang-resource/include/stdint.h',
                'clang-resource/include/stddef.h', 'sdk/target/include/c++/v1/array', 'sdk/target/include/c++/v1/cstdint'}
            if run['execution_required']: required_resources.add('ps5-core.ld')
            if run['generation_required']:
                required_resources.update({'generator/config/game.toml', 'generator/xdk/5849/layout.toml',
                                           'generator/xdk/5849/signatures.toml'})
            if run['game_unit_required']:
                required_resources.update({'game-runtime/cpu/'+name for name in
                    ('x86_ops.h', 'x86_context.h', 'guest_address.h')})
            if set(run['resources']) != required_resources or not all(run['resources'].values()):
                reasons.append('compiler_resources_not_verified')
            for test, expected in [('provider', '0'), ('entry', '0'), ('expected-syntax-error', '1'), ('entry-repeat', '0'), ('missing', '0')]:
                if run['compilations'].get(test, {}).get('result') != expected:
                    reasons.append('compile_'+test+'_not_verified')
            objects = run['compiled_objects']
            if not {'provider.o', 'entry.o', 'entry-repeat.o', 'missing.o'} <= set(objects):
                reasons.append('compiled_objects_not_verified')
            if any(not re.fullmatch('[0-9a-f]{64}', objects.get(name, '') or '')
                   for name in ('provider.o', 'entry.o', 'entry-repeat.o', 'missing.o')):
                reasons.append('compiled_object_hashes_not_verified')
            if not re.fullmatch('[0-9a-f]{64}', objects.get('entry.o', '')) or objects.get('entry.o') != objects.get('entry-repeat.o'):
                reasons.append('compile_repeat_not_identical')
            if run['compiler_verdict'] != 'PASS': reasons.append('compiler_not_passed')
            if run['io_verdict'] != 'PASS': reasons.append('io_not_passed')
            if run['execution_required']:
                for test in ('first', 'repeat'):
                    values = run['execution_results'].get(test, {})
                    if len(values) != 16 or not all(values.values()) or not run['execution_ends'].get(test):
                        reasons.append('execution_'+test+'_not_verified')
                    if not run['module_unloads'].get(test): reasons.append('unload_'+test+'_not_verified')
                verdict = run['execution_verdict'] or {}
                if (verdict.get('status') != 'PASS' or verdict.get('calls') != '32' or
                    verdict.get('regions_before') is None or verdict.get('regions_before') != verdict.get('regions_after') or
                    verdict.get('bytes_before') is None or verdict.get('bytes_before') != verdict.get('bytes_after')):
                    reasons.append('execution_verdict_not_verified')
                if not run['execution_negative']: reasons.append('invalid_module_rejection_not_verified')
            if run['generation_required']:
                if not run['generation_xbe_verified'] or run['generation_end'] != '0':
                    reasons.append('generation_not_completed')
                files = run['generation_files']
                if len(files) != 71 or any(len(values) != 2 or values[0] != values[1] or
                        not re.fullmatch('[0-9a-f]{64}', values[0] or '') for values in files.values()):
                    reasons.append('generated_file_parity_not_verified')
                if not run['generation_overwrite_refused']: reasons.append('generation_overwrite_not_refused')
                verdict = run['generation_verdict'] or {}
                if verdict.get('status') != 'PASS' or verdict.get('files') != '71':
                    reasons.append('generation_verdict_not_verified')
            if run['game_unit_required']:
                if not run['generation_required'] or not run['execution_required']:
                    reasons.append('game_unit_missing_prerequisites')
                count = run['game_unit_count']
                if count not in ('1', '65') or (expected_game_units is not None and count != str(expected_game_units)):
                    reasons.append('game_unit_count_not_verified')
                names = {'x86_recomp.0.cpp'} if count == '1' else {'x86_func_mapping.cpp'} | {
                    'x86_recomp.'+str(n)+'.cpp' for n in range(64)}
                ends = run['game_unit_ends']
                if set(ends) != names or any(ended.get('result') != '0' or ended.get('restart_required') != '0'
                        for ended in ends.values()):
                    reasons.append('game_unit_compile_not_verified')
                if count == '1':
                    if not re.fullmatch('[0-9a-f]{64}', objects.get('real-game-unit.o', '') or ''):
                        reasons.append('game_unit_object_not_verified')
                else:
                    bindings = run['game_unit_objects']
                    if set(bindings) != names or any(record.get('valid') != '1' or
                            not re.fullmatch('[0-9a-f]{64}', record.get('sha256', '')) or
                            record.get('sha256') != objects.get(name[:-4]+'.o')
                            for name, record in bindings.items()):
                        reasons.append('game_unit_object_not_verified')
                verdict = run['game_unit_verdict'] or {}
                if (verdict.get('status') != 'PASS' or verdict.get('units') != count or
                        verdict.get('game_code_linked') != '0' or verdict.get('game_code_executed') != '0'):
                    reasons.append('game_unit_verdict_not_verified')
            if run['native_required']:
                if not run['execution_required'] or run['native_end'] != '0':
                    reasons.append('native_title_not_completed')
                expected_objects = {'x86_func_mapping.o'} | {'x86_recomp.'+str(n)+'.o' for n in range(64)}
                if (set(run['native_objects']) != expected_objects or not run['native_public'] or
                        any(not re.fullmatch('[0-9a-f]{64}', value or '') for value in
                            list(run['native_objects'].values())+list(run['native_public'].values()))):
                    reasons.append('native_inputs_not_verified')
                if set(run['native_artifacts']) != {'elf', 'self'} or any(not re.fullmatch('[0-9a-f]{64}', value or '')
                        for value in run['native_artifacts'].values()):
                    reasons.append('native_artifacts_not_verified')
                verdict = run['native_verdict'] or {}
                if verdict.get('status') != 'PASS' or verdict.get('game_code_linked') != '1' or verdict.get('game_code_executed') != '0':
                    reasons.append('native_verdict_not_verified')
        for test, expected in [('first', '0'), ('expected-unresolved', '1'), ('repeat', '0')]:
            if run['links'].get(test, {}).get('result') != expected: reasons.append('link_'+test+'_not_verified')
        first = run['outputs'].get('first')
        if not first or not re.fullmatch('[0-9a-f]{64}', first) or run['outputs'].get('repeat') != first:
            reasons.append('repeat_output_not_verified')
        if not run['worker_joined']: reasons.append('worker_termination_not_verified')
        if not run['shutdown']: reasons.append('shutdown_request_not_captured')
        run['status'] = 'FAIL' if run['fatal_lines'] or run['verdict'] == 'FAIL' else ('INCOMPLETE' if reasons else 'PASS')
        run['reasons'] = reasons
    return dict(schema=1, kind='console_preparation_probe_capture' if any(run['mode'] == 'compiler' for run in runs) else 'console_linker_probe_capture',
        status=runs[-1]['status'] if runs else 'INCOMPLETE', runs=runs,
        game_ready=False, linked_code_execution_tested=any(run['execution_results'] for run in runs),
        note='PASS covers only the recorded checks for this profile; game-unit modes do not link or execute game code. Shell termination needs kernel/process evidence.')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('log', type=Path)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--build-id')
    p.add_argument('--require-execution', action='store_true')
    p.add_argument('--require-generation', action='store_true')
    p.add_argument('--require-game-unit', action='store_true')
    p.add_argument('--game-unit-count', type=int, choices=(1, 65))
    p.add_argument('--require-native-title', action='store_true')
    a = p.parse_args()
    raw = a.log.read_bytes()
    # PuTTY can save UTF-16; preserve the original rather than rewriting it.
    text = raw.decode('utf-16') if raw.startswith((b'\xff\xfe', b'\xfe\xff')) else raw.decode('utf-8', errors='replace')
    result = analyze(text, a.build_id, a.require_execution, a.require_generation, a.require_game_unit,
                     a.game_unit_count, a.require_native_title)
    a.output.parent.mkdir(parents=True, exist_ok=True)
    a.output.write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(dict(status=result['status'], runs=len(result['runs']), output=str(a.output))))
    return 0 if result['status'] == 'PASS' else 1


if __name__ == '__main__': raise SystemExit(main())

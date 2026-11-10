# Copyright (C) 2026 OpenAI Codex, for szampan
# SPDX-License-Identifier: GPL-3.0-only
import unittest
from analyze_linker_probe import analyze


def complete_capture():
    prefix = '[SIMS2][LINKPROBE][INFO] build=fixture run=test '
    records = ['event=worker_begin']
    records += ['event=input_verified file='+name+' valid=1' for name in ('entry.o', 'provider.o', 'missing.o')]
    records += ['event=link_end test=first result=0', 'event=output_verified test=first sha256='+'a'*64,
                'event=link_end test=expected-unresolved result=1', 'event=link_end test=repeat result=0',
                'event=output_verified test=repeat sha256='+'a'*64, 'event=verdict status=PASS',
                'event=worker_join result=0', 'event=shutdown verdict=0']
    return '\n'.join(prefix+record for record in records)


def compiler_capture():
    prefix = '[SIMS2][COMPILERPROBE][INFO] build=fixture run=compiler-test '
    records = ['event=worker_begin']
    records += ['event=resource_verified file='+name+' valid=1' for name in (
        'compile_fixture.cpp', 'clang-resource/include/stdint.h', 'clang-resource/include/stddef.h',
        'sdk/target/include/c++/v1/array', 'sdk/target/include/c++/v1/cstdint')]
    records += ['event=io_verdict status=PASS']
    records += ['event=compile_end test='+name+' result='+result for name, result in (
        ('provider', '0'), ('entry', '0'), ('expected-syntax-error', '1'), ('entry-repeat', '0'), ('missing', '0'))]
    records += ['event=compiled_object_verified path=/data/run/'+name+' sha256='+'b'*64+' valid=1'
        for name in ('provider.o', 'entry.o', 'entry-repeat.o', 'missing.o')]
    records += ['event=clang_diagnostic text=error: SIMS2_expected_compiler_diagnostic',
        'event=compiler_verdict status=PASS', 'event=link_end test=first result=0',
        'event=output_verified test=first sha256='+'a'*64,
        'event=link_end test=expected-unresolved result=1', 'event=link_end test=repeat result=0',
        'event=output_verified test=repeat sha256='+'a'*64, 'event=verdict status=PASS',
        'event=worker_join result=0', 'event=shutdown verdict=0']
    return '\n'.join(prefix+record for record in records)


def execution_capture():
    prefix = '[SIMS2][COMPILERPROBE][INFO] build=fixture run=compiler-test '
    text = compiler_capture().replace('event=worker_begin', 'event=worker_begin executing_linked_code=1')
    records = ['event=resource_verified file=ps5-core.ld valid=1',
               'event=execution_negative valid=1']
    for label in ('first', 'repeat'):
        records += [f'event=execution_result test={label} input={n:08x} received={n+1:08x} expected={n+1:08x} valid=1' for n in range(16)]
        records += [f'event=module_unload test={label} result=0', f'event=execution_end test={label} calls=16 valid=1']
    records += ['event=execution_verdict status=PASS calls=32 regions_before=0 regions_after=0 bytes_before=0 bytes_after=0']
    return text.replace(prefix+'event=verdict status=PASS', '\n'.join(prefix+r for r in records)+'\n'+prefix+'event=verdict status=PASS')


def generation_capture():
    prefix = '[SIMS2][COMPILERPROBE][INFO] build=fixture run=compiler-test '
    text = execution_capture().replace('event=worker_begin', 'event=worker_begin generating_game_source=1')
    records = ['event=resource_verified file='+name+' valid=1' for name in (
        'generator/config/game.toml', 'generator/xdk/5849/layout.toml', 'generator/xdk/5849/signatures.toml')]
    records += ['event=generation_xbe_verified valid=1', 'event=generation_end result=0']
    for _ in range(2):
        records += [f'event=generation_file_verified file=generated-{n}.cpp sha256='+('d'*64)+' valid=1' for n in range(71)]
    records += ['event=generation_overwrite_refused result=2 valid=1', 'event=generation_verdict status=PASS files=71']
    return text.replace(prefix+'event=verdict status=PASS', '\n'.join(prefix+r for r in records)+'\n'+prefix+'event=verdict status=PASS')


def game_unit_capture():
    prefix = '[SIMS2][COMPILERPROBE][INFO] build=fixture run=compiler-test '
    text = generation_capture().replace('event=worker_begin', 'event=worker_begin compiling_game_unit=1')
    records = ['event=resource_verified file=game-runtime/cpu/'+name+' valid=1' for name in
               ('x86_ops.h', 'x86_context.h', 'guest_address.h')]
    records += ['event=game_unit_end result=0 elapsed=1.0 restart_required=0',
                'event=compiled_object_verified path=/data/run/real-game-unit.o sha256='+'e'*64+' valid=1',
                'event=game_unit_verdict status=PASS units=1 game_code_linked=0 game_code_executed=0']
    return text.replace(prefix+'event=verdict status=PASS', '\n'.join(prefix+r for r in records)+'\n'+prefix+'event=verdict status=PASS')


def all_game_units_capture():
    prefix = '[SIMS2][COMPILERPROBE][INFO] build=fixture run=compiler-test '
    text = game_unit_capture().replace('compiling_game_unit=1', 'compiling_game_unit=1 game_unit_count=65')
    text = '\n'.join(line for line in text.splitlines() if 'event=game_unit_end ' not in line and
        'path=/data/run/real-game-unit.o ' not in line)
    text = text.replace('units=1 game_code_linked', 'units=65 game_code_linked')
    records = []
    for name in ['x86_func_mapping.cpp']+['x86_recomp.'+str(n)+'.cpp' for n in range(64)]:
        records += [f'event=game_unit_end test={name} result=0 elapsed=1.0 restart_required=0',
            'event=compiled_object_verified path=/data/run/'+name[:-4]+'.o sha256='+'e'*64+' valid=1',
            f'event=game_unit_object test={name} sha256='+'e'*64+' valid=1']
    return text.replace(prefix+'event=verdict status=PASS', '\n'.join(prefix+r for r in records)+'\n'+prefix+'event=verdict status=PASS')


def native_title_capture():
    prefix = '[SIMS2][COMPILERPROBE][INFO] build=fixture run=compiler-test '
    text = execution_capture().replace('event=worker_begin', 'event=worker_begin preparing_native_title=1')
    records = ['event=native_diagnostic text=public_input_verified file=native/inputs/public.a sha256='+'c'*64]
    records += ['event=native_diagnostic text=private_object_verified file='+name+' sha256='+'d'*64 for name in
                ['x86_func_mapping.o']+['x86_recomp.'+str(n)+'.o' for n in range(64)]]
    records += ['event=native_title_end result=0 elapsed=10.0',
                'event=native_artifact test=elf sha256='+'e'*64+' valid=1',
                'event=native_artifact test=self sha256='+'f'*64+' valid=1',
                'event=native_title_verdict status=PASS game_code_linked=1 game_code_executed=0']
    return text.replace(prefix+'event=verdict status=PASS', '\n'.join(prefix+r for r in records)+'\n'+prefix+'event=verdict status=PASS')


class CaptureVerdicts(unittest.TestCase):
    def test_complete(self):
        self.assertEqual(analyze(complete_capture(), 'fixture')['status'], 'PASS')

    def test_missing_capture(self):
        self.assertEqual(analyze('')['status'], 'INCOMPLETE')

    def test_truncated_before_quit(self):
        self.assertEqual(analyze(complete_capture().rsplit('\n', 1)[0])['status'], 'INCOMPLETE')

    def test_wrong_build(self):
        self.assertNotEqual(analyze(complete_capture(), 'other')['status'], 'PASS')

    def test_missing_negative_case(self):
        text = '\n'.join(line for line in complete_capture().splitlines() if 'test=expected-unresolved' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_changed_output(self):
        text = complete_capture().replace('test=repeat sha256='+'a'*64, 'test=repeat sha256='+'b'*64)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_kernel_crash_after_success(self):
        self.assertEqual(analyze(complete_capture()+'\nA user thread receives a fatal signal')['status'], 'FAIL')

    def test_latest_incomplete_run_not_hidden(self):
        text = complete_capture()+'\n[SIMS2][LINKPROBE][INFO] build=fixture run=second event=worker_begin'
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_complete_compiler_and_expected_error(self):
        self.assertEqual(analyze(compiler_capture(), 'fixture')['status'], 'PASS')

    def test_linker_only_is_not_compiler_proof(self):
        self.assertEqual(analyze(complete_capture().replace('[LINKPROBE]', '[COMPILERPROBE]'))['status'], 'INCOMPLETE')

    def test_missing_cpp_compile(self):
        text = '\n'.join(line for line in compiler_capture().splitlines() if 'compile_end test=provider ' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_expected_syntax_error_must_fail(self):
        text = compiler_capture().replace('test=expected-syntax-error result=1', 'test=expected-syntax-error result=0')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_missing_standard_header_verification(self):
        text = '\n'.join(line for line in compiler_capture().splitlines() if 'file=sdk/target/include/c++/v1/array ' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_compiled_repeat_must_match(self):
        text = compiler_capture().replace('entry-repeat.o sha256='+'b'*64, 'entry-repeat.o sha256='+'c'*64)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_io_failure_cannot_pass(self):
        text = compiler_capture().replace('[INFO] build=fixture run=compiler-test event=io_verdict status=PASS',
            '[ERROR] build=fixture run=compiler-test event=io_verdict status=FAIL')
        self.assertEqual(analyze(text)['status'], 'FAIL')

    def test_incomplete_compiler_run_after_linker_pass(self):
        text = complete_capture()+'\n[SIMS2][COMPILERPROBE][INFO] build=fixture run=compiler-test event=worker_begin'
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_complete_execution(self):
        result = analyze(execution_capture(), 'fixture', require_execution=True)
        self.assertEqual(result['status'], 'PASS')
        self.assertTrue(result['linked_code_execution_tested'])

    def test_compilation_is_not_execution(self):
        self.assertEqual(analyze(compiler_capture(), require_execution=True)['status'], 'INCOMPLETE')

    def test_execution_missing_oracle_call(self):
        text = '\n'.join(line for line in execution_capture().splitlines() if 'test=repeat input=0000000f' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_execution_wrong_result(self):
        text = execution_capture().replace('received=00000001', 'received=ffffffff')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_execution_memory_leak(self):
        text = execution_capture().replace('regions_after=0', 'regions_after=1')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_execution_needs_unload(self):
        text = execution_capture().replace('event=module_unload test=first result=0', 'event=module_unload test=first result=1')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_generation_complete(self):
        self.assertEqual(analyze(generation_capture(), require_generation=True)['status'], 'PASS')

    def test_execution_does_not_prove_generation(self):
        self.assertEqual(analyze(execution_capture(), require_generation=True)['status'], 'INCOMPLETE')

    def test_generation_missing_file(self):
        text = '\n'.join(line for line in generation_capture().splitlines() if 'file=generated-70.cpp ' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_generation_needs_overwrite_refusal(self):
        text = generation_capture().replace('generation_overwrite_refused result=2 valid=1', 'generation_overwrite_refused result=0 valid=0')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_generation_needs_supported_xbe(self):
        text = generation_capture().replace('generation_xbe_verified valid=1', 'generation_xbe_verified valid=0')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_game_unit_complete(self):
        self.assertEqual(analyze(game_unit_capture(), require_game_unit=True)['status'], 'PASS')

    def test_generation_does_not_prove_game_compilation(self):
        self.assertEqual(analyze(generation_capture(), require_game_unit=True)['status'], 'INCOMPLETE')

    def test_game_unit_failed_compile(self):
        text = game_unit_capture().replace('event=game_unit_end result=0', 'event=game_unit_end result=1')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_game_unit_object_required(self):
        text = '\n'.join(line for line in game_unit_capture().splitlines() if 'path=/data/run/real-game-unit.o ' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_game_unit_cannot_claim_execution(self):
        text = game_unit_capture().replace('game_code_executed=0', 'game_code_executed=1')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_all_game_units_complete(self):
        self.assertEqual(analyze(all_game_units_capture(), expected_game_units=65)['status'], 'PASS')

    def test_one_unit_cannot_prove_all(self):
        self.assertEqual(analyze(game_unit_capture(), expected_game_units=65)['status'], 'INCOMPLETE')

    def test_missing_batch_unit(self):
        text = '\n'.join(line for line in all_game_units_capture().splitlines() if 'test=x86_recomp.63.cpp ' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_batch_object_mismatch(self):
        text = all_game_units_capture().replace('event=game_unit_object test=x86_recomp.63.cpp sha256='+'e'*64,
            'event=game_unit_object test=x86_recomp.63.cpp sha256='+'f'*64)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_batch_bad_count(self):
        self.assertEqual(analyze(all_game_units_capture().replace('game_unit_count=65', 'game_unit_count=64'))['status'], 'INCOMPLETE')

    def test_native_title_complete(self):
        self.assertEqual(analyze(native_title_capture(), require_native_title=True)['status'], 'PASS')

    def test_link_fixture_does_not_prove_native_title(self):
        self.assertEqual(analyze(execution_capture(), require_native_title=True)['status'], 'INCOMPLETE')

    def test_native_title_missing_game_object(self):
        text = '\n'.join(line for line in native_title_capture().splitlines() if 'file=x86_recomp.63.o ' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_native_title_failed_packaging(self):
        text = native_title_capture().replace('native_title_end result=0', 'native_title_end result=1')
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')

    def test_native_title_missing_self(self):
        text = '\n'.join(line for line in native_title_capture().splitlines() if 'native_artifact test=self ' not in line)
        self.assertEqual(analyze(text)['status'], 'INCOMPLETE')


if __name__ == '__main__': unittest.main()

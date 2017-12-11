public_sub_headers = [
'eina_promise.h',
'eina_safety_checks.h',
'eina_error.h',
'eina_debug.h',
'eina_debug_private.h',
'eina_log.h',
'eina_inline_log.x',
'eina_fp.h',
'eina_inline_f32p32.x',
'eina_inline_f16p16.x',
'eina_inline_f8p24.x',
'eina_inline_fp.x',
'eina_hash.h',
'eina_inline_hash.x',
'eina_lalloc.h',
'eina_clist.h',
'eina_inline_clist.x',
'eina_inarray.h',
'eina_inlist.h',
'eina_inline_inlist.x',
'eina_list.h',
'eina_file.h',
'eina_inline_file.x',
'eina_mempool.h',
'eina_module.h',
'eina_rectangle.h',
'eina_types.h',
'eina_array.h',
'eina_counter.h',
'eina_inline_array.x',
'eina_magic.h',
'eina_stringshare.h',
'eina_binshare.h',
'eina_binbuf.h',
'eina_ustringshare.h',
'eina_inline_stringshare.x',
'eina_inline_ustringshare.x',
'eina_inline_list.x',
'eina_accessor.h',
'eina_convert.h',
'eina_rbtree.h',
'eina_benchmark.h',
'eina_inline_rbtree.x',
'eina_inline_mempool.x',
'eina_inline_rectangle.x',
'eina_inline_trash.x',
'eina_thread.h',
'eina_trash.h',
'eina_iterator.h',
'eina_main.h',
'eina_cpu.h',
'eina_inline_cpu.x',
'eina_sched.h',
'eina_tiler.h',
'eina_hamster.h',
'eina_matrixsparse.h',
'eina_inline_tiler.x',
'eina_str.h',
'eina_inline_str.x',
'eina_strbuf.h',
'eina_ustrbuf.h',
'eina_unicode.h',
'eina_quadtree.h',
'eina_simple_xml_parser.h',
'eina_lock.h',
'eina_prefix.h',
'eina_refcount.h',
'eina_mmap.h',
'eina_xattr.h',
'eina_value.h',
'eina_inline_value.x',
'eina_value_util.h',
'eina_inline_value_util.x',
'eina_inline_lock_barrier.x',
'eina_inline_lock_posix.x',
'eina_tmpstr.h',
'eina_alloca.h',
'eina_cow.h',
'eina_inline_unicode.x',
'eina_thread_queue.h',
'eina_matrix.h',
'eina_quad.h',
'eina_crc.h',
'eina_inline_crc.x',
'eina_evlog.h',
'eina_util.h',
'eina_inline_util.x',
'eina_quaternion.h',
'eina_vector.h',
'eina_inline_vector.x',
'eina_bezier.h',
'eina_safepointer.h',
'eina_inline_safepointer.x',
'eina_slice.h',
'eina_inline_slice.x',
'eina_inline_modinfo.x',
'eina_freeq.h',
'eina_slstr.h']

public_headers = [
  'Eina.h'
  #TODO config is missing
]

sources = [
'eina_abi.c',
'eina_accessor.c',
'eina_array.c',
'eina_benchmark.c',
'eina_binbuf.c',
'eina_binshare.c',
'eina_convert.c',
'eina_counter.c',
'eina_cow.c',
'eina_cpu.c',
'eina_crc.c',
'eina_debug.c',
'eina_debug_bt.c',
'eina_debug_bt_file.c',
'eina_debug_chunk.c',
'eina_debug_thread.c',
'eina_debug_cpu.c',
'eina_debug_timer.c',
'eina_error.c',
'eina_evlog.c',
'eina_file_common.h',
'eina_file_common.c',
'eina_fp.c',
'eina_hamster.c',
'eina_hash.c',
'eina_inarray.c',
'eina_inlist.c',
'eina_iterator.c',
'eina_lalloc.c',
'eina_list.c',
'eina_lock.c',
'eina_log.c',
'eina_magic.c',
'eina_main.c',
'eina_matrix.c',
'eina_matrixsparse.c',
'eina_mempool.c',
'eina_mmap.c',
'eina_module.c',
'eina_prefix.c',
'eina_promise.c',
'eina_promise_private.h',
'eina_quad.c',
'eina_quadtree.c',
'eina_rbtree.c',
'eina_rectangle.c',
'eina_safety_checks.c',
'eina_sched.c',
'eina_share_common.c',
'eina_simple_xml_parser.c',
'eina_str.c',
'eina_strbuf.c',
'eina_strbuf_common.c',
'eina_stringshare.c',
'eina_thread.c',
'eina_thread_queue.c',
'eina_tiler.c',
'eina_tmpstr.c',
'eina_unicode.c',
'eina_ustrbuf.c',
'eina_ustringshare.c',
'eina_util.c',
'eina_value.c',
'eina_value_util.c',
'eina_xattr.c',
'eina_private.h',
'eina_share_common.h',
'eina_strbuf_common.h',
'eina_quaternion.c',
'eina_bezier.c',
'eina_safepointer.c',
'eina_freeq.c',
'eina_slstr.c']

if target_machine.system() == 'cygwin'
  sources += 'eina_file_win32.c'
else
  sources += 'eina_file.c'
endif

#FIXME
#weird to have here ... but oooooh well and its not correct aaarg
efl_config_h = configuration_data()
efl_config_h.set('EFL_VERSION_MAJOR', version_major)
efl_config_h.set('EFL_VERSION_MINOR', version_minor)
efl_config_h.set('EFL_VERSION_MICRO', version_micro)
efl_config_h.set('EFL_BUILD_ID', get_option('build-id'))

#FIXME placeholder
efl_config_h.set('EFL_API_EO_DEF', '#define EFL_API_EO_DEF "FIXME NOT IMPLEMENTED"')
efl_config_h.set('EFL_API_LEGACY_DEF', '#define EFL_API_LEGACY_DEF "FIXME NOT IMPLEMENTED"')

efl_config = configure_file(
  input: '../efl/Efl_Config.h.in',
  output: 'Efl_Config.h',
  configuration: efl_config_h)

public_headers += efl_config

#end fixme

unwind = dependency('libunwind')
valgrind = dependency('valgrind')

eina_lib = library('eina', sources,
                   include_directories : config_dir,
                   dependencies: [m, rt, dl, valgrind, unwind, thread_dep, eina_mem_pools],
                   link_args : '-DHAVE_LIBUNWIND',
                   install: true)

eina = declare_dependency(
  include_directories: include_directories('.') ,
  dependencies : [thread_dep],
  link_with: eina_lib,
)

install_headers(public_headers)
install_headers(public_sub_headers, subdir : 'eina')

#!/usr/bin/env python
# coding: utf-8

#
# Скопировать часть библиотеки Boost для проекта Atom
#
# Пример: BCP=bcp tools/scripts/copy_boost.py /home/ilya/opt/programming/atom-project/boost_1_44_0 libs/boost-lib/
#

from fabrico import call_cmd
import os
import shutil


def add_options(parser):
    parser.add_option("--bcp", action="store", dest="bcp", metavar='bcp_path', 
                      help="path to bcp utility", default='bcp')

if __name__ == '__main__':
    from parse_options import parse_options
    options, (boost_src, boost_dst) = parse_options("usage: %prog [options] boost_src boost_dst", add_options, num_args=2)
    bcp = options.bcp

    # политика: вроде как bcp сам не удаляет boost_dst, поэтому работаем по месту
    if os.path.exists(boost_dst):
        # удаляем все кроме своего
        lst = ['LICENSE_1_0.txt', 'README', 'SConscript', 'test_include']
        for fname in os.listdir(boost_dst):
            if not fname in lst:
                fpath = os.path.join(boost_dst, fname)
                print 'rm', fpath
                if os.path.isfile(fpath):
                    os.remove(fpath)
                else:
                    shutil.rmtree(fpath)
    
    cmd = '''%(bcp)s --boost=%(boost_src)s boost/smart_ptr.hpp boost/test boost/function.hpp boost/lambda boost/filesystem \
system boost/regex format boost/foreach.hpp boost/iterator boost/cast.hpp boost/range/reference.hpp \
boost/assign/list_of.hpp boost/assign.hpp %(boost_dst)s''' % locals()
    #print cmd
    call_cmd(cmd, err_msg='bcp failed.')

    # Чистим все неиспользуемое
    def call_in_dst(cmd, rel_cwd=None):
        cwd = boost_dst
        if rel_cwd:
            cwd = os.path.join(boost_dst, rel_cwd)
        call_cmd(cmd, cwd=cwd)

    # I boost/preprocessor
    # 1 - iteration - 1 файл для Function
    call_in_dst('rm -rf `ls | grep -v forward1.hpp`', 'boost/preprocessor/iteration/detail/iter')
    
    # 2 - не gcc
    call_in_dst('rm -rf list/detail/dmc list/detail/edg \
repetition/detail/dmc repetition/detail/edg repetition/detail/msvc \
control/detail/dmc control/detail/edg control/detail/msvc', 'boost/preprocessor')
    
    # 3 - ?
    #call_in_dst('rm -rf boost/preprocessor/seq/fold_left.hpp')
    
    # II boost/mpl
    # 1 - не gcc
    call_in_dst('rm -rf `ls | grep -v gcc`', 'boost/mpl/aux_/preprocessed')
    # 2
    call_in_dst('rm -rf boost/mpl/vector')

    # III boost/type_traits
    call_in_dst('rm -rf is_mem_fun_pointer_tester.hpp is_function_ptr_tester.hpp', 'boost/type_traits/detail')
    
    # IV test
    # runtime - это некий Boost.Param
    #call_in_dst('rm -rf boost/test/utils/runtime')
    call_in_dst('rm -rf libs/test/build')

    ## Bind - для Lambda 2 файла нужны, для Function тоже
    #call_in_dst('rm -rf boost/bind.hpp')
    #call_in_dst('rm -rf `ls | grep -v mem_fn_template.hpp | grep -v mem_fn_cc.hpp | grep -v mem_fn.hpp`', 'boost/bind')

    # V Regex
    # удаляем все исходники не из списка libboost_regex.a
    call_in_dst('rm -rf libs/config libs/regex/build libs/regex/test')

    # VII System
    call_in_dst('rm -rf `ls | grep -v src`', 'libs/system')
    
    # VII Разное
    call_in_dst('rm -rf Jamroot boost.png doc libs/format')
    

    
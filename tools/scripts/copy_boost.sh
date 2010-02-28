#!/bin/sh
#
# Скопировать часть библиотеки Boost для проекта Atom
#
# Пример: BCP=bcp tools/scripts/copy_boost.sh /home/ilya/opt/programming/atom-project/boost-1.33.1 libs/boost-lib/
#

BOOST_SRC=$1
if [ "x$BOOST_SRC" == "x" ]; then
    echo "Give me location of the Boost dir! Exit."
    exit 1;
fi

BOOST_DST=$2
if [ "x$BOOST_DST" == "x" ]; then
    echo "Give me destination to copy Boost! Exit."
    exit 1;
fi
if [ ! -d $BOOST_DST ]; then
    mkdir -p $BOOST_DST
fi

if [ "x$BCP" == "x" ]; then
    BCP=bcp
fi

$BCP --boost=$BOOST_SRC boost/smart_ptr.hpp boost/test boost/function.hpp boost/lambda boost/filesystem boost/regex format $BOOST_DST

###############################
# Чистим все неиспользуемое

CWD=`pwd`

# I boost/preprocessor

# 1 - iteration - 1 файл для Function
cd $BOOST_DST/boost/preprocessor/iteration/detail/iter
rm -rf `ls | grep -v forward1.hpp`
cd $CWD
#rm -rf $BOOST_DST/boost/preprocessor/iteration/detail/iter

# 2 - не gcc
rm -rf $BOOST_DST/boost/preprocessor/list/detail/dmc \
       $BOOST_DST/boost/preprocessor/list/detail/edg

rm -rf $BOOST_DST/boost/preprocessor/repetition/detail/dmc  \
       $BOOST_DST/boost/preprocessor/repetition/detail/edg  \
       $BOOST_DST/boost/preprocessor/repetition/detail/msvc

rm -rf $BOOST_DST/boost/preprocessor/control/detail/dmc  \
       $BOOST_DST/boost/preprocessor/control/detail/edg  \
       $BOOST_DST/boost/preprocessor/control/detail/msvc

# 3 - ?
rm -rf $BOOST_DST/boost/preprocessor/seq/fold_left.hpp

# II boost/mpl
# 1 - не gcc
cd $BOOST_DST/boost/mpl/aux_/preprocessed
rm -rf `ls | grep -v gcc`
cd $CWD

# III boost/type_traits
rm -rf $BOOST_DST/boost/type_traits/detail/is_mem_fun_pointer_tester.hpp \
       $BOOST_DST/boost/type_traits/detail/is_function_ptr_tester.hpp

# IV test & bind & iterator
# runtime - это некий Boost.Param, iterator - не имеющее большого значения дополнение
rm -rf $BOOST_DST/boost/test/utils/runtime $BOOST_DST/boost/test/utils/iterator \
       $BOOST_DST/libs/test/build

# Bind - для Lambda 2 файла нужны
rm -rf $BOOST_DST/boost/bind.hpp
cd $BOOST_DST/boost/bind
rm -rf `ls | grep -v mem_fn_template.hpp | grep -v mem_fn_cc.hpp`
cd $CWD

# Iterator - для Lambda один файл нужен, а для filesystem - много
#cd $BOOST_DST/boost/iterator
#rm -rf `ls | grep -v iterator_traits.hpp`  
#cd $CWD

# V Regex
# удаляем все исходники не из списка libboost_regex.a
rm -rf $BOOST_DST/libs/config $BOOST_DST/libs/thread \
       $BOOST_DST/libs/regex/build $BOOST_DST/libs/regex/test
# thread'ы тоже почему-то копируются зря
rm -rf $BOOST_DST/boost/thread

# VI копируем вспомогательные файлы
cp -t $BOOST_DST tools/boost-tmpl/*

# VII падчим Boost.Lambda, чтобы в PCH можно было использовать
patch -f -p0 < tools/boost-tmpl/lambda_pch.diff
if [ "x$?" != "x0" ]; then
    echo "Error: cant patch Boost!"
fi

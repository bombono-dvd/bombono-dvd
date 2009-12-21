#!/bin/sh
#
# make_menu.sh [ GM options ] <menu-item> <background> <frame-dir> <output>
#

usageversion () 
{
    cat >&2 <<END
Usage: make_menu.sh [ GM options ] <menu-item> <background> <frame-dir> <output>
\t Use '!' with -geometry option is mandatory.
END
}

if [ "$#" -lt "4" ]; then
    usageversion
    exit 1;
fi

###########################################

geometry_opt=
gravity_opt=

while [ "$#" != "4" ]
do
        case "$1" in
        -h)     usageversion; exit 0 ;;

        -geometry) shift; geometry_opt=$1; ;;
        -gravity) shift; gravity_opt=$1; ;;
        #-resize)

        *)      echo >&2 "$progname: unknown option or argument $1"
                usageversion; exit 2 ;;
        esac
        shift
done

if [ "x$geometry_opt" == "x" ]; then
    geometry_opt="325x245!"
    if [ "x$gravity_opt" == "x" ]; then
        gravity_opt="center"
    fi
fi

menu_item=$1
background=$2
frame_dir=$3
output=$4

###########################################

common_opts="-geometry $geometry_opt"

tmp_vframe=`mktemp /tmp/vframe.XXXXXX` || exit 1
gm convert $common_opts $frame_dir/vframe.png $tmp_vframe


if [ "x$gravity_opt" != "x" ]; then
    common_opts="$common_opts -gravity $gravity_opt"
fi

gm composite $common_opts $frame_dir/frame.png $background png:- | gm composite $common_opts $menu_item - $tmp_vframe $output

rm -rf $tmp_vframe

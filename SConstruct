#!/usr/bin/env python
import string
import os
import sys

scripts_path = 'tools/scripts'

sys.path.append(scripts_path)
import BuildVars as BV

#
# Atom's project main SConstruct file
#
# (inspired by Blender's SConstruct file,
#  http://www.blender.org/)
#

BV.Args = ARGUMENTS
BV.Targets = COMMAND_LINE_TARGETS

############################################
# Dependency libraries & Settings

# via pkg-config
RequiredLibs = ['glibmm-2.4', 'libxml++-2.6', 'gtkmm-2.4']

# define path to libs
def SetLibraries(config, user_options_dict):
    for lib in RequiredLibs:
        BV.SetLibInfo(config, user_options_dict, lib)

# 
def AddLibOptions(user_options):
    # we make arg's tuple from list
    args = tuple([ (BV.MakeDictName(lib),) for lib in RequiredLibs ])
    user_options.AddVariables(*args)


# legacy stuff (support for GM is hard => trash)
# to turn on:
# 1. False -> True
# 2. uncomment in src/SConscript
BuildMcomposite = False
if BuildMcomposite:
    RequiredLibs.extend(['mjpegtools', 'GraphicsMagick++'])

# global -pthread
IsThreadedBuild = False
ThreadOpt = ['-pthread']
            
############################################
# Compilers

common_warn_flags = []
cxx_warn_flags    = []
debug_flags   = ['-g', '-O0']
release_flags = ['-O2']
defines       = []

def CalcCommonFlags():
    if BV.IsReenter(CalcCommonFlags):
        return

    global common_warn_flags, cxx_warn_flags, debug_flags, defines
    if BV.IsGccCompiler():
        # GCC
        common_warn_flags = ['-ansi']
        # -Wno-reorder - not to warn if not accurate order in ctor (let compiler do!)
        # :TODO: why 
        #   CXXCOM = $CXX -o $TARGET -c $CXXFLAGS $CCFLAGS ...
        # but not
        #   CXXCOM = $CXX -o $TARGET -c $CCFLAGS $CXXFLAGS ...
        cxx_warn_flags = ['-Wall', '-W', '-Wno-reorder'] # -W == -Wextra
        # Oh, GCC! Just give me the speed. If you don't stop getting slower, I'll go to Clang.
        debug_flags += ['-pipe']
    elif BV.IsClangCompiler():
        common_warn_flags = ['-ansi']
        cxx_warn_flags    = [
            '-Wall', '-Wno-reorder', 
            '-Wno-unused-variable'   # many Boost code throws it like "boost::lambda::_1"
        ]
    elif BV.Cc == 'como' or BV.Cxx == 'como':
        # Comeau compiler
        #
        # Currently used as compile-link check only to conform "clean C++" better.
        # See extended remarks about settings used here
        #

        # don't mix compilers
        BV.Cc  = 'como'
        BV.Cxx = 'como'

        common_warn_flags = \
        [
         # accept defines like #define g_error(...) __VA_ARGS__   
         '--variadic_macros', 
         # 1   do not warn when overloaded virtual function is partially overridden
         # 2,3 type qualifiers are meaningless
         # 4   delete of pointer to incomplete class
         '--diag_suppress=611,21,815,414',
        ]
        cxx_warn_flags = []
        # policy: no XOPEN things meanwhile.
        defines = [ 
                    ('_POSIX_C_SOURCE','200112L'),  # fdopen(), fileno(), ...
                    'BOOST_NO_INT64_T',             # Glibc' int64_t trap for non-gcc compilers
                    '_ISOC99_SOURCE'                # C99 extentions: snprintf()
                  ]

#
# Users CPPFLAGS support
# Because SCons cannot handle -U option (~ #undef) it is done via
# additional variable named POST_CPPFLAGS
#
UserCppFlags = ''

def AdjustConfigOptions(env):
    CalcCommonFlags()

    if BV.IsDebugCfg():
        cflags = debug_flags
    else:
        cflags = release_flags

    ldflags = []
    #if BV.BuildProfile:
    #    cflags += profile_flags
    #    ldflags += profile_flags
    
    env.Replace(CC = BV.Cc)
    env.Replace(CXX = BV.Cxx)
    env.Prepend(CCFLAGS = cflags)
    env.Prepend(LINKFLAGS = ldflags)
    env.Prepend(CPPDEFINES = defines)
    
    # to be able to override previous options (not just to add)
    env.Append(CCFLAGS = BV.CFlags.split())
    env.Append(CXXFLAGS = BV.CxxFlags.split())
    env.Append(LINKFLAGS = BV.LdFlags.split())
    env.Append(POST_CPPFLAGS = UserCppFlags.split())
        
    # :TODO: set them when need
    #env.Replace (PATH = user_options_dict['PATH'])
    #env.Replace (AR = user_options_dict['AR'])

    # Large File Support
    if not BV.IsReenter(AdjustConfigOptions):
        if BV.IsSConsLE_0_96(env):
            # :TRICKY: nobody but me uses so old scons, so I do it straight
            dict = { 'CPPDEFINES': ['_LARGEFILE_SOURCE', ('_FILE_OFFSET_BITS', '64')] }
        else:
            dict = BV.ParseFlagsForCommand('getconf LFS_CFLAGS', 1)
            dict.update(BV.ParseFlagsForCommand('getconf LFS_LDFLAGS'))
            dict.update(BV.ParseFlagsForCommand('getconf LFS_LIBS'))
        AdjustConfigOptions.lfs = dict
    env.Append(**AdjustConfigOptions.lfs)

    # Multithreading, global
    if IsThreadedBuild:
        env.Append(CCFLAGS = ThreadOpt, LINKFLAGS = ThreadOpt)

############################################
# Defaults

BV.CfgFile  = BV.Args.get('CFG_FILE','config.opts')

def_env = Environment(ENV = os.environ)
def_env_dict  = def_env.Dictionary()

user_options_dict = {} 

Variables = BV.Variables
EnumVariable = BV.EnumVariable
BoolVariable = BV.BoolVariable

def ParseVariables(user_options):
    user_options.AddVariables (
            (EnumVariable('BUILD_CFG',
                        'Select release or debug build.', 
                        'release', allowed_values = ('release', 'debug'))),
    		('BUILD_DIR', 'Target directory for intermediate files.',
    					'build'),
            (BoolVariable('BUILD_BRIEF',
                        'Set to 1 if you want brief compiling/linking output.',
                        'false')),
            (EnumVariable('BUILD_QUICK',
                        "Set to 'true' if you want to reduce building time using: \n"
                        "\t1) PCH - precompiled headers \n"
                        "\t2) some SCons perfomance tuning.\n"
                        "\t'auto' implies true for debug build and false for release one.",
                        'auto', allowed_values = ('auto', 'false', 'true'))),
            ('PREFIX',  'Change the default install directory.', '/usr/local'),
            ('DESTDIR', 'Set the intermediate install directory.', ''),
            ('CC', 'C compiler.'),
            ('CXX', 'C++ compiler.'),
            ('CPPFLAGS','Extra CPP flags.', ''),
            ('CFLAGS',  'Extra C Compiler flags (for C++ too).', ''),
            ('CXXFLAGS','Extra C++ Compiler flags.', ''),
            ('LDFLAGS', 'Extra Linker flags.', ''),
            (BoolVariable('TEST',
                        'Set to 1 if you want to build and run tests.',
                        'false')),
            (BoolVariable('USE_EXT_BOOST',
                        'Leave this setting 0 to use embedded Boost library version (recommended).',
                        'false')),
            ('BOOST_INCLUDE', 'Set to include path for external(not embedded) version of the Boost library.', ''),
            ('BOOST_LIBPATH', 'Set to library dir path for external(not embedded) version of the Boost library.', ''),
            ('DVDREAD_INCLUDE', 'Set to include path for libdvdread header files.', ''),
            ('DVDREAD_LIBPATH', 'Set to library path where the libdvdread is located.', ''),
            # Undocumented
            (BoolVariable('TEST_BUILD',
                        'Set to 1 if you want to build with tests (all).',
                        'false')),
            )
    # we need to add 'lib dict vars' user_options to load in user_options_env
    AddLibOptions(user_options)

    # Configuration
    def add_calc_variables(*lst):
        lst = ((name,) for name in lst)
        user_options.AddVariables( *lst )
    add_calc_variables('IS_GCC', 'IS_CLANG', 'CONFIGURATION')

    # make help (scons -h)
    user_options_env = Environment(ENV = os.environ, options = user_options)
    Help(user_options.GenerateHelpText(user_options_env))
    # fill in user_env_dict
    global user_options_dict
    user_options_dict = user_options_env.Dictionary()
    # must be later than assigning
    BV.InitUOD(user_options_dict)
    Export('user_options_dict')
    user_options_dict['AdjustConfigOptions'] = AdjustConfigOptions

    # fill in BuildVars
    BV.BuildCfg = user_options_dict['BUILD_CFG']
    BV.BuildDir = user_options_dict['BUILD_DIR']
    BV.BuildBrief = user_options_dict['BUILD_BRIEF']

    BV.Cc  = user_options_dict['CC']
    BV.Cxx = user_options_dict['CXX']
    global UserCppFlags
    UserCppFlags  = user_options_dict['CPPFLAGS']
    BV.CFlags = user_options_dict['CFLAGS']
    BV.CxxFlags = user_options_dict['CXXFLAGS']
    BV.LdFlags  = user_options_dict['LDFLAGS']
    BV.RunTests = user_options_dict['TEST']
    BV.BuildTests = user_options_dict['TEST_BUILD']

    # make build dir if not exist;
    # need to make manually for .sconsign
    if os.path.isdir(BV.BuildDir) == 0:
    	os.makedirs(BV.BuildDir)
    #SourceSignatures('timestamp')
    SConsignFile(BV.BuildDir+os.sep+'.sconsign')
    # conf dir
    user_options_dict['ConfigDir'] = BV.BuildDir+ '/' + BV.GetSrcDirPath() +'/cfg'
    user_options_dict['DVDREAD_DICT'] = { 'CPPPATH' : [user_options_dict['DVDREAD_INCLUDE']],
                                          'LIBPATH' : [user_options_dict['DVDREAD_LIBPATH']] }

############################################
# Config

if os.path.exists( BV.CfgFile ):
    print "Using config file: " + BV.CfgFile

    user_options = Variables(BV.CfgFile, BV.Args)
    ParseVariables(user_options)

else:
    print "Creating new config file: " + BV.CfgFile

    user_options = Variables(None, BV.Args)
    ParseVariables(user_options)
    try:
        def write_name_value(name, val):
            config.write('%s = %r\n' % (name, val))
        def write_dict_value(name):
            write_name_value(name, user_options_dict[name])

        # save to file
        config = open(BV.CfgFile, 'w')
    
        config.write("# Options for building Atom project\n")
        config.write('BUILD_CFG = %r\n' % (BV.BuildCfg))
        config.write('BUILD_DIR = %r\n' % (BV.BuildDir))
        config.write('BUILD_BRIEF = %r\n' % (BV.BuildBrief))
        write_dict_value('BUILD_QUICK')
        config.write('TEST = %r\n' % (BV.RunTests))
        write_dict_value('PREFIX')
        write_dict_value('DESTDIR')
    
        config.write('\n# Compiler information\n')
        config.write('CC = %r\n' % (BV.Cc))
        config.write('CXX = %r\n' % (BV.Cxx))
        write_name_value('CPPFLAGS', UserCppFlags)
        config.write('CFLAGS = %r\n' % (BV.CFlags))
        config.write('CXXFLAGS = %r\n' % (BV.CxxFlags))
        config.write('LDFLAGS = %r\n' % (BV.LdFlags))

        config.write('\n# Boost library\n')
        write_dict_value('USE_EXT_BOOST')
        write_dict_value('BOOST_INCLUDE')
        write_dict_value('BOOST_LIBPATH')

        config.write('\n# libdvdread library\n')
        write_dict_value('DVDREAD_INCLUDE')
        write_dict_value('DVDREAD_LIBPATH')

        config.write('\n### Calculated data ###\n')
        SConscript('Autoconfig')

        config.write('\n')
        write_dict_value('IS_GCC')
        write_dict_value('IS_CLANG')

        config.write('\n# Configuration\n')
        write_dict_value('CONFIGURATION')

        SetLibraries(config, user_options_dict)

        config.write('\n# Undocumented\n')
        # this one has only one purpose I can say: just build all
        # to test/profile SCons speed
        config.write('TEST_BUILD = %r\n' % (BV.BuildTests))

        config.close()
    except:
        # something went wrong, remove config and exit/raise
        #print 'Error!'
        config.close()
        os.remove(BV.CfgFile)
        # raise is better, we'll see the error and its location
        raise #Exit(1)

# optimization: checking files as make
Decider('timestamp-newer')
BV.VariantDir(BV.BuildDir, '.', duplicate=0)

def GenerateBaseConfigH(target, source, env):
    cfg_file = open(target[0].path, 'w')
    print >> cfg_file, "/* Generated by means of Autoconfig */"

    config_dict = user_options_dict['CONFIGURATION']
    key_list = config_dict.keys()
    key_list.sort()

    for key in key_list:
        var = config_dict[key]

        text = var.get('text', None)
        if text:
            print >> cfg_file
            BV.AddComment(cfg_file, var)
            print >> cfg_file, text
            continue

        BV.AddDefine(cfg_file, key, **var)
    return None

BV.MakeConfigFile( target = user_options_dict['ConfigDir']+"/config.h",
                   source = [BV.CfgFile],
                   gen_function = GenerateBaseConfigH )

############################################
# Main environment

# 1 main env
user_options_dict['LIB_BUILD_DIR'] =  '#'+BV.BuildDir+'/lib'
user_options_dict['BIN_BUILD_DIR'] =  '#'+BV.BuildDir+'/bin'

env  = BV.MakeMainEnv() #Environment(ENV = os.environ)
menv = env.Clone()

CalcCommonFlags()
# 2 set project's compiler flags - warnings and defines
# :TODO: remake via gcc' -include option, see "build todos"
# need for shared_ptr speed (to use quick heap implementation
# for ref count holding)
prj_defines = [
    'BOOST_SP_USE_QUICK_ALLOCATOR',
    'BOOST_SYSTEM_NO_DEPRECATED' # remove warnings for 'boost::system::posix_category' defined but not used ...
]
if not BV.IsDebugCfg():
    prj_defines += ['NDEBUG']

menv.Append(CCFLAGS    = common_warn_flags)
menv.Append(CXXFLAGS   = cxx_warn_flags)
menv.Append(CPPDEFINES = prj_defines)

AdjustConfigOptions(env)
AdjustConfigOptions(menv)
# env for 3rd libs, menv - for our projects
Export('env', 'menv')

# 4 get lib dicts
GetDict = BV.GetDict

def GetGtkmmDict():
    dct = GetDict('gtkmm-2.4')
    if not IsThreadedBuild:
        # make happy gdb (debugger) by linking with multithreaded libc (in advance),
        # cause it doesn't like singlethreaded app turns into multithreaded one
        # in runtime (gtk shared libraries've become multithreaded)
        dct['LINKFLAGS'] = dct.get('LINKFLAGS', []) + ThreadOpt
    return dct

gtk2mm_dict   = GetGtkmmDict()
libxmlpp_dict = GetDict('libxml++-2.6')

############################################
# Environments
#
# :TODO: substitute env creating with distribute one

#
# 3rd libs:
#   mpeg2dec (=libmpeg2)
#   Boost.Test
#
SConscript(BV.BuildDir+'/libs/SConscript')
Import('boost_env', 'boost_test_dict')

# 
# mlib
#
# Depends on ext: Boost, boost-logging, Boost.Filesystem&System
#
mlib_env = boost_env.Clone()
mlib_env.Append(CPPPATH = ['#libs/boost-logging']) #, LIBS = ['pthread']))
# libboost_system.a linking is needed only for static builds
mlib_env.Append( LIBS = ['boost_filesystem', 'boost_system', 'boost_regex'] )
Export('mlib_env')

# 
# mlib/tests
#
# Depends on: mlib
# Depends on ext: Boost.Test
#
mlib_tests_env = mlib_env.Clone()
mlib_tests_env.AppendUnique(**boost_test_dict)
Export('mlib_tests_env')

user_options_dict['DVDREAD_DICT']['LIBS'] = ['dvdread']
def AddMovieDicts(env):
    env.AppendUnique(**user_options_dict['LIBMPEG2_DICT'])
    env.AppendUnique(**user_options_dict['DVDREAD_DICT'])

# 
# mdemux
#
# Depends on: mlib
# Depends on ext: boost, libmpeg2, libdvdread
#
mdemux_env = mlib_env.Clone()
AddMovieDicts(mdemux_env)
Export('mdemux_env')

# 
# mdemux/tests
#
# Depends on: mdemux
# Depends on ext: Boost.Test
#
mdemux_tests_env = mdemux_env.Clone()
mdemux_tests_env.AppendUnique(**boost_test_dict)
Export('mdemux_tests_env')

# 
# mbase
#
# Depends on: mlib
# Depends on ext: glibmm-2.4, libxml++-2.6
#
mbase_env = mlib_env.Clone()
mbase_env.AppendUnique(**GetDict('glibmm-2.4'))
mbase_env.AppendUnique(**libxmlpp_dict)
Export('mbase_env')

# 
# mbase/tests
#
# Depends on: mlib/tests
#
mbase_tests_env = mbase_env.Clone()
mbase_tests_env.AppendUnique(**boost_test_dict)
Export('mbase_tests_env')

# 
# mgui
#
# Depends on: mcomposite, mproject
# Depends on ext: gtkmm-2.4
#
mgui_env = mbase_env.Clone()
AddMovieDicts(mgui_env)
mgui_env.AppendUnique(**gtk2mm_dict)

# if BV.Cc == 'como':
#     mgui_env.Append(CXXFLAGS='--g++')
#     mgui_env.Append(CPPDEFINES=['BOOST_NO_TEMPLATE_PARTIAL_SPECIALIZATION'])
#     pass
Export('mgui_env')

# 
# mgui/tests
#
# Depends on: mlib/tests, mdemux/tests, mgui
#
mgui_tests_env = mgui_env.Clone()
mgui_tests_env.AppendUnique(**boost_test_dict)
Export('mgui_tests_env')

if BuildMcomposite:
    #
    # mcomposite
    #
    # Depends on: mlib, mbase
    # Depends on ext: boost, mjpegtools, GraphicsMagick++
    #
    mcomposite_env = mbase_env.Clone()
    mcomposite_env.AppendUnique(**user_options_dict['LIBMPEG2_DICT'])
    mcomposite_env.AppendUnique(**GetDict('mjpegtools'))
    mcomposite_env.AppendUnique(**GetDict('GraphicsMagick++'))
    Export('mcomposite_env')
    
    # 
    # mcomposite/tests
    #
    # Depends on: mcomposite, mlib/tests
    #
    mcomposite_tests_env = mcomposite_env.Clone()
    mcomposite_tests_env.AppendUnique(**boost_test_dict)
    Export('mcomposite_tests_env')

#
# Installation 
#

prefix = user_options_dict['PREFIX']

dest_dir = str(user_options_dict['DESTDIR'])
def MakeEndPrefix(prefix_name, f_arg, *args, **kw):
    # dest_dir is thrown away because f_arg is an absolute path! 
    #prefix = os.path.join(dest_dir, f_arg, *args)
    prefix = os.path.join(dest_dir + f_arg, *args)
    if (not "add_to_dict" in kw) or kw["add_to_dict"]:
        user_options_dict[prefix_name] = prefix
    return prefix

MakeEndPrefix('DEST_PREFIX', prefix)
bin_prefix  = MakeEndPrefix('BIN_PREFIX',  prefix, 'bin')
data_prefix = MakeEndPrefix('DATA_PREFIX', prefix, 'share', 'bombono', add_to_dict=0)

mgui_env.Alias('install', [bin_prefix, data_prefix])
# resources
BV.InstallDir(mgui_env, data_prefix, "resources")

user_options_dict['XGETTEXT_SOURCES'] = []
def AddSourcesForXgettext(src_files):
    user_options_dict['XGETTEXT_SOURCES'] += [ File(src) for src in src_files ]
user_options_dict['AddSourcesForXgettext'] = AddSourcesForXgettext

SConscript([
            BV.BuildDir+'/SConscript',
            BV.BuildDir+'/src/SConscript',
            BV.BuildDir+'/po/SConscript',
           ])


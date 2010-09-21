# Global variables for Atom build process
import os
import platform

import SCons
from SConsTwin import *

Args = {}
Targets = []
CfgFile = ''

BuildCfg = ''
BuildDir = ''
BuildProfile = 0
# brief compiling/linking output
BuildBrief = 0

Cc = ''
Cxx = ''
CFlags = ''
CxxFlags = ''
LdFlags = ''

# build tests along with building
BuildTests = 0
# run tests along with building
RunTests = 0

# = user_options_dict
UserOptDict = {}

def PrintBright(is_end):
    if not is_end:
        print
        print "****************************************************"
    else:
        print "****************************************************"
        print

def IsDebugCfg():
    return BuildCfg == 'debug'

def IsReenter(function_name):
    """ return false only if called in (function) 'function_name' for the first time """
    res = '__OnceEntered' in dir(function_name)
    function_name.__OnceEntered = 1

    return res

########################################################################
# Version function

def IsSConsGE_0_96_92(env):
    return IsSConsVersionGE((0, 96, 92))

def IsSConsGE_0_96_93():
    return IsSConsVersionGE((0, 96, 93))

def DoSConsUseSTREnvs(env):
    """ for versions ~0.96.94 and above we can set output 
    with *STR variables """
    return IsSConsGE_0_96_92(env)

########################################################################

def CheckSettings(main_env):
    global Cc, Cxx, BuildDir, Targets, RunTests, BuildTests
    if RunTests :
        print 'Building and running tests'
    else:
        if BuildTests:
            print 'Building with tests'

    def_env = GetDefEnv()
    # for SCons =<0.96 we need to warn that just 'scons' is not enough:
    # - 0.96.1 have that _bug_
    # - 0.96.94 have not, so with >0.96 we are free
    #if '_get_major_minor' in dir(def_env) and '__version__' in dir(SCons):
    #    v_major, v_minor = def_env._get_major_minor(SCons.__version__)
    #    if(v_major, v_minor) <= (0, 96) and \
    #      ( (len(BuildDir) >= 1 and BuildDir[0] == '/') or (len(BuildDir) >= 2 and BuildDir[0:2] == '..') ) and \
    #      (Targets == [] or Targets == ['.']) :
    #        PrintBright(0)
    #        print 'Warning! "scons" or "scons ." detected while BUILD_DIR is outside "."!'
    #        print 'To build successfully with SCons <= v0.96.1 you may need to run something like '
    #        print '\t"scons <...> ' + BuildDir + '".' 
    #        PrintBright(1)
    #:TO_TEST:
    if IsSConsLE_0_96(def_env) and \
      ( (len(BuildDir) >= 1 and BuildDir[0] == '/') or (len(BuildDir) >= 2 and BuildDir[0:2] == '..') ) and \
      (Targets == [] or Targets == ['.']) :
        PrintBright(0)
        print 'Warning! "scons" or "scons ." detected while BUILD_DIR is outside "."!'
        print 'To build successfully with SCons <= v0.96.1 you may need to run something like '
        print '\t"scons <...> ' + BuildDir + '".' 
        PrintBright(1)

    # we use TestSConscript() function instead of SConscript()
    # to run tests 
    def TestSConscript(*args, **kwds):
        if BuildTests or RunTests:
            def_env.SConscript(*args, **kwds)
    def_env.Export('TestSConscript')
    # make test Builder
    if RunTests:
        #def generate_test_actions(source, target, env, for_signature):    
        #    return '%s' % (source[0])
        #test_bld = main_env.Builder(generator = generate_test_actions)
        #
        #def TestingString(target, source, env):
        #    return '\nRunning tests: %s' % (source[0])
        #test_bld.action.strfunction = TestingString

        # :TODO: now all tests have the same args; should be fixed
        # by storing dict of (test_bin_Node, args) and using it in make_test_str()
        #
        # --catch_system_errors=no is needed because Boost.Test of new Boost versions conflicts
        # with other software (e.g. GTK) by handling signals its own way.
        # Attention: UNIX signals are CRAP!!! I hate you.
        UT_ARGS = '--catch_system_errors=no'

        def make_test_str(source, is_cmd):
            res = '%s' % (source[0])
            if is_cmd or (not BuildBrief):
                if UT_ARGS:
                    res += ' ' + UT_ARGS 
            return res

        def generate_test_actions(source, target, env, for_signature):
            return make_test_str(source, True)

        def generate_test_str(target, source, env):
            return '\nRunning tests: %s' % (make_test_str(source, False))

        test_act = MakeGenAction(generate_test_actions, generate_test_str)
        test_bld = main_env.Builder(action = test_act)

        main_env.Append( BUILDERS = {'UnitTest_': test_bld} )

    def UnitTest(test_program_name, env):
        if RunTests:
            # use non-existent target to run command always
            env.UnitTest_(target = '_fiction_target_', source = test_program_name)
    main_env.Export('UnitTest')

    # set brief output
    SetBriefOutput(main_env)

    # to separate our output from SCons'
    print

# Non/Verbose output
def SetBriefOutput(env):
    if BuildBrief :
        def GetBaseNameFromTarget(target):
            target_path = str(target[0]) # target[0] is not string somehow
            return os.path.basename(target_path)

        def ArhievingString(target, source, env, **kw):
            return '\nMaking library %s ...' % (GetBaseNameFromTarget(target))

        def LinkingString(target, source, env, **kw):
            #target_path = str(target[0]) # target[0] is not string somehow
            return '\nLinking %s ...\n' % (GetBaseNameFromTarget(target))

        def _InstallString(dest, source, env, **kw):
            # :TODO: do for faked PCH only
            return ' '
        InstallString = _InstallString
        # for >= 0.98.1 strings can be set up only
        if IsSConsVersionGE((0, 98, 1)):
            InstallString = ' '

        def GCHString(target, source, env, **kw):
            return 'Making precompiled header %s ...' % (target[0])

        if not DoSConsUseSTREnvs(env):
            # :WARNING: This is not official way to change output!
            import SCons.Action
            #import SCons.Builder
        
            def CompilingString(target, source, env):
                return '%s ...' % (source[0])
    
            # C
            SCons.Defaults.CAction.strfunction = CompilingString
            SCons.Defaults.ShCAction.strfunction = CompilingString
            # C++
            SCons.Defaults.CXXAction.strfunction = CompilingString
            SCons.Defaults.ShCXXAction.strfunction = CompilingString
            # Arhieving
            SCons.Defaults.ArAction.strfunction = ArhievingString
            # Linking
            SCons.Defaults.LinkAction.strfunction = LinkingString
            SCons.Defaults.ShLinkAction.strfunction = LinkingString
            # Installing
            SCons.Environment.installAction.strfunction = InstallString
            # PCH
            import gch
            gch.GchAction.strfunction = GCHString
            gch.GchShAction.strfunction = GCHString
        
            # not work - some other Action is being returned
            #print env['BUILDERS']['CXXFile'].action.strfunction #= installString
        else:
            src_str = '$SOURCES ...'

            # C
            env['CCCOMSTR']   = src_str
            env['SHCCCOMSTR'] = src_str
            # C++
            env['CXXCOMSTR']   = src_str
            env['SHCXXCOMSTR'] = src_str
            # Arhieving
            env['ARCOMSTR'] = ArhievingString
            #env['RANLIBCOMSTR'] = lambda target, **kw: 'Ranlib '+GetBaseNameFromTarget(target)+' ...'
            env['RANLIBCOMSTR'] = '...'
            # Linking
            env['LINKCOMSTR'] = LinkingString
            env['SHLINKCOMSTR'] = LinkingString
            # Installing
            env['INSTALLSTR'] = InstallString
            # PCH
            env['GCHCOMSTR'] = GCHString
            env['GCHSHCOMSTR'] = GCHString

def ErrorAndExit(msg):
    PrintBright(0)
    print msg
    PrintBright(1)

    GetDefEnv().Exit(1)

def ParseFlagsForCommand(cmd, add_defines = 0):
    tmp_env = GetDefEnv().Clone()
    # as for SCons 1.2.0, ENV is "empty" for default environment
    tmp_env['ENV'] = os.environ

    ### we need to strip pkg-config output to remove \n
    ##mjpeg_cflags = os.popen('pkg-config --cflags mjpegtools').read().strip()
    ##mjpeg_libs   = os.popen('pkg-config --libs mjpegtools').read().strip()
    ##mjpeg_env.Append(CPPFLAGS = mjpeg_cflags)
    # more right way to do that

    tmp_env.ParseConfig(cmd)
    tmp_dict = tmp_env.Dictionary()

    #res_dict = { 'CPPPATH' : tmp_dict['CPPPATH'],
    #             'LIBPATH' : tmp_dict['LIBPATH'],
    #             'LIBS'    : tmp_dict['LIBS']     }
    res_dict = {}
    keys_list = ['CPPPATH', 'LIBPATH', 'LIBS']
    if add_defines:
        keys_list.append('CPPDEFINES')
    for key in keys_list:
        if key in tmp_dict.keys():
            res_dict[key] = tmp_dict[key]

    if Cc != 'como':
        # need to add link flags like "-Wl,--rpath -Wl,<path>"
        if 'LIBPATH' in res_dict.keys():
            # we dont want to change dict['LIBPATH'] so we need a copy
            #lib_list = []
            #lib_list[:] = dict['LIBPATH']
            lib_list = list(res_dict['LIBPATH'])
            # but we dont want to "rpath" anything like /usr/X11R6/lib
            for i in xrange(len(lib_list)-1, -1, -1) :
                if lib_list[i].startswith('/usr/') :
                    #print i    
                    lib_list[i:i+1] = []

            tmp_env.AppendUnique(RPATH = lib_list)
            res_dict['RPATH'] = tmp_dict['RPATH']

    return res_dict

def GetLIBFlags(package_name):
    ''' Get package_name's compile flags using pkg-config 
        Return dictionary full of CPPPATH, LIBPATH, ... '''

    res = os.system('pkg-config --exists --print-errors '+ package_name)
    if res:
        msg = "pkg-config: Can't find library '" + package_name + "'!\n" \
              "Please check if this package is in $PKG_CONFIG_PATH."
        ErrorAndExit(msg)
    return ParseFlagsForCommand('pkg-config --cflags --libs ' + package_name)

################################################################
# New syntax support

if not IsSConsGE_0_96_93():
    # Copy is depricated ... but versions < 0.96.93 dont know Clone, just good old Copy
    SCons.Environment.Base.Clone = SCons.Environment.Base.Copy

# emulating Variable class for versions < 0.98.1
IsVariablesNow = IsSConsVersionGE((0, 98, 1))
if IsVariablesNow:
    EnumVariable = SCons.Variables.EnumVariable
    BoolVariable = SCons.Variables.BoolVariable
else:
    # emulate Variables class via Options
    SCons.Options.Options.AddVariables    = SCons.Options.Options.AddOptions

    EnumVariable = SCons.Options.EnumOption
    BoolVariable = SCons.Options.BoolOption

def Variables(*args, **kw):
    if IsVariablesNow:
        return SCons.Variables.Variables(*args, **kw)
    else:
        return SCons.Options.Options(*args, **kw)

################################################################
# Autoconfig stuff

def GetArch():
    typ = platform.processor()
    if typ == '':
        typ = platform.machine()
    return typ

def IsX86Arch():
    str = GetArch()
    return len(str) >= 4 and str[2:4] == '86'

def IsPPCArch():
    str = GetArch().lower()
    return str.startswith('ppc') or str.startswith('powerpc')

def IsSparcArch():
    str = GetArch().lower()
    return str.startswith('sparc')

def IsAlphaArch():
    str = GetArch().lower()
    return str.startswith('alpha')

def IsGccLike():
    # all those compilers who understand gcc extensions: gcc, clang
    return UserOptDict['IS_GCC']

def IsGccCompiler():
    #return Cc == 'gcc'
    return UserOptDict['IS_GCC'] and not UserOptDict['IS_CLANG']

def IsClangCompiler():
    def has_tail(name, tail):
        ln = len(name) - len(tail)
        return name[ln:] == tail

    #res = has_tail(BV.Cc, 'clang')
    res = UserOptDict['IS_CLANG']
    if res:
        assert( has_tail(Cxx, 'clang++') )
    return res

def MakeObjectComment(obj_name, is_one_set):
    prefix = "Define "
    if is_one_set:
        prefix += "to 1 "

    return prefix + "if you have the " + obj_name + "."

def MakeHeaderComment(obj_name, is_one_set):
    return MakeObjectComment('<'+obj_name+'> header file', is_one_set)

def AddComment(cfg_file, var):
    comment = None
    if var.get('comment'):
        comment = var['comment']
    elif var.get('ocomment'):
        comment = MakeObjectComment(var['ocomment'], var.get('val') == '1')
    elif var.get('ccomment'):
        comment = MakeHeaderComment(var['ccomment'], var.get('val') == '1')

    if comment:
        print >> cfg_file, "/* " + comment + " */"

def AddDefine(cfg_file, key, **var):
    print >> cfg_file
    AddComment(cfg_file, var)

    if var['is_on']:
        str = var.get('val', None)
        if str == None:
            print >> cfg_file, "#define %s" % key
        else:
            print >> cfg_file, "#define %s %s" % (key, var['val'])
    else:
        print >> cfg_file, "/* #undef %s */" % key

GenFunctionMap = {}

def MakeConfigFile(target, source, gen_function):
    '''MakeConfigFile is designed for creating config headers
    using 'gen_function' and traditional target/source pair.'''
    def_env = GetDefEnv()

    if 'CfgBuilder' not in def_env['BUILDERS'].keys():
        # first time, create cfg builder
        def GenerateConfigFile(target, source, env):
            global GenFunctionMap
            gen_func = GenFunctionMap[target[0].abspath]

            return gen_func(target, source, env)

        cfg_bld = def_env.Builder(action = GenerateConfigFile)
        def_env.Append(BUILDERS = {'CfgBuilder' : cfg_bld})

    global GenFunctionMap
    key = def_env.File(target).abspath
    assert not(key in GenFunctionMap.keys())
    GenFunctionMap[key] = gen_function

    def_env.CfgBuilder(target, source)

################################################################
# Misc

def MakeDictName(pkg_name):
    def MakeSaveName(name):
        # for example, GraphicsMagick++ -> GraphicsMagick_plus_plus
        safe_name = name.replace('+', '_plus').replace('-', '_minus').replace('.', '_dot')

        return safe_name
    return MakeSaveName(pkg_name) + '_dict'

def SetLibInfo(file, set_dict, pkg_name):
    dict = GetLIBFlags(pkg_name)
    dict_name = MakeDictName(pkg_name)
    # 1
    # for key in dict.keys():
    #     config.write( '%s_%s = %r\n' % (pkg_name, key, dict[key]) )
    file.write( '\n# %s library information\n' % (pkg_name) )
    file.write( '%s = %r\n' % (dict_name, dict) )
    # 2
    set_dict[dict_name] = dict

# add -I to all paths
def MakeIncludeOptions(dir_list, to_convert=0):
    dst = []
    if to_convert:
        def_env = GetDefEnv()
        for i in xrange(len(dir_list)):
            dir_list[i] = def_env.Dir(dir_list[i]).path
    for path in dir_list:
        dst.append('-I' + path)
    return dst

def MoveIncludeOptions(dict):
    if 'CPPPATH' in dict.keys():
        dict['CPP_POST_FLAGS'] = MakeIncludeOptions(dict['CPPPATH'])
        del dict['CPPPATH']

def GetDict(name):
    dict = UserOptDict[MakeDictName(name)]
    #reduce SCons' C scanner area, CPPPATH -> CPP_POST_FLAGS
    if IsToBuildQuick():
        MoveIncludeOptions(dict)
    return dict

#
# MakeMainEnv
# 

def GetSrcDirPath():
    return 'src' #'.'

# to be used as Dir in several places
def GetSrcBuildDir():
    if not 'obj' in dir(GetSrcBuildDir):
        GetSrcBuildDir.obj = GetDefEnv().Dir(BuildDir + '/' + GetSrcDirPath())
    return GetSrcBuildDir.obj

def CreateEnvVersion2(**kw):
    tools = ['default', 'AuxTools']
    if kw.has_key('tools'):
        tools += kw['tools']
    kw['tools'] = tools

    env = GetDefEnv().Environment(ENV = os.environ, toolpath = ['tools/scripts'], **kw)
    env.Prepend(CPPPATH = [GetSrcBuildDir(), '#/' + GetSrcDirPath()])
    return env

def IsToBuildQuick():
    can_bq = IsGccLike() # gcc and clang only (because of PCH)

    res = False
    if UserOptDict['BUILD_QUICK']:
        if can_bq:
            res = True
        elif not IsReenter(IsToBuildQuick):
            # warn once only
            print 'BUILD_QUICK is not supported for current compiler(%s)!' % Cc

    return res

def MakeMainEnv():
    if IsToBuildQuick():
        
        if IsGccCompiler():
            is_clang = False
        elif IsClangCompiler():
            is_clang = True
        else:
            assert False

        def SetPCH(env, header_name, additional_envs=[]):
            # :COMMENT: the "is_def" version is done to
            # check gcc' .gch using; once not used, dummy_pc_.h
            # header is used instead and error arised, resp.
            # (that is the purpose)
            # :WARNING: "is_def" may be not portable across SCons
            # versions and so should be tested, resp.
            is_def = 1
            if is_def:
                def make_pch(pch_env):
                    # we need to set args definitely to override SCons '# and BuildDir' dualism
                    return pch_env.Gch(target = header_name + env['GCHSUFFIX'], source = env.File(header_name).srcnode())[0]

                if is_clang:
                    pch_file = make_pch(env.Clone())
                    # :KLUDGE: CPPFLAGS is right way but it breaks if there is an C file
                    # in environment
                    env.Append(CXXFLAGS = '-include %s' % env.File(header_name).path)
                else:
                    pch_file = make_pch(env)
                    env.Depends(pch_file, env.InstallAs(target = './'+header_name, source = '#tools/scripts/dummy_pc_.h'))

                env['DepGch'] = 1
                for add_env in additional_envs:
                    add_env['DepGch'] = 1
            else:
                pch_file = env.Gch(header_name)[0]

            env['Gch'] = pch_file
            for add_env in additional_envs:
                add_env['Gch'] = pch_file

        env = CreateEnvVersion2(tools = ['gch'])
        suffix = '.pch' if is_clang else '.gch'
        env['GCHSUFFIX'] = suffix
    
        # 2 - reduce SCons' C scanner area, CPPPATH -> CPP_POST_FLAGS
        env['_CPPINCFLAGS'] = env['_CPPINCFLAGS'] + ' $CPP_POST_FLAGS'
    else:
        def SetPCH(env, header_name, additional_envs=[]):
            pass
        env = CreateEnvVersion2()

    UserOptDict['SetPCH'] = SetPCH
    # we use function Dir() so SCons do not change it
    # for local libs
    env.Append(LIBPATH = env.Dir(UserOptDict['LIB_BUILD_DIR']))
    CheckSettings(env)

    return env

# install src_dir into dst_dir (call 'Install' Builder)
def InstallDir(env, dst_dir, src_dir):
    tgt = None

    just_use_install = 1
    if not IsSConsGE_0_96_93():
        just_use_install = 0
    elif IsSConsVersionGE((0, 98, 0)) and not IsSConsVersionGE((1, 1, 0)):
        # all SCons versions 0.98-1.1.0 are broken with some dir hadndling, see
        # http://scons.tigris.org/issues/show_bug.cgi?id=1953
        just_use_install = 0

    if just_use_install:
        # can copy directories natively
        tgt = env.Install(dst_dir, src_dir)
    else:
        #Cautions:
        #   * src_dir must be relative path
        #   * it's slow - about 0.5 sec overhead for any (!) scons operation
        #     on src_dir = 'resources', on my comp
    
        src_list = []
        for path, dirs, files in os.walk(src_dir):
            src_list = src_list + [os.path.join(path, file) for file in files]
    
        dst_list = [os.path.join(dst_dir, file) for file in src_list]
    
        tgt = env.InstallAs(dst_list, src_list)
    return tgt

def make_source_files(fpath, lst):
    typ = type(lst)
    if typ != list :
        assert( typ == str )
        lst = GetDefEnv().Split(lst)

    return [ os.path.join(fpath, fname) for fname in lst ]

def InitUOD(user_options_dict):
    global UserOptDict
    UserOptDict = user_options_dict
    UserOptDict['make_source_files'] = make_source_files


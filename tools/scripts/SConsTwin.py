import SCons

def GetDefEnv():
    import SCons.Defaults
    return SCons.Defaults.DefaultEnvironment()

def IsSConsLE_0_96(env):
    """ Is SCons version <= 0.96 """
    if '_get_major_minor' in dir(env) and '__version__' in dir(SCons):
        v_major, v_minor = env._get_major_minor(SCons.__version__)
        if (v_major, v_minor) <= (0, 96):
            return 1
    return 0

def IsSConsVersionGE(triple):
    env = GetDefEnv()
    if '_get_major_minor_revision' in dir(env) and \
       '__version__' in dir(SCons):
        scons_ver = env._get_major_minor_revision(SCons.__version__)
        if scons_ver >= triple:
            return 1
    return 0

def MakeGenAction(function, strfunction=None):
    if IsSConsVersionGE((1, 2, 0)):
        # official way to create Action generator
        kw = {}
        if strfunction:
            kw['strfunction'] = strfunction
        act = SCons.Action.Action(function, generator=1, **kw)
    else:
        act = SCons.Action.CommandGeneratorAction(function)
    return act



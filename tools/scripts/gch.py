# -*- coding: utf-8 -*-
# $Id$
#
# SCons builder for gcc's precompiled headers
# Copyright (C) 2006, 2007  Tim Blechmann
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

# $Revision$
# $LastChangedRevision$
# $LastChangedDate$
# $LastChangedBy$

import SCons.Action
import SCons.Builder
import SCons.Scanner.C
import SCons.Util
import SCons.Script

import BuildVars
import SCons.Defaults

def gen_suffix(env, sources):
    return sources[0].get_suffix() + env['GCHSUFFIX']


GetCScannerFunc = None
GCHCOMString    = None
GCHSHCOMString  = None

#SCons.Script.EnsureSConsVersion(0,96,92)
def_env = SCons.Defaults.DefaultEnvironment()
if BuildVars.IsSConsGE_0_96_92(def_env):
    GchAction = SCons.Action.Action('$GCHCOM', '$GCHCOMSTR')
    GchShAction = SCons.Action.Action('$GCHSHCOM', '$GCHSHCOMSTR')
    
    # compose gch string from standard one, adding -x c++-header after '-o ' option
    import re
    pat = re.compile(re.escape("$TARGET "))
    def MakeGchString(cmd_str):
        res = re.subn(pat, "$TARGET -x c++-header ", cmd_str)
        assert res[1] == 1
        return res[0]

    GetCScannerFunc = lambda : SCons.Scanner.C.CScanner()
    GCHCOMString    = MakeGchString(def_env['CXXCOM'])   # '$CXX -o $TARGET -x c++-header -c $CXXFLAGS $_CCCOMCOM $SOURCE'
    GCHSHCOMString  = MakeGchString(def_env['SHCXXCOM']) # '$CXX -o $TARGET -x c++-header -c $SHCXXFLAGS $_CCCOMCOM $SOURCE'
else:
    # < 0.96.92
    GchAction = SCons.Action.Action('$GCHCOM')
    GchShAction = SCons.Action.Action('$GCHSHCOM')

    GetCScannerFunc = lambda : SCons.Scanner.C.CScan()
    _CCCOMCOM_str = '$CPPFLAGS $_CPPDEFFLAGS $_CPPINCFLAGS'
    GCHCOMString    = '$CXX $CXXFLAGS -x c++-header '+_CCCOMCOM_str+' -c -o $TARGET $SOURCE'
    GCHSHCOMString  = '$SHCXX $SHCXXFLAGS -x c++-header '+_CCCOMCOM_str+' -c -o $TARGET $SOURCE'

GchShBuilder = SCons.Builder.Builder(action = GchShAction,
                                     source_scanner = GetCScannerFunc(),
                                     suffix = gen_suffix)

GchBuilder = SCons.Builder.Builder(action = GchAction,
                                   source_scanner = GetCScannerFunc(),
                                   suffix = gen_suffix)


def SetPCHDependencies(target, source, env, gch_key):
    if env.has_key(gch_key) and env[gch_key]:
        gch_node = env[gch_key]
        # Murav'jov - alternative variant
        if env.has_key('DepGch') and env['DepGch']:
            env.Depends(target, gch_node)
        else:
            scanner = GetCScannerFunc()
            path = scanner.path(env)
            deps = scanner(source[0], env, path)

            if str(gch_node.sources[0]) in [str(x) for x in deps]:
                env.Depends(target, gch_node)

def static_pch_emitter(target,source,env):
    SCons.Defaults.StaticObjectEmitter( target, source, env )

    SetPCHDependencies(target, source, env, 'Gch')

    return (target, source)

def shared_pch_emitter(target,source,env):
    SCons.Defaults.SharedObjectEmitter( target, source, env )

    SetPCHDependencies(target, source, env, 'GchSh')

    return (target, source)

def generate(env):
    """
    Add builders and construction variables for gcc PCH to an Environment.
    """
    # Murav'jov - no using, temporarily?
    #env.Append(BUILDERS = {
    #    'gch': env.Builder(
    #    action = GchAction,
    #    target_factory = env.fs.File,
    #    ),
    #    'gchsh': env.Builder(
    #    action = GchShAction,
    #    target_factory = env.fs.File,
    #    ),
    #    })

    try:
        bld = env['BUILDERS']['Gch']
        bldsh = env['BUILDERS']['GchSh']
    except KeyError:
        bld = GchBuilder
        bldsh = GchShBuilder
        env['BUILDERS']['Gch'] = bld
        env['BUILDERS']['GchSh'] = bldsh

    env['GCHCOM']     = GCHCOMString
    env['GCHSHCOM']   = GCHSHCOMString
    env['GCHSUFFIX']  = '.gch'

    for suffix in SCons.Util.Split('.c .C .cc .cxx .cpp .c++'):
        env['BUILDERS']['StaticObject'].add_emitter( suffix, static_pch_emitter )
        env['BUILDERS']['SharedObject'].add_emitter( suffix, shared_pch_emitter )


def exists(env):
    return env.Detect('g++')

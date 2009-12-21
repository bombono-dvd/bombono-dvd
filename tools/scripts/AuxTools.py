import os.path
import SCons.Tool

# we dont like to write like
#   env.Depends('.', env.Program(...))
# , so we use this emitter
class BindEmitter:
    def __init__(self, parent_emitter):
        self.parent_emitter = parent_emitter

    def __call__(self, target, source, env):
        self.parent_emitter(target, source, env)

        dir_node = env.Dir('.')
        #print target[0].path
        #print dir_node.path
        if os.path.split(target[0].path)[0] != dir_node.path:
            env.Depends(dir_node, target)

        return (target, source)

def generate(env):
    SCons.Tool.createProgBuilder(env)
    SCons.Tool.createStaticLibBuilder(env)
    SCons.Tool.createSharedLibBuilder(env)

    def SetBindEmitter(builder_name, env):
        builder = env['BUILDERS'][builder_name]
        builder.emitter = BindEmitter(builder.emitter)

    for name in ['Program', 'Library', 'SharedLibrary']:
        SetBindEmitter(name, env)

def exists(env):
    return 1



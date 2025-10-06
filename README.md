# shader_cache
SUBPROJECT

A program which manages a collection of fixed shaders, and allows programmers to interact with them safely

## TODO
* I would prefer if a custom c++ object was created for each shader program, then instead of allowing for potentially invalid shader cache calls such as trying to set an attribute on a shader that doesn't exist then we wouldn't allow that
* additionally a shader is not fully configured until all of its uniforms have been set at least once, I want to check for this before drawing as well.

## Build setup

### Setting up clangd

If you're using clangd for linting and editor support, you'll need to
communicate the structure of the project to it for it to behave well. The
easiest way to do this is using a tool like compiledb:

```sh
compiledb make
```

This should generate a compile_commands.json that clangd will use to properly
link the various source folders.
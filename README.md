# HLVM-Engine Doc

## Setup

In the [HLVM-Engine](https://github.com/yhyu13/HLVM-Engine.git) `./Document` directory

```
git submodule update --init --recursive
./Setup.sh
```

## Build
```
cd ./Doxygen && ./Build.sh && cd ..
cd ./Sphinx && ./Build.sh && cd ..
```

The sphinx html files are in `Document/Engine/Sphinx/Build/build/html/index.html`

## Clean
```
cd ./Doxygen && ./Clean.sh && cd ..
cd ./Sphinx && ./Clean.sh && cd ..
```

## Reference
- [HLVM-Engine](https://github.com/yhyu13/HLVM-Engine.git)
- [Documenting c++ with doxygen, sphinx, exhale](https://rgoswami.me/posts/doc-cpp-dox-sph-exhale/)
- [Exhale - Read The Doc](https://exhale.readthedocs.io/en/latest/quickstart.html#getting-started-with-sphinx)
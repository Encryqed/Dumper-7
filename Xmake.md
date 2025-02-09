# Xmake Build Guide
To install Xmake visit: https://xmake.io/#/getting_started?id=installation

And for extra info on Xmake's commands visit: https://xmake.io/#/getting_started

<br>

I also recommend this extention for VSCode: https://marketplace.visualstudio.com/items?itemName=tboox.xmake-vscode

## To configure the project
```bash
xmake f -p windows -a <arch> -m <build_mode>

# Examples
xmake f -p windows -a x64 -m release # As Release 
xmake f -p windows -a x64 -m debug # as Debug
```

## Fresh build
```bash
xmake clean
xmake build
```

## To generate a visual studio project to build with Xmake you can use:
```bash
xmake project -k vs -m "debug;release"
# Or
xmake project -k vsxmake2022 -m "debug;release"
```


<br><sub>Author: [@Omega172</sub>](https://github.com/Omega172) 

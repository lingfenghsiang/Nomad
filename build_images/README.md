# Build docker images for compiling

Building the image mentioned in `PROJECT_DIR/compile.sh`
```
docker build -t docklf/compile_environment:glogflagtest -f build_images/userspace.dockerbuild .
```
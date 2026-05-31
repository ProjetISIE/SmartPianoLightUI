xhost +local:podman
podman run \
  -it \
  --device /dev/dri:/dev/dri \
  -e DISPLAY=$DISPLAY \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  -v /tmp/smartpiano.sock:/tmp/smartpiano.sock \
  smart-piano-ui

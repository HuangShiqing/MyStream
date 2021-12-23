export LD_LIBRARY_PATH=/home/pi/DeepLearning/clone/MNN/build/:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/pi/DeepLearning/code/MyNet:$LD_LIBRARY_PATH
export GST_PLUGIN_PATH=/mnt/MyStream/build/gst-motor/so:/mnt/MyStream/build/gst-infer/so
# gst-launch-1.0 v4l2src device=/dev/video0 ! capsfilter caps=video/x-raw,format=RGB,width=320,height=240,framerate=25/1 ! autovideosink
gst-launch-1.0 v4l2src device=/dev/video0 ! capsfilter caps=video/x-raw,format=RGB,width=320,height=240,framerate=25/1 ! infer silent=false ! motor silent=false

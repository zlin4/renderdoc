A modified RenderDoc version to dump per-draw images of a frame. Please refer to RenderDoc original website for general usage: https://renderdoc.org/.

First, build the project with Visual Studio. Then run it and capture a frame log file (refer to its original documents for more information). To dump per-draw images:

Open a renderdoc log file.
Click event 1 (EID 1) in the event browser.
You can config how you want to see the images, such as the background color, in the texture viewer. All images will use the same config.
Click the export button in event browser. Then renderdoc should create a folder named RenderDocPerDrawImages in your desktop and dump the png files in it.

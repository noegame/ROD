# Rod C Tests

```
test_aruco_detection.c          Pipeline CV (sharpen→resize→detect)
test_camera_interface.c         Contract API (real vs emulated)
test_camera_parameters.c        Hardware tuning (exposure/gain)
test_emulated_camera_impl.c     Emulated camera behavior
test_detection_dataset.c        Detection accuracy on known images
```

## How to run the tests

Following commands should be executed in the folder `rod_c`:

```bash
./build/tests/test_camera_interface /var/roboteseo/pictures/camera_tests/optimized/
./build/tests/test_emulated_camera_impl /var/roboteseo/pictures/camera_tests/optimized/
./build/tests/test_detection_dataset
./build/tests/test_camera_parameters [width] [height] [output_dir]
./build/tests/test_aruco_detection <input.jpg> <output.jpg>
```
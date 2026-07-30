#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <camera/camera_api.h>

/* NOTE:
 *   This is just for testing a much smaller frame capture process. This will eventually have a signal handler,
 *   intended to listen for signals from the model_operations process for frames.
 *
 *   Currently, each call to these functions would get it to initialize and then disconnect from the camera, which is
 *   a very inefficient way of doing this.
 **/
static void dump_frame(camera_handle_t handle, camera_buffer_t* buffer, void* arg)
{
    dumpFrameContext_t* context;
    int                 err;
    char                fileName[MAX_STRING_LEGNTH];
    size_t              bufferSize;


}

/* Cleanup camera and screen
 *
 *
 **/
static int cleanup_camera(camera_handle_t handle, dumpFrameContext_t* dumpFrameContext, uint32_t flags){

}

int main()
{
    camera_handle_t cam_handle = INVALID_CAMERA_HANDLE; // handle to the camera
    camera_unit_t   cameraUnit = CAMERA_UNIT_1;         //
    dumpFrameContext_t dumpFrameContext = {0};

    if(camera_connect(0, &cam_handle) != CAMERA_EOK){
        fprintf(stderr, "[ERROR] Failed to connect to camera sensor.\n");
        return EXIT_FAILURE;
    }

    if(camera_register_callback(cam_handle, CAMERA_STAGE_STILL, frame_callback, NULL) != CAMERA_EOK){
        fprintf(stderr, "[ERROR] Failed to register datapath callback.\n");
        return EXIT_FAILURE;
    }

    camera_set_resolution(cam_handle, CAMERA_STAGE_STILL, &capture_res);
    camera_start_viewfinder(cam_handle);

    printf("!!!Triggering frame capture...\n");
    camera_take_photo(cam_handle);
    (void) usleep(500);
    camera_stop_viewfinder(cam_handle);
    camera_disconnect(cam_handle);

}

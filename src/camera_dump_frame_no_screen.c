/*
 * Copyright (c) 2025, BlackBerry Limited. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file camera_dump_frame_no_screen.c
 *
 * @brief Dump frames received from the sensor service
 */

#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <camera/camera_api.h>

// Size of Screen group ID string
#define GROUP_ID_STRING_LEN                 64

// Our maximum size for our window ID string
#define WINDOW_ID_MAX_SIZE                  64

// Maximum filepath length
#define MAX_STRING_LEGNTH                   80

// Default dump file path
static const char* DEFAULT_FILE_PATH =      "/data/share/captures";

// Flag set by the signal handler to request shutdown
static volatile sig_atomic_t exitExample = 0;

// Flags for cleanup on exit
typedef enum {
    // Nothing to cleanup
    CLEANUP_NONE = 0,
    // Destroy mutex
    CLEANUP_MUTEX = 1 << 0,
    // Stop viewfinder
    CLEANUP_VIEWFINDER = 1 << 1,
    // Close camera handle
    CLEANUP_CAMERA_HANDLE = 1 << 2,
} cleanupFlags_t;

// Context needed to dump a frame
typedef struct {
    char                filePath[MAX_STRING_LEGNTH];
    camera_frametype_t  frametype;
    pthread_mutex_t     mutex;
    bool                frameDumped;
} dumpFrameContext_t;

/**
 * @brief Signal handler which requests a graceful shutdown
 *
 * @param[in] signum Signal number received
 */
static void handleSignal(int signum)
{
    (void) signum;
    exitExample = 1;
}

/**
 * @brief Clean up camera and screen depending on flags set
 *
 * @param[in] handle Handle to the camera
 * @param[in] dumpFrameContext Pointer to dump frame context
 * @param[in] flags Flags which indicate what to clean up
 *
 * @return Return EOK on success, otherwise an error
 */
static int stopApp(camera_handle_t handle,
                   dumpFrameContext_t* dumpFrameContext,
                   uint32_t flags)
{
    int err;
    int rc = EOK;

    // Stop everything we need on exit
    if ((flags & (uint32_t) CLEANUP_VIEWFINDER) != 0u) {
        err = camera_stop_viewfinder(handle);
        if (err != EOK) {
            (void) fprintf(stderr, "Failed to stop viewfinder: err=%d\n", err);
            rc = err;
        }
    }

    // Clean up camera handle which was created by camera_open
    if ((flags & (uint32_t) CLEANUP_CAMERA_HANDLE) != 0u) {
        err = camera_close(handle);
        if (err != EOK) {
            (void) fprintf(stderr, "Failed to close handle: err=%d\n", err);
            rc = err;
        }
    }

    // Destroy mutex
    if ((flags & (uint32_t) CLEANUP_MUTEX) != 0u) {
        err = pthread_mutex_destroy(&dumpFrameContext->mutex);
        if (err != EOK) {
            (void) fprintf(stderr, "Failed to destroy mutex: err=%d\n", err);
            rc = err;
        }
    }


    return rc;
}

/**
 * @brief Helper function for writing a buffer in memory to disk
 *
 * @param[in] fileName Path including the file name
 * @param[in] ptr Buffer in memory to write
 * @param[in] size Size of buffer in bytes
 *
 * @return Return EOK on success, otherwise an error
 */
static int writeToDisk(const char* fileName, void* ptr, size_t size)
{
    FILE*   fptr;
    int     err;
    size_t  written;

    fptr = fopen(fileName, "w+b");
    if (fptr == NULL) {
        (void) fprintf(stderr, "Failed to open %s: err=%d\n", fileName, errno);
        return errno;
    }

    written = fwrite(ptr, size, 1, fptr);
    if (written < 1) {
        (void) fprintf(stderr, "Failed to write to disk %s: err=%d\n", fileName, errno);
        (void) fclose(fptr);
        return errno;
    }

    err = fclose(fptr);
    if (err == EOF) {
        (void) fprintf(stderr, "Failed to close file pointer: err=%d\n", errno);
        return errno;
    }

    (void) printf("Frame written to %s\n", fileName);

    return err;
}

/**
 * @brief Helper function for finding size of @c buffer in bytes
 *
 * @details Modify @c getBufferSize to support more types; it currently supports
 * only popular types
 *
 * @param[in] buffer Camera buffer
 * @param[in] frametype Camera frametype
 * @param[out] size Size of camera buffer in bytes
 *
 * @return Return EOK on success, otherwise an error
 */
int getBufferSize(camera_buffer_t buffer, camera_frametype_t frametype, size_t* size)
{
    int err = EOK;

    // Sanity check
    if (size == NULL) {
        (void) fprintf(stderr, "NULL parameter");
        return EINVAL;
    }

    switch((int)frametype) {
        case (int) CAMERA_FRAMETYPE_BGR8888:
            *size = buffer.framedesc.bgr8888.stride * buffer.framedesc.bgr8888.height;
            break;
        case (int) CAMERA_FRAMETYPE_RGB8888:
            *size = buffer.framedesc.rgb8888.stride * buffer.framedesc.rgb8888.height;
            break;
        case (int) CAMERA_FRAMETYPE_NV12:
            *size = buffer.framedesc.nv12.stride * buffer.framedesc.nv12.height;
            break;
        case (int) CAMERA_FRAMETYPE_YCBYCR:
            *size = buffer.framedesc.bgr8888.stride * buffer.framedesc.bgr8888.height;
            break;
        case (int) CAMERA_FRAMETYPE_CBYCRY:
            *size = buffer.framedesc.cbycry.stride * buffer.framedesc.cbycry.height;
            break;
        case (int) CAMERA_FRAMETYPE_RGB565:
            *size = buffer.framedesc.rgb565.stride * buffer.framedesc.rgb565.height;
            break;
        default:
            (void) fprintf(stderr, "Frametype %d is not supported\n", frametype);
            err = EINVAL;
            break;
    }

    return err;
}

/**
 * @brief Callback which is called every time a new @c buffer is available
 *
 * @details Dumps a single frame to disk as a raw file, then requests exit
 *
 * @param[in] handle Handle to camera
 * @param[in] buffer Camera buffer received
 * @param[in] arg Pointer to @c dumpFrameContext_t
 */
static void dumpFrame(camera_handle_t handle, camera_buffer_t* buffer, void* arg)
{
    dumpFrameContext_t* context;
    int                 err;
    char                fileName[MAX_STRING_LEGNTH];
    size_t              bufferSize;

    // Sanity check
    if ((buffer == NULL) ||
        (arg == NULL)) {
        (void) fprintf(stderr, "NULL argument in callback\n");
    return;
        }

        context = (dumpFrameContext_t*) arg;

        err = pthread_mutex_lock(&context->mutex);
        if (err != EOK) {
            (void) fprintf(stderr, "Failed to lock mutex: err=%d\n", err);
            return;
        }

        // Only dump the first frame we receive; ignore any subsequent frames
        if (context->frameDumped) {
            (void) pthread_mutex_unlock(&context->mutex);
            return;
        }

        err = snprintf(fileName,
                       sizeof(fileName),
                       "%s/frame.raw",
                       context->filePath);
        if (err < 0) {
            (void) fprintf(stderr, "Failed to create a file name: err=%d\n", errno);
            (void) pthread_mutex_unlock(&context->mutex);
            return;
        }
        err = getBufferSize(*buffer, context->frametype, &bufferSize);
        if (err != EOK) {
            (void) fprintf(stderr, "Failed to get buffer size: err=%d\n", err);
            (void) pthread_mutex_unlock(&context->mutex);
            return;
        }
        err = writeToDisk(fileName, buffer->framebuf, bufferSize);
        if (err != EOK) {
            (void) fprintf(stderr, "Failed to dump a raw frame: err=%d\n", err);
        } else {
            context->frameDumped = true;
            // We have our frame; request the main loop to exit
            exitExample = 1;
        }

        err = pthread_mutex_unlock(&context->mutex);
        if (err != EOK) {
            (void) fprintf(stderr, "Failed to unlock mutex: err=%d\n", err);
        }
}

/**
 * @brief Helper function for starting camera-dump-frame
 *
 * @param[out] dumpFrameContext Context for dumping frames
 * @param[in] cameraUnit Camera unit number
 * @param[out] cameraHandle Handle to camera
 * @param[out] flags Flags indicating what to clean up
 *
 * @return Return EOK on success, otherwise an error
 */
static int startApp(dumpFrameContext_t* dumpFrameContext,
                    camera_unit_t cameraUnit,
                    camera_handle_t* cameraHandle,
                    uint32_t* flags)
{
    int             err;
    camera_error_t  cameraErr;

    // Sanity check
    if ((dumpFrameContext == NULL) ||
        (cameraHandle == NULL) ||
        (flags == NULL)) {
        (void) fprintf(stderr, "NULL parameters\n");
        }

        *flags = (uint32_t) CLEANUP_NONE;

    // Initialize mutex
    err = pthread_mutex_init(&dumpFrameContext->mutex, NULL);
    if (err != EOK) {
        (void) fprintf(stderr, "Failed to init mutex: err=%d\n", err);
        (void) stopApp(*cameraHandle, dumpFrameContext, *flags);
        return err;
    }
    *flags |= (uint32_t) CLEANUP_MUTEX;

    cameraErr = camera_open(cameraUnit, CAMERA_MODE_RO, cameraHandle);
    if (cameraErr != CAMERA_EOK) {
        (void) fprintf(stderr, "Failed to open the camera: err=%d\n", err);
        (void) stopApp(*cameraHandle, dumpFrameContext, *flags);
        return (int) cameraErr;
    }
    *flags |= (uint32_t) CLEANUP_CAMERA_HANDLE;

    // Get camera frametype
    cameraErr = camera_get_vf_property(*cameraHandle, CAMERA_IMGPROP_FORMAT, &dumpFrameContext->frametype);
    if (cameraErr != EOK) {
        (void) fprintf(stderr, "Failed to get viewfinder frametype: err=%d\n", cameraErr);
        (void) stopApp(*cameraHandle, dumpFrameContext, *flags);
        return (int) cameraErr;
    }

    // Start camera viewfinder
    cameraErr = camera_start_viewfinder(*cameraHandle, dumpFrame, NULL, (void*)dumpFrameContext);
    if (cameraErr != EOK) {
        (void) fprintf(stderr, "Failed to start viewfinder for camera: err=%d\n", cameraErr);
        (void) stopApp(*cameraHandle, dumpFrameContext, *flags);
        return (int) cameraErr;
    }
    *flags |= (uint32_t) CLEANUP_VIEWFINDER;

    return EOK;
}

int main(int argc, char *argv[])
{
    int                 err = EOK;
    int                 opt;
    uint32_t            flags = (uint32_t) CLEANUP_NONE;
    long                unitLong;
    bool                userPathGiven = false;
    camera_handle_t     cameraHandle = CAMERA_HANDLE_INVALID;
    camera_unit_t       cameraUnit = CAMERA_UNIT_1;
    dumpFrameContext_t  dumpFrameContext = {0};

    while ((opt = getopt(argc, argv, "f:u:")) != -1) {
        switch (opt) {
            case (int) 'f':
                userPathGiven = true;
                err = snprintf(dumpFrameContext.filePath, sizeof(dumpFrameContext.filePath), "%s", optarg);
                if (err < 0) {
                    (void) fprintf(stderr, "Failed to get user file path: err=%d\n", errno);
                } else {
                    err = EOK;
                }
                break;
            case (int) 'u':
                errno = EOK;
                unitLong = strtol(optarg, NULL, 10);
                if (errno != EOK) {
                    (void) fprintf(stderr, "Failed to parse unit argument: err %d\n", errno);
                    err = EINVAL;
                } else {
                    cameraUnit = (camera_unit_t) unitLong;
                }
                break;
            default:
                (void) fprintf(stderr, "Ignoring unrecognized option: %c\n", opt);
                break;
        }
    }

    if (err != EOK) {
        return -1;
    }
    if (!userPathGiven) {
        err = snprintf(dumpFrameContext.filePath,
                       sizeof(dumpFrameContext.filePath),
                       "%s",
                       DEFAULT_FILE_PATH);
        if (err < 0) {
            (void) fprintf(stderr, "Failed to copy default path: err=%d\n", errno);
            return -1;
        }
    }

    // Install signal handlers so we can shut down gracefully
    (void) signal(SIGINT, handleSignal);
    (void) signal(SIGTERM, handleSignal);

    err = startApp(&dumpFrameContext,
                   cameraUnit,
                   &cameraHandle,
                   &flags);
    if (err != EOK) {
        (void) fprintf(stderr, "Failed to start camera-dump-frame: err=%d\n", err);
        return -1;
    }

    (void) printf("Capturing a single frame to %s\n", dumpFrameContext.filePath);

    // The viewfinder callback dumps one frame and sets exitExample;
    // wait here until that happens (or until interrupted)
    int waited_ms = 0;
    while (!exitExample && waited_ms < 5000) {
        (void) usleep(10 * 1000);
        waited_ms += 10;
    }
    if (!exitExample) {
        (void) fprintf(stderr, "Timed out waiting for a frame\n");
        (void) stopApp(cameraHandle, &dumpFrameContext, flags);
        return -1;
    }

    (void) printf("Exiting camera-dump-frame\n");

    // Cleanup on exit
    err = stopApp(cameraHandle, &dumpFrameContext, flags);
    if (err != EOK) {
        (void) fprintf(stderr, "Failed to clean up camera-dump-frame-no-screen: err=%d\n", err);
        return -1;
    }

    return 0;
}

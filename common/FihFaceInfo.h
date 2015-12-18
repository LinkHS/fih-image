#ifndef __FIH_FACE_INFO_H
#define __FIH_FACE_INFO_H


/******************************************************************************
 *  The information of a face from camera face detection.
 ******************************************************************************/
 struct CameraFace {
    /**
     * Bounds of the face [left, top, right, bottom]. (-1000, -1000) represents
     * the top-left of the camera field of view, and (1000, 1000) represents the
     * bottom-right of the field of view. The width and height cannot be 0 or
     * negative. This is supported by both hardware and software face detection.
     *
     * The direction is relative to the sensor orientation, that is, what the
     * sensor sees. The direction is not affected by the rotation or mirroring
     * of CAMERA_CMD_SET_DISPLAY_ORIENTATION.
     */
    int32 rect[4];

    /**
     * The confidence level of the face. The range is 1 to 100. 100 is the
     * highest confidence. This is supported by both hardware and software
     * face detection.
     */
    int32 score;

    /**
     * An unique id per face while the face is visible to the tracker. If
     * the face leaves the field-of-view and comes back, it will get a new
     * id. If the value is 0, id is not supported.
     */
    int32 id;

    /**
     * The coordinates of the center of the left eye. The range is -1000 to
     * 1000. -2000, -2000 if this is not supported.
     */
    int32 left_eye[2];

    /**
     * The coordinates of the center of the right eye. The range is -1000 to
     * 1000. -2000, -2000 if this is not supported.
     */
    int32 right_eye[2];

    /**
     * The coordinates of the center of the mouth. The range is -1000 to 1000.
     * -2000, -2000 if this is not supported.
     */
    int32 mouth[2];
};


/******************************************************************************
 *   FD Pose Information: ROP & RIP
 *****************************************************************************/
struct FaceInfo {

    int32 rop_dir;
    int32 rip_dir;
};


/******************************************************************************
 *  The metadata of the frame data.
 *****************************************************************************/
struct CameraFaceMetadata {
    /**
     * The number of detected faces in the frame.
     */
    int32 number_of_faces;

    /**
     * An array of the detected faces. The length is number_of_faces.
     */
    CameraFace *faces;
    FaceInfo   *posInfo;
};

#endif

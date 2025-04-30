import numpy
import cv2
import ctypes
import os
import loguru


def main():
    # The path to the python root
    path = os.path.abspath(os.path.dirname(__file__))

    # Path to the shared library
    path_to_lib = os.path.join(path, "..", "build", "liblib.dylib")

    # Load the shared library
    library = ctypes.CDLL(path_to_lib)

    # Pointer to the buffer
    buffer = ctypes.POINTER(ctypes.c_uint8)()
    buffer_pointer = ctypes.pointer(buffer)

    # Pointer to the width
    width = ctypes.c_uint32(0)
    width_pointer = ctypes.pointer(width)

    # Pointer to the height
    height = ctypes.c_uint32(0)
    height_pointer = ctypes.pointer(height)

    # Set verbose output
    library.setVerbose(True)

    # Initialize
    status = library.initialize(buffer_pointer, width_pointer, height_pointer)
    if status != 0:
        loguru.logger.critical("Failed to initialize camera stream")
        return

    # Get values
    width = width_pointer.contents.value
    height = height_pointer.contents.value
    loguru.logger.debug("Size ({}, {})", width, height)

    # Start the stream
    status = library.start(14)
    if status != 0:
        loguru.logger.critical("Failed to start camera stream")
        return

    # Set the fade
    status = library.setFadeTime(1_000)
    if status != 0:
        loguru.logger.critical("Failed to set the fade time")
        return

    # Set the timeout for the events
    status = library.setTimeout(10_000)
    if status != 0:
        loguru.logger.critical("Failed to set the itmeout")
        return

    # Load into numpy array
    buffer = numpy.ctypeslib.as_array(buffer_pointer.contents, (height, width, 3))

    while True:
        # Display
        cv2.imshow("Window", buffer)
        key = cv2.waitKey(1)
        if key == ord("q"):
            break

    # Stop the stream
    library.stop()


if __name__ == "__main__":
    main()

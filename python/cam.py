import numpy
import cv2
import ctypes
import os
import loguru


# The path to the python root
__path = os.path.abspath(os.path.dirname(__file__))

# Path to the shared library
__path_to_lib = os.path.join(__path, "..", "build", "liblib.dylib")

# Load the shared library
__library = ctypes.CDLL(__path_to_lib)


def init() -> numpy.ndarray:
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
    __library.setVerbose(True)

    # Initialize
    status = __library.initialize(buffer_pointer, width_pointer, height_pointer)
    if status != 0:
        loguru.logger.critical("Failed to initialize camera stream")
        raise RuntimeError

    # Get values
    width = width_pointer.contents.value
    height = height_pointer.contents.value
    loguru.logger.debug("Size ({}, {})", width, height)

    # Load into numpy array
    buffer = numpy.ctypeslib.as_array(buffer_pointer.contents, (height, width, 3))

    return buffer


def start() -> None:
    # Start the stream
    status = __library.start(14)
    if status != 0:
        loguru.logger.critical("Failed to start camera stream")
        raise RuntimeError

    # Set the fade
    status = __library.setFadeTime(1_000)
    if status != 0:
        loguru.logger.critical("Failed to set the fade time")
        raise RuntimeError

    # Set the timeout for the events
    status = __library.setTimeout(10_000)
    if status != 0:
        loguru.logger.critical("Failed to set the timeout")
        raise RuntimeError


def stop() -> None:
    # Stops the program
    status = __library.stop()
    if status != 0:
        loguru.logger.critical("Failed to setop the camera stream")
        raise RuntimeError

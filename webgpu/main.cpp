#include "core.hpp"

#include <thread>
#include <semaphore>
#include <iostream>
#include <filesystem>

#if !_WIN32 
    #error "No Linux platform"
#else

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#include "commdlg.h"
#undef near
#undef far


void* win32_alloc(size_t size){
    return VirtualAlloc(0, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}
template <typename T>
T* win32_alloc(){
    auto* t = (T*)win32_alloc(sizeof(T));
    new (t) T();
    return t;
}

#endif

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

GLFWwindow* 
init_glfw(app_memory_t* app_mem) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    // glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	
    auto* window = glfwCreateWindow(800, 600, "Webgpu App", 0, 0);

    app_mem->config.window_handle = glfwGetWin32Window(window);

    assert(window);

    return window;
}

LONG exception_filter(_EXCEPTION_POINTERS* exception_info) {
    auto [
        code,
        flags,
        record,
        address,
        param_count,
        info
    ] = *exception_info->ExceptionRecord;

    if (code == EXCEPTION_BREAKPOINT) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    std::string msg;
    switch(code) {
        case EXCEPTION_ACCESS_VIOLATION: msg = "Access Violation.\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: msg = "The thread tried to access an array element that is out of bounds and the underlying hardware supports bounds checking."; break;
        case EXCEPTION_BREAKPOINT: msg = "A breakpoint was encountered."; break;
        case EXCEPTION_DATATYPE_MISALIGNMENT: msg = "The thread tried to read or write data that is misaligned on hardware that does not provide alignment. For example, 16-bit values must be aligned on 2-byte boundaries; 32-bit values on 4-byte boundaries, and so on."; break;
        case EXCEPTION_FLT_DENORMAL_OPERAND: msg = "One of the operands in a floating-point operation is denormal. A denormal value is one that is too small to represent as a standard floating-point value."; break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: msg = "The thread tried to divide a floating-point value by a floating-point divisor of zero."; break;
        case EXCEPTION_FLT_INEXACT_RESULT: msg = "The result of a floating-point operation cannot be represented exactly as a decimal fraction."; break;
        case EXCEPTION_FLT_INVALID_OPERATION: msg = "This exception represents any floating-point exception not included in this list."; break;
        case EXCEPTION_FLT_OVERFLOW: msg = "The exponent of a floating-point operation is greater than the magnitude allowed by the corresponding type."; break;
        case EXCEPTION_FLT_STACK_CHECK: msg = "The stack overflowed or underflowed as the result of a floating-point operation."; break;
        case EXCEPTION_FLT_UNDERFLOW: msg = "The exponent of a floating-point operation is less than the magnitude allowed by the corresponding type."; break;
        case EXCEPTION_ILLEGAL_INSTRUCTION: msg = "The thread tried to execute an invalid instruction."; break;
        case EXCEPTION_IN_PAGE_ERROR: msg = "The thread tried to access a page that was not present, and the system was unable to load the page. For example, this exception might occur if a network connection is lost while running a program over the network."; break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO: msg = "The thread tried to divide an integer value by an integer divisor of zero."; break;
        case EXCEPTION_INT_OVERFLOW: msg = "The result of an integer operation caused a carry out of the most significant bit of the result."; break;
        case EXCEPTION_INVALID_DISPOSITION: msg = "An exception handler returned an invalid disposition to the exception dispatcher. Programmers using a high-level language such as C should never encounter this exception."; break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION: msg = "The thread tried to continue execution after a noncontinuable exception occurred."; break;
        case EXCEPTION_PRIV_INSTRUCTION: msg = "The thread tried to execute an instruction whose operation is not allowed in the current machine mode."; break;
        case EXCEPTION_SINGLE_STEP: msg = "A trace trap or other single-instruction mechanism signaled that one instruction has been executed."; break;
        case EXCEPTION_STACK_OVERFLOW: msg = "The thread used up its stack."; break;
    }

    if (code == EXCEPTION_ACCESS_VIOLATION) {
        switch(info[0]) {
            case 0: msg = "Access Violation (read).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 1: msg = "Access Violation (write).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 8: msg = "Access Violation (dep).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
        }

        msg += fmt::format("\nAttempted to access virtual address {}", (void*)info[1]);
    }
    if (code == EXCEPTION_IN_PAGE_ERROR) {
        switch(info[0]) {
            case 0: msg = "Page Violation (read).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 1: msg = "Page Violation (write).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
            case 8: msg = "Page Violation (dep).\nThe thread tried to read from or write to a virtual address for which it does not have the appropriate access."; break;
        }

        msg += fmt::format("\nAttempted to access virtual address {}", (void*)info[1]);
    }

    MessageBox(0, fmt::format("Exception at address {}\nCode: {}\nFlag: {}", address, msg!=""?msg:fmt::format("{}", code), flags).c_str(), 0, MB_ABORTRETRYIGNORE);

    gen_error(__FUNCTION__, "Exception at {} Code: {} Flag: {}", address, msg!=""?msg:fmt::format("{}", code), flags);

    return EXCEPTION_CONTINUE_SEARCH;
}


i32 message_box_proc(const char* text) {
    return MessageBox(0, text, 0, MB_OKCANCEL);
}

bool open_file_dialog(char* filename, size_t max_file_size) {
    OPENFILENAME open_file_name{};
    open_file_name.lStructSize = sizeof(OPENFILENAME);
    open_file_name.hwndOwner = 0;
    return false;
}

// #define MULTITHREAD_ENGINE

#include <webgpu/webgpu.h>
#include <webgpu/glfw3webgpu.h>
#include <webgpu/glfw3webgpu.c>


WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
    // A simple structure holding the local information shared with the
    // onAdapterRequestEnded callback.
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    // Callback called by wgpuInstanceRequestAdapter when the request returns
    // This is a C++ lambda function, but could be any function defined in the
    // global scope. It must be non-capturing (the brackets [] are empty) so
    // that it behaves like a regular C function pointer, which is what
    // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
    // is to convey what we want to capture through the pUserData pointer,
    // provided as the last argument of wgpuInstanceRequestAdapter and received
    // by the callback as its last argument.
    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        } else {
            std::cout << "Could not get WebGPU adapter: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        onAdapterRequestEnded,
        (void*)&userData
    );

    // In theory we should wait until onAdapterReady has been called, which
    // could take some time (what the 'await' keyword does in the JavaScript
    // code). In practice, we know that when the wgpuInstanceRequestAdapter()
    // function returns its callback has been called.
    assert(userData.requestEnded);

    return userData.adapter;
}

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        } else {
            std::cout << "Could not get WebGPU device: " << message << std::endl;
        }
        userData.requestEnded = true;
    };

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        onDeviceRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.device;
}

int 
main(int argc, char* argv[]) {
    _set_error_mode(_OUT_TO_MSGBOX);
    SetUnhandledExceptionFilter(exception_filter);
    
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = nullptr;

    WGPUInstance instance = wgpuCreateInstance(&desc);

    if (!instance) {
        std::cerr << "Could not initialize WebGPU!" << std::endl;
        return 1;
    }

    // 4. Display the object (WGPUInstance is a simple pointer, it may be
    // copied around without worrying about its size).
    std::cout << "WGPU instance: " << instance << std::endl;

    if(!glfwInit()){
        assert(0);
        return -1;
    }
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // NEW
    GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
    assert(window);
    WGPUSurface surface = glfwGetWGPUSurface(instance, window);
    std::cout << "Requesting adapter..." << std::endl;

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = nullptr;
    adapterOpts.compatibleSurface = surface;

    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
    std::cout << "Got adapter: " << adapter << std::endl;

    std::cout << "Requesting device..." << std::endl;

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = nullptr;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = nullptr; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = nullptr;
    deviceDesc.defaultQueue.label = "The default queue";
    WGPUDevice device = requestDevice(adapter, &deviceDesc);

    std::cout << "Got device: " << device << std::endl;

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        gen_error("wgpu", "Uncaptured device error: type {}, ({})", type, message);
    };
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, nullptr /* pUserData */);

    WGPUQueue queue = wgpuDeviceGetQueue(device);

    // auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
    //     gen_info("wgpu", "Queued work finished with status: {}", status);
    // };
    // wgpuQueueOnSubmittedWorkDone(queue, onQueueWorkDone, nullptr /* pUserData */);


    WGPUTextureFormat swapChainFormat = wgpuSurfaceGetPreferredFormat(surface, adapter);
    WGPUSwapChainDescriptor swapChainDesc = {};
    swapChainDesc.nextInChain = nullptr;
    swapChainDesc.width = 640;
    swapChainDesc.height = 480;
    swapChainDesc.format = swapChainFormat;
    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
    swapChainDesc.presentMode = WGPUPresentMode_Fifo;

    WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
    std::cout << "Swapchain: " << swapChain << std::endl;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
        if (!nextTexture) {
            std::cerr << "Cannot acquire next swap chain texture" << std::endl;
            break;
        }

        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.nextInChain = nullptr;
        encoderDesc.label = "My command encoder";
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        WGPURenderPassDescriptor renderPassDesc = {};
        // [...] Describe Render Pass
        WGPURenderPassColorAttachment renderPassColorAttachment = {};
        // [...] Set up the attachment

        renderPassDesc.colorAttachmentCount = 1;
        renderPassDesc.colorAttachments = &renderPassColorAttachment;
        renderPassColorAttachment.view = nextTexture;
        renderPassColorAttachment.resolveTarget = nullptr;
        renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
        renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
        renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
        renderPassDesc.depthStencilAttachment = nullptr;
        renderPassDesc.timestampWriteCount = 0;
        renderPassDesc.timestampWrites = nullptr;
        renderPassDesc.nextInChain = nullptr;
        renderPassDesc.timestampWriteCount = 0;
		renderPassDesc.timestampWrites = nullptr;

        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
        wgpuRenderPassEncoderEnd(renderPass);

        WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
		cmdBufferDescriptor.nextInChain = nullptr;
		cmdBufferDescriptor.label = "Command buffer";
		WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
		wgpuQueueSubmit(queue, 1, &command);

        wgpuSwapChainPresent(swapChain);        
    }
}
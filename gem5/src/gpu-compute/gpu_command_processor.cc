/*
 * Copyright (c) 2018 Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "gpu-compute/gpu_command_processor.hh"

#include <cassert>

#include "arch/amdgpu/vega/pagetable_walker.hh"
#include "base/chunk_generator.hh"
#include "debug/GPUCommandProc.hh"
#include "debug/GPUKernelInfo.hh"
#include "dev/amdgpu/amdgpu_device.hh"
#include "gpu-compute/dispatcher.hh"
#include "mem/abstract_mem.hh"
#include "mem/packet_access.hh"
#include "mem/se_translating_port_proxy.hh"
#include "mem/translating_port_proxy.hh"
#include "params/GPUCommandProcessor.hh"
#include "sim/full_system.hh"
#include "sim/process.hh"
#include "sim/proxy_ptr.hh"
#include "sim/syscall_emul_buf.hh"

namespace gem5
{

GPUCommandProcessor::GPUCommandProcessor(const Params &p)
    : DmaVirtDevice(p), dispatcher(*p.dispatcher), _driver(nullptr),
      walker(p.walker), hsaPP(p.hsapp)
{
    assert(hsaPP);
    hsaPP->setDevice(this);
    dispatcher.setCommandProcessor(this);
}

HSAPacketProcessor&
GPUCommandProcessor::hsaPacketProc()
{
    return *hsaPP;
}

/**
 * Forward the VRAM requestor ID needed for device memory from GPU device.
 */
RequestorID
GPUCommandProcessor::vramRequestorId()
{
    return gpuDevice->vramRequestorId();
}

TranslationGenPtr
GPUCommandProcessor::translate(Addr vaddr, Addr size)
{
    if (!FullSystem) {
        // Grab the process and try to translate the virtual address with it;
        // with new extensions, it will likely be wrong to just arbitrarily
        // grab context zero.
        auto process = sys->threads[0]->getProcessPtr();

        return process->pTable->translateRange(vaddr, size);
    }

    // In full system use the page tables setup by the kernel driver rather
    // than the CPU page tables.
    return TranslationGenPtr(
        new AMDGPUVM::UserTranslationGen(&gpuDevice->getVM(), walker,
                                         1 /* vmid */, vaddr, size));
}

/**
 * submitDispatchPkt() is the entry point into the CP from the HSAPP
 * and is only meant to be used with AQL kernel dispatch packets.
 * After the HSAPP receives and extracts an AQL packet, it sends
 * it to the CP, which is responsible for gathering all relevant
 * information about a task, initializing CU state, and sending
 * it to the dispatcher for WG creation and dispatch.
 *
 * First we need capture all information from the the AQL pkt and
 * the code object, then store it in an HSAQueueEntry. Once the
 * packet and code are extracted, we extract information from the
 * queue descriptor that the CP needs to perform state initialization
 * on the CU. Finally we call dispatch() to send the task to the
 * dispatcher. When the task completely finishes, we call finishPkt()
 * on the HSA packet processor in order to remove the packet from the
 * queue, and notify the runtime that the task has completed.
 */
void
GPUCommandProcessor::submitDispatchPkt(void *raw_pkt, uint32_t queue_id,
                                       Addr host_pkt_addr)
{
    _hsa_dispatch_packet_t *disp_pkt = (_hsa_dispatch_packet_t*)raw_pkt;
    assert(!(disp_pkt->kernel_object & (system()->cacheLineSize() - 1)));

    /**
     * Need to use a raw pointer for DmaVirtDevice API. This is deleted
     * in the dispatchKernelObject method.
     */
    AMDKernelCode *akc = new AMDKernelCode;

    /**
     * The kernel_object is a pointer to the machine code, whose entry
     * point is an 'amd_kernel_code_t' type, which is included in the
     * kernel binary, and describes various aspects of the kernel. The
     * desired entry is the 'kernel_code_entry_byte_offset' field,
     * which provides the byte offset (positive or negative) from the
     * address of the amd_kernel_code_t to the start of the machine
     * instructions.
     *
     * For SE mode we can read from the port proxy. In FS mode, we may need
     * to wait for the guest OS to setup translations, especially when using
     * the KVM CPU, so it is preferred to read the code object using a timing
     * DMA request.
     */
    if (!FullSystem) {
        /**
         * we need to read a pointer in the application's address
         * space to pull out the kernel code descriptor.
         */
        auto *tc = sys->threads[0];
        SETranslatingPortProxy virt_proxy(tc);

        DPRINTF(GPUCommandProc, "reading kernel_object using proxy\n");
        virt_proxy.readBlob(disp_pkt->kernel_object, (uint8_t*)akc,
            sizeof(AMDKernelCode));

        dispatchKernelObject(akc, raw_pkt, queue_id, host_pkt_addr);
    } else {
        /**
         * In full system mode, the page table entry may point to a system
         * page or a device page. System pages use the proxy as normal, but
         * a device page needs to be read from device memory. Check what type
         * it is here.
         */
        bool is_system_page = true;
        Addr phys_addr = disp_pkt->kernel_object;

        /**
         * Full system currently only supports running on single VMID (one
         * virtual memory space), i.e., one application running on GPU at a
         * time. Because of this, for now we know the VMID is always 1. Later
         * the VMID would have to be passed on to the command processor.
         */
        int vmid = 1;
        unsigned tmp_bytes;
        walker->startFunctional(gpuDevice->getVM().getPageTableBase(vmid),
                                phys_addr, tmp_bytes, BaseMMU::Mode::Read,
                                is_system_page);

        DPRINTF(GPUCommandProc, "kernel_object vaddr %#lx paddr %#lx size %d"
                " s:%d\n", disp_pkt->kernel_object, phys_addr,
                sizeof(AMDKernelCode), is_system_page);

        /**
         * System objects use DMA device. Device objects need to use device
         * memory.
         */
        if (is_system_page) {
            DPRINTF(GPUCommandProc,
                    "sending system DMA read for kernel_object\n");

            auto dma_callback = new DmaVirtCallback<uint32_t>(
              [=](const uint32_t&) {
                dispatchKernelObject(akc, raw_pkt, queue_id, host_pkt_addr);
              });

            dmaReadVirt(disp_pkt->kernel_object, sizeof(AMDKernelCode),
                    dma_callback, (void *)akc);
        } else {
            DPRINTF(GPUCommandProc,
                    "kernel_object in device, using device mem\n");

            // Read from GPU memory manager one cache line at a time to prevent
            // rare cases where the AKC spans two memory pages.
            ChunkGenerator gen(disp_pkt->kernel_object, sizeof(AMDKernelCode),
                               system()->cacheLineSize());
            for (; !gen.done(); gen.next()) {
                Addr chunk_addr = gen.addr();
                int vmid = 1;
                unsigned dummy;
                walker->startFunctional(
                    gpuDevice->getVM().getPageTableBase(vmid), chunk_addr,
                    dummy, BaseMMU::Mode::Read, is_system_page);

                Request::Flags flags = Request::PHYSICAL;
                RequestPtr request = std::make_shared<Request>(chunk_addr,
                    system()->cacheLineSize(), flags,
                    walker->getDevRequestor());
                Packet *readPkt = new Packet(request, MemCmd::ReadReq);
                readPkt->dataStatic((uint8_t *)akc + gen.complete());
                system()->getDeviceMemory(readPkt)->access(readPkt);
                delete readPkt;
            }

            dispatchKernelObject(akc, raw_pkt, queue_id, host_pkt_addr);
        }
    }
}

void
GPUCommandProcessor::dispatchKernelObject(AMDKernelCode *akc, void *raw_pkt,
                                        uint32_t queue_id, Addr host_pkt_addr)
{
    _hsa_dispatch_packet_t *disp_pkt = (_hsa_dispatch_packet_t*)raw_pkt;

    DPRINTF(GPUCommandProc, "GPU machine code is %lli bytes from start of the "
        "kernel object\n", akc->kernel_code_entry_byte_offset);

    Addr machine_code_addr = (Addr)disp_pkt->kernel_object
        + akc->kernel_code_entry_byte_offset;

    DPRINTF(GPUCommandProc, "Machine code starts at addr: %#x\n",
        machine_code_addr);

    std::string kernel_name;

    /**
     * BLIT kernels don't have symbol names.  BLIT kernels are built-in compute
     * kernels issued by ROCm to handle DMAs for dGPUs when the SDMA
     * hardware engines are unavailable or explicitly disabled.  They can also
     * be used to do copies that ROCm things would be better performed
     * by the shader than the SDMA engines.  They are also sometimes used on
     * APUs to implement asynchronous memcopy operations from 2 pointers in
     * host memory.  I have no idea what BLIT stands for.
     * */
    if (akc->runtime_loader_kernel_symbol) {
        kernel_name = "Some kernel";
    } else {
        kernel_name = "Blit kernel";
    }

    DPRINTF(GPUKernelInfo, "Kernel name: %s\n", kernel_name.c_str());

    GfxVersion gfxVersion = FullSystem ? gpuDevice->getGfxVersion()
                          : driver()->getGfxVersion();
    HSAQueueEntry *task = new HSAQueueEntry(kernel_name, queue_id,
        dynamic_task_id, raw_pkt, akc, host_pkt_addr, machine_code_addr,
        gfxVersion);

    DPRINTF(GPUCommandProc, "Task ID: %i Got AQL: wg size (%dx%dx%d), "
        "grid size (%dx%dx%d) kernarg addr: %#x, completion "
        "signal addr:%#x\n", dynamic_task_id, disp_pkt->workgroup_size_x,
        disp_pkt->workgroup_size_y, disp_pkt->workgroup_size_z,
        disp_pkt->grid_size_x, disp_pkt->grid_size_y,
        disp_pkt->grid_size_z, disp_pkt->kernarg_address,
        disp_pkt->completion_signal);

    DPRINTF(GPUCommandProc, "Extracted code object: %s (num vector regs: %d, "
        "num scalar regs: %d, code addr: %#x, kernarg size: %d, "
        "LDS size: %d)\n", kernel_name, task->numVectorRegs(),
        task->numScalarRegs(), task->codeAddr(), 0, 0);

    initABI(task);
    ++dynamic_task_id;

    // The driver expects the start time to be in ns
    Tick start_ts = curTick() / sim_clock::as_int::ns;
    dispatchStartTime.insert({disp_pkt->completion_signal, start_ts});

    delete akc;
}

void
GPUCommandProcessor::sendCompletionSignal(Addr signal_handle)
{
    // Originally the completion signal was read functionally and written
    // with a timing DMA. This can cause issues in FullSystem mode and
    // cause translation failures. Therefore, in FullSystem mode everything
    // is done in timing mode.

    if (!FullSystem) {
        /**
        * HACK: The semantics of the HSA signal is to decrement
        * the current signal value. We cheat here and read out
        * he value from main memory using functional access and
        * then just DMA the decremented value.
        */
        uint64_t signal_value = functionalReadHsaSignal(signal_handle);

        updateHsaSignal(signal_handle, signal_value - 1);
    } else {
        // The semantics of the HSA signal is to decrement the current
        // signal value by one. Do this asynchronously via DMAs and
        // callbacks as we can safely continue with this function
        // while waiting for the next packet from the host.
        updateHsaSignalAsync(signal_handle, -1);
    }
}

void
GPUCommandProcessor::updateHsaSignalAsync(Addr signal_handle, int64_t diff)
{
    Addr mailbox_addr = getHsaSignalMailboxAddr(signal_handle);
    uint64_t *mailboxValue = new uint64_t;
    auto cb2 = new DmaVirtCallback<uint64_t>(
        [ = ] (const uint64_t &)
            { updateHsaMailboxData(signal_handle, mailboxValue); });
    dmaReadVirt(mailbox_addr, sizeof(uint64_t), cb2, (void *)mailboxValue);
    DPRINTF(GPUCommandProc, "updateHsaSignalAsync reading mailbox addr %lx\n",
            mailbox_addr);
}

void
GPUCommandProcessor::updateHsaMailboxData(Addr signal_handle,
                                          uint64_t *mailbox_value)
{
    Addr event_addr = getHsaSignalEventAddr(signal_handle);

    DPRINTF(GPUCommandProc, "updateHsaMailboxData read %ld\n", *mailbox_value);
    if (*mailbox_value != 0) {
        // This is an interruptible signal. Now, read the
        // event ID and directly communicate with the driver
        // about that event notification.
        auto cb = new DmaVirtCallback<uint64_t>(
            [ = ] (const uint64_t &)
                { updateHsaEventData(signal_handle, mailbox_value); });
        dmaReadVirt(event_addr, sizeof(uint64_t), cb, (void *)mailbox_value);
    } else {
        delete mailbox_value;

        Addr ts_addr = signal_handle + offsetof(amd_signal_t, start_ts);

        amd_event_t *event_ts = new amd_event_t;
        event_ts->start_ts = dispatchStartTime[signal_handle];
        event_ts->end_ts = curTick() / sim_clock::as_int::ns;
        auto cb = new DmaVirtCallback<uint64_t>(
            [ = ] (const uint64_t &)
                { updateHsaEventTs(signal_handle, event_ts); });
        dmaWriteVirt(ts_addr, sizeof(amd_event_t), cb, (void *)event_ts);
        DPRINTF(GPUCommandProc, "updateHsaMailboxData reading timestamp addr "
                "%lx\n", ts_addr);

        dispatchStartTime.erase(signal_handle);
    }
}

void
GPUCommandProcessor::updateHsaEventData(Addr signal_handle,
                                        uint64_t *event_value)
{
    Addr mailbox_addr = getHsaSignalMailboxAddr(signal_handle);

    DPRINTF(GPUCommandProc, "updateHsaEventData read %ld\n", *event_value);
    // Write *event_value to the mailbox to clear the event
    auto cb = new DmaVirtCallback<uint64_t>(
        [ = ] (const uint64_t &)
            { updateHsaSignalDone(event_value); }, *event_value);
    dmaWriteVirt(mailbox_addr, sizeof(uint64_t), cb, &cb->dmaBuffer, 0);

    Addr ts_addr = signal_handle + offsetof(amd_signal_t, start_ts);

    amd_event_t *event_ts = new amd_event_t;
    event_ts->start_ts = dispatchStartTime[signal_handle];
    event_ts->end_ts = curTick() / sim_clock::as_int::ns;
    auto cb2 = new DmaVirtCallback<uint64_t>(
        [ = ] (const uint64_t &)
            { updateHsaEventTs(signal_handle, event_ts); });
    dmaWriteVirt(ts_addr, sizeof(amd_event_t), cb2, (void *)event_ts);
    DPRINTF(GPUCommandProc, "updateHsaEventData reading timestamp addr %lx\n",
            ts_addr);

    dispatchStartTime.erase(signal_handle);
}

void
GPUCommandProcessor::updateHsaEventTs(Addr signal_handle,
                                      amd_event_t *ts)
{
    delete ts;

    Addr value_addr = getHsaSignalValueAddr(signal_handle);
    int64_t diff = -1;

    uint64_t *signalValue = new uint64_t;
    auto cb = new DmaVirtCallback<uint64_t>(
        [ = ] (const uint64_t &)
            { updateHsaSignalData(value_addr, diff, signalValue); });
    dmaReadVirt(value_addr, sizeof(uint64_t), cb, (void *)signalValue);
    DPRINTF(GPUCommandProc, "updateHsaSignalAsync reading value addr %lx\n",
            value_addr);
}

void
GPUCommandProcessor::updateHsaSignalData(Addr value_addr, int64_t diff,
                                         uint64_t *prev_value)
{
    // Reuse the value allocated for the read
    DPRINTF(GPUCommandProc, "updateHsaSignalData read %ld, writing %ld\n",
            *prev_value, *prev_value + diff);
    *prev_value += diff;
    auto cb = new DmaVirtCallback<uint64_t>(
        [ = ] (const uint64_t &)
            { updateHsaSignalDone(prev_value); });
    dmaWriteVirt(value_addr, sizeof(uint64_t), cb, (void *)prev_value);
}

void
GPUCommandProcessor::updateHsaSignalDone(uint64_t *signal_value)
{
    delete signal_value;
}

uint64_t
GPUCommandProcessor::functionalReadHsaSignal(Addr signal_handle)
{
    Addr value_addr = getHsaSignalValueAddr(signal_handle);
    auto tc = system()->threads[0];
    ConstVPtr<Addr> prev_value(value_addr, tc);
    return *prev_value;
}

void
GPUCommandProcessor::updateHsaSignal(Addr signal_handle, uint64_t signal_value,
                                     HsaSignalCallbackFunction function)
{
    // The signal value is aligned 8 bytes from
    // the actual handle in the runtime
    Addr value_addr = getHsaSignalValueAddr(signal_handle);
    Addr mailbox_addr = getHsaSignalMailboxAddr(signal_handle);
    Addr event_addr = getHsaSignalEventAddr(signal_handle);
    DPRINTF(GPUCommandProc, "Triggering completion signal: %x!\n", value_addr);

    auto cb = new DmaVirtCallback<uint64_t>(function, signal_value);

    dmaWriteVirt(value_addr, sizeof(Addr), cb, &cb->dmaBuffer, 0);

    auto tc = system()->threads[0];
    ConstVPtr<uint64_t> mailbox_ptr(mailbox_addr, tc);

    // Notifying an event with its mailbox pointer is
    // not supported in the current implementation. Just use
    // mailbox pointer to distinguish between interruptible
    // and default signal. Interruptible signal will have
    // a valid mailbox pointer.
    if (*mailbox_ptr != 0) {
        // This is an interruptible signal. Now, read the
        // event ID and directly communicate with the driver
        // about that event notification.
        ConstVPtr<uint32_t> event_val(event_addr, tc);

        DPRINTF(GPUCommandProc, "Calling signal wakeup event on "
                "signal event value %d\n", *event_val);

        // The mailbox/wakeup signal uses the SE mode proxy port to write
        // the event value. This is not available in full system mode so
        // instead we need to issue a DMA write to the address. The value of
        // *event_val clears the event.
        if (FullSystem) {
            auto cb = new DmaVirtCallback<uint64_t>(function, *event_val);
            dmaWriteVirt(mailbox_addr, sizeof(Addr), cb, &cb->dmaBuffer, 0);
        } else {
            signalWakeupEvent(*event_val);
        }
    }
}

void
GPUCommandProcessor::attachDriver(GPUComputeDriver *gpu_driver)
{
    fatal_if(_driver, "Should not overwrite driver.");
    // TODO: GPU Driver inheritance hierarchy doesn't really make sense.
    // Should get rid of the base class.
    _driver = gpu_driver;
    assert(_driver);
}

GPUComputeDriver*
GPUCommandProcessor::driver()
{
    return _driver;
}

/**
 * submitVendorPkt() is for accepting vendor-specific packets from
 * the HSAPP. Vendor-specific packets may be used by the runtime to
 * send commands to the HSA device that are specific to a particular
 * vendor. The vendor-specific packets should be defined by the vendor
 * in the runtime.
 */

/**
 * TODO: For now we simply tell the HSAPP to finish the packet and write a
 * completion signal, if any. However, in the future proper handing may be
 * required for vendor specific packets.
 *
 * In the version of ROCm that is currently supported the runtime will send
 * packets that direct the CP to invalidate the GPU caches. We do this
 * automatically on each kernel launch in the CU, so that situation is safe
 * for now.
 */
void
GPUCommandProcessor::submitVendorPkt(void *raw_pkt, uint32_t queue_id,
    Addr host_pkt_addr)
{
    auto vendor_pkt = (_hsa_generic_vendor_pkt *)raw_pkt;

    if (vendor_pkt->completion_signal) {
        sendCompletionSignal(vendor_pkt->completion_signal);
    }

    warn("Ignoring vendor packet\n");

    hsaPP->finishPkt(raw_pkt, queue_id);
}

/**
 * submitAgentDispatchPkt() is for accepting agent dispatch packets.
 * These packets will control the dispatch of Wg on the device, and inform
 * the host when a specified number of Wg have been executed on the device.
 *
 * For now it simply finishes the pkt.
 */
void
GPUCommandProcessor::submitAgentDispatchPkt(void *raw_pkt, uint32_t queue_id,
    Addr host_pkt_addr)
{
    //Parse the Packet, see what it wants us to do
    _hsa_agent_dispatch_packet_t * agent_pkt =
        (_hsa_agent_dispatch_packet_t *)raw_pkt;

    if (agent_pkt->type == AgentCmd::Nop) {
        DPRINTF(GPUCommandProc, "Agent Dispatch Packet NOP\n");
    } else if (agent_pkt->type == AgentCmd::Steal) {
        //This is where we steal the HSA Task's completion signal
        int kid = agent_pkt->arg[0];
        DPRINTF(GPUCommandProc,
            "Agent Dispatch Packet Stealing signal handle for kernel %d\n",
            kid);

        HSAQueueEntry *task = dispatcher.hsaTask(kid);
        uint64_t signal_addr = task->completionSignal();// + sizeof(uint64_t);

        uint64_t return_address = agent_pkt->return_address;
        DPRINTF(GPUCommandProc, "Return Addr: %p\n",return_address);
        //*return_address = signal_addr;
        Addr *new_signal_addr = new Addr;
        *new_signal_addr  = (Addr)signal_addr;
        dmaWriteVirt(return_address, sizeof(Addr), nullptr, new_signal_addr, 0);

        DPRINTF(GPUCommandProc,
            "Agent Dispatch Packet Stealing signal handle from kid %d :" \
            "(%x:%x) writing into %x\n",
            kid,signal_addr,new_signal_addr,return_address);

    } else
    {
        panic("The agent dispatch packet provided an unknown argument in" \
        "arg[0],currently only 0(nop) or 1(return kernel signal) is accepted");
    }

    hsaPP->finishPkt(raw_pkt, queue_id);
}

/**
 * Once the CP has finished extracting all relevant information about
 * a task and has initialized the ABI state, we send a description of
 * the task to the dispatcher. The dispatcher will create and dispatch
 * WGs to the CUs.
 */
void
GPUCommandProcessor::dispatchPkt(HSAQueueEntry *task)
{
    dispatcher.dispatch(task);
}

void
GPUCommandProcessor::signalWakeupEvent(uint32_t event_id)
{
    _driver->signalWakeupEvent(event_id);
}

/**
 * The CP is responsible for traversing all HSA-ABI-related data
 * structures from memory and initializing the ABI state.
 * Information provided by the MQD, AQL packet, and code object
 * metadata will be used to initialze register file state.
 */
void
GPUCommandProcessor::initABI(HSAQueueEntry *task)
{
    auto cb = new DmaVirtCallback<uint32_t>(
        [ = ] (const uint32_t &readDispIdOffset)
            { ReadDispIdOffsetDmaEvent(task, readDispIdOffset); }, 0);

    Addr hostReadIdxPtr
        = hsaPP->getQueueDesc(task->queueId())->hostReadIndexPtr;

    dmaReadVirt(hostReadIdxPtr + sizeof(hostReadIdxPtr),
        sizeof(uint32_t), cb, &cb->dmaBuffer);
}

System*
GPUCommandProcessor::system()
{
    return sys;
}

AddrRangeList
GPUCommandProcessor::getAddrRanges() const
{
    AddrRangeList ranges;
    return ranges;
}

void
GPUCommandProcessor::setGPUDevice(AMDGPUDevice *gpu_device)
{
    gpuDevice = gpu_device;
    walker->setDevRequestor(gpuDevice->vramRequestorId());
}

void
GPUCommandProcessor::setShader(Shader *shader)
{
    _shader = shader;
}

Shader*
GPUCommandProcessor::shader()
{
    return _shader;
}

} // namespace gem5

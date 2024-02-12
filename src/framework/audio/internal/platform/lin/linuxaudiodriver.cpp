/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore BVBA and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "framework/audio/midiqueue.h"
#include "linuxaudiodriver.h"
#include "../alsa/alsaaudiodriver.h" //FIX: relative path, set path in CMakeLists
#if JACK_AUDIO
#include "../jack/jackaudiodriver.h" //FIX: relative path, set path in CMakeLists
#endif

#include "translation.h"
#include "log.h"
#include "runtime.h"

using namespace mu::audio;

LinuxAudioDriver::LinuxAudioDriver()
{
}

LinuxAudioDriver::~LinuxAudioDriver()
{
}

void LinuxAudioDriver::init()
{
    LOGI(" -- init, this: %lx", this);
    m_devicesListener.startWithCallback([this]() {
        return availableOutputDevices();
    });

    m_devicesListener.devicesChanged().onNotify(this, [this]() {
        m_availableOutputDevicesChanged.notify();
    });
}

std::string LinuxAudioDriver::name() const
{
    return m_current_audioDriverState->name();
}

bool LinuxAudioDriver::open(const Spec& spec, Spec* activeSpec)
{
    if (!m_current_audioDriverState->open(spec, activeSpec)) {
        return false;
    }
    m_spec = *activeSpec;
    return true;
}

void LinuxAudioDriver::close()
{
    return m_current_audioDriverState->close();
}

bool LinuxAudioDriver::isOpened() const
{
    return m_current_audioDriverState->isOpened();
}

AudioDeviceID LinuxAudioDriver::outputDevice() const
{
    if (m_current_audioDriverState != nullptr) {
        return m_current_audioDriverState->m_deviceId;
    } else {
        LOGE("device is not opened");
        return m_deviceId; // FIX: should return optional type
    }
}

bool LinuxAudioDriver::makeDevice(const AudioDeviceID& deviceId)
{
#if defined(JACK_AUDIO)
    if (deviceId == "jack") {
        m_current_audioDriverState = std::make_unique<JackDriverState>();
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    } else if (deviceId == "alsa") {
        m_current_audioDriverState = std::make_unique<AlsaDriverState>();
#endif
#else
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    if (deviceId == "alsa") {
        m_current_audioDriverState = std::make_unique<AlsaDriverState>();
#endif
#endif
    } else {
        LOGE() << "Unknown device name: " << deviceId;
        return false;
    }
    LOGI(" -- driver: %lx", m_current_audioDriverState.get());
    return true;
}

// reopens the same device (if m_spec has changed)
bool LinuxAudioDriver::reopen(const AudioDeviceID& deviceId, Spec newSpec)
{
    bool reopen = m_current_audioDriverState->isOpened();
    // close current device if opened
    if (reopen) {
        m_current_audioDriverState->close();
    }
    // maybe change device, if needed
    if (m_current_audioDriverState->name() != deviceId) {
        // select the new device driver
        if (!makeDevice(deviceId)) {
            return false;
        }
    }
    // open the device driver
    if (!m_current_audioDriverState->open(newSpec, &newSpec)) {
        return false;
    }
    return true;
}

bool LinuxAudioDriver::selectOutputDevice(const AudioDeviceID& deviceId)
{
    // When starting, no previously device has been selected
    if (m_current_audioDriverState == nullptr) {
        LOGW() << "no previously opened device";
        return makeDevice(deviceId);
    }
    // If for some reason we select the same device, do nothing
    if (m_current_audioDriverState->name() == deviceId) {
        return true;
    }
    reopen(deviceId, m_spec);
    m_outputDeviceChanged.notify();
    return true;
}

bool LinuxAudioDriver::resetToDefaultOutputDevice()
{
#if defined(JACK_AUDIO)
    return selectOutputDevice("jack"); // FIX:
#else
    return selectOutputDevice("alsa"); // FIX:
#endif
}

mu::async::Notification LinuxAudioDriver::outputDeviceChanged() const
{
    return m_outputDeviceChanged;
}

AudioDeviceList LinuxAudioDriver::availableOutputDevices() const
{
    AudioDeviceList devices;
#if defined(JACK_AUDIO)
    devices.push_back({ "jack", trc("audio", "JACK") });
#endif
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
    devices.push_back({ "alsa", trc("audio", "ALSA") });
#endif
    return devices;
}

mu::async::Notification LinuxAudioDriver::availableOutputDevicesChanged() const
{
    return m_availableOutputDevicesChanged;
}

unsigned int LinuxAudioDriver::outputDeviceBufferSize() const
{
    return m_current_audioDriverState->m_spec.samples;
}

bool LinuxAudioDriver::setOutputDeviceBufferSize(unsigned int bufferSize)
{
    if (m_spec.samples == (int)bufferSize) {
        return true;
    }
    m_spec.samples = (int)bufferSize;
    if (!reopen(m_current_audioDriverState->name(), m_spec)) {
        return false;
    }
    m_bufferSizeChanged.notify();
    return true;
}

mu::async::Notification LinuxAudioDriver::outputDeviceBufferSizeChanged() const
{
    return m_bufferSizeChanged;
}

std::vector<unsigned int> LinuxAudioDriver::availableOutputDeviceBufferSizes() const
{
    std::vector<unsigned int> result;

    unsigned int n = 4096; // FIX: magic number
    while (n >= MINIMUM_BUFFER_SIZE) {
        result.push_back(n);
        n /= 2;
    }

    std::sort(result.begin(), result.end());

    return result;
}

#if 0
std::shared_ptr<ThreadSafeQueue<const Event>&> LinuxAudioDriver::getMidiQueue() const
{
    LOGI(" -- getAudioDriverState() m_current_audioDriverState=%lx", m_current_audioDriverState.get());
    if (m_current_audioDriverState) {
        LOGI(" -- driverHandle: %lx", m_current_audioDriverState->getAudioDriverQueue());
        return m_current_audioDriverState->getAudioDriverQueue();
    } else {
        LOGW("Cant get audiodriver-handle, driver not open");
        return nullptr;
    }
}
#endif
 
bool LinuxAudioDriver::pushMidiEvent(mu::midi::Event& e) {
    if (m_current_audioDriverState) {
        LOGI(" -- push midi-event to audiodriver");
        m_current_audioDriverState->pushMidiEvent(e);
        return true;
    }
    return false;
}

void LinuxAudioDriver::resume()
{
}

void LinuxAudioDriver::suspend()
{
}

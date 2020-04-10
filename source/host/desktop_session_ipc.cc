//
// Aspia Project
// Copyright (C) 2020 Dmitry Chapyshev <dmitry@aspia.ru>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//

#include "host/desktop_session_ipc.h"

#include "base/logging.h"
#include "codec/video_util.h"
#include "desktop/mouse_cursor.h"
#include "desktop/shared_memory_desktop_frame.h"
#include "ipc/shared_memory.h"

namespace host {

class DesktopSessionIpc::SharedBuffer : public ipc::SharedMemoryBase
{
public:
    ~SharedBuffer() = default;

    static std::unique_ptr<SharedBuffer> wrap(std::unique_ptr<ipc::SharedMemory> shared_memory)
    {
        std::shared_ptr<ipc::SharedMemory> shared_frame(shared_memory.release());
        return std::unique_ptr<SharedBuffer>(new SharedBuffer(shared_frame));
    }

    std::unique_ptr<SharedBuffer> share()
    {
        return std::unique_ptr<SharedBuffer>(new SharedBuffer(shared_memory_));
    }

    void* data() override
    {
        return shared_memory_->data();
    }

    Handle handle() const override
    {
        return shared_memory_->handle();
    }

    int id() const override
    {
        return shared_memory_->id();
    }

private:
    explicit SharedBuffer(std::shared_ptr<ipc::SharedMemory>& shared_memory)
        : shared_memory_(shared_memory)
    {
        // Nothing
    }

    std::shared_ptr<ipc::SharedMemory> shared_memory_;

    DISALLOW_COPY_AND_ASSIGN(SharedBuffer);
};

DesktopSessionIpc::DesktopSessionIpc(std::unique_ptr<ipc::Channel> channel, Delegate* delegate)
    : channel_(std::move(channel)),
      delegate_(delegate)
{
    DCHECK(channel_);
    DCHECK(delegate_);
}

DesktopSessionIpc::~DesktopSessionIpc() = default;

void DesktopSessionIpc::start()
{
    if (!delegate_)
        return;

    channel_->setListener(this);
    channel_->resume();

    delegate_->onDesktopSessionStarted();
}

void DesktopSessionIpc::stop()
{
    delegate_ = nullptr;
}

void DesktopSessionIpc::setEnabled(bool enable)
{
    outgoing_message_.Clear();
    outgoing_message_.mutable_set_enabled()->set_enable(enable);
    channel_->send(base::serialize(outgoing_message_));
}

void DesktopSessionIpc::setConfig(const Config& config)
{
    outgoing_message_.Clear();

    proto::internal::SetConfig* set_config = outgoing_message_.mutable_set_config();
    set_config->set_disable_font_smoothing(config.disable_font_smoothing);
    set_config->set_disable_wallpaper(config.disable_wallpaper);
    set_config->set_disable_effects(config.disable_effects);
    set_config->set_block_input(config.block_input);

    channel_->send(base::serialize(outgoing_message_));
}

void DesktopSessionIpc::selectScreen(const proto::Screen& screen)
{
    outgoing_message_.Clear();
    outgoing_message_.mutable_select_source()->mutable_screen()->CopyFrom(screen);
    channel_->send(base::serialize(outgoing_message_));
}

void DesktopSessionIpc::injectKeyEvent(const proto::KeyEvent& event)
{
    outgoing_message_.Clear();
    outgoing_message_.mutable_key_event()->CopyFrom(event);
    channel_->send(base::serialize(outgoing_message_));
}

void DesktopSessionIpc::injectPointerEvent(const proto::PointerEvent& event)
{
    outgoing_message_.Clear();
    outgoing_message_.mutable_pointer_event()->CopyFrom(event);
    channel_->send(base::serialize(outgoing_message_));
}

void DesktopSessionIpc::injectClipboardEvent(const proto::ClipboardEvent& event)
{
    outgoing_message_.Clear();
    outgoing_message_.mutable_clipboard_event()->CopyFrom(event);
    channel_->send(base::serialize(outgoing_message_));
}

void DesktopSessionIpc::userSessionControl(proto::internal::UserSessionControl::Action action)
{
    outgoing_message_.Clear();
    outgoing_message_.mutable_user_session_control()->set_action(action);
    channel_->send(base::serialize(outgoing_message_));
}

void DesktopSessionIpc::onDisconnected()
{
    if (delegate_)
        delegate_->onDesktopSessionStopped();
}

void DesktopSessionIpc::onMessageReceived(const base::ByteArray& buffer)
{
    incoming_message_.Clear();

    if (!base::parse(buffer, &incoming_message_))
    {
        LOG(LS_ERROR) << "Invalid message from desktop";
        return;
    }

    if (incoming_message_.has_encode_frame())
    {
        onEncodeFrame(incoming_message_.encode_frame());
    }
    else if (incoming_message_.has_screen_list())
    {
        if (delegate_)
            delegate_->onScreenListChanged(incoming_message_.screen_list());
    }
    else if (incoming_message_.has_shared_buffer())
    {
        switch (incoming_message_.shared_buffer().type())
        {
            case proto::internal::SharedBuffer::CREATE:
                onCreateSharedBuffer(incoming_message_.shared_buffer().shared_buffer_id());
                break;

            case proto::internal::SharedBuffer::RELEASE:
                onReleaseSharedBuffer(incoming_message_.shared_buffer().shared_buffer_id());
                break;

            default:
                NOTREACHED();
                break;
        }
    }
    else if (incoming_message_.has_clipboard_event())
    {
        if (delegate_)
            delegate_->onClipboardEvent(incoming_message_.clipboard_event());
    }
    else
    {
        LOG(LS_ERROR) << "Unhandled message from desktop";
        return;
    }
}

void DesktopSessionIpc::onEncodeFrame(const proto::internal::EncodeFrame& encode_frame)
{
    if (encode_frame.has_frame())
    {
        const proto::internal::SerializedDesktopFrame& serialized_frame = encode_frame.frame();

        std::unique_ptr<SharedBuffer> shared_buffer = sharedBuffer(serialized_frame.shared_buffer_id());
        if (!shared_buffer)
            return;

        std::unique_ptr<desktop::Frame> frame = desktop::SharedMemoryFrame::attach(
            desktop::Size(serialized_frame.width(), serialized_frame.height()),
            codec::parsePixelFormat(serialized_frame.pixel_format()),
            std::move(shared_buffer));

        desktop::Region* updated_region = frame->updatedRegion();

        for (int i = 0; i < serialized_frame.dirty_rect_size(); ++i)
            updated_region->addRect(codec::parseRect(serialized_frame.dirty_rect(i)));

        if (delegate_)
            delegate_->onScreenCaptured(*frame);
    }

    if (encode_frame.has_mouse_cursor() && delegate_)
    {
        const proto::internal::SerializedMouseCursor& mouse_cursor =
            encode_frame.mouse_cursor();

        delegate_->onCursorCaptured(desktop::MouseCursor(
            base::fromStdString(mouse_cursor.data()),
            desktop::Size(mouse_cursor.width(), mouse_cursor.height()),
            desktop::Point(mouse_cursor.hotspot_x(), mouse_cursor.hotspot_y())));
    }

    outgoing_message_.Clear();
    outgoing_message_.mutable_encode_frame_result()->set_dummy(1);
    channel_->send(base::serialize(outgoing_message_));
}

void DesktopSessionIpc::onCreateSharedBuffer(int shared_buffer_id)
{
    std::unique_ptr<ipc::SharedMemory> shared_memory =
        ipc::SharedMemory::open(ipc::SharedMemory::Mode::READ_ONLY, shared_buffer_id);

    if (!shared_memory)
    {
        LOG(LS_ERROR) << "Failed to create the shared buffer " << shared_buffer_id;
        return;
    }

    shared_buffers_.emplace(shared_buffer_id, SharedBuffer::wrap(std::move(shared_memory)));
}

void DesktopSessionIpc::onReleaseSharedBuffer(int shared_buffer_id)
{
    shared_buffers_.erase(shared_buffer_id);
}

std::unique_ptr<DesktopSessionIpc::SharedBuffer> DesktopSessionIpc::sharedBuffer(
    int shared_buffer_id)
{
    auto result = shared_buffers_.find(shared_buffer_id);
    if (result == shared_buffers_.end())
    {
        LOG(LS_ERROR) << "Failed to find the shared buffer " << shared_buffer_id;
        return nullptr;
    }

    return result->second->share();
}

} // namespace host

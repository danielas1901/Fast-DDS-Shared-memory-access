struct BufferNode
    {
        struct Status
        {
            // When buffers are enqueued in a port this validity_id is copied to the BufferDescriptor in the port.
            // If the sender process needs to recover the buffer by force, just increment this validity_id, the
            // receiver process must check this validity_id vs the validity_id in the BufferDescriptor, if not equal
            // the buffer has been invalidated by the sender.
            uint64_t validity_id : 24;
            // This counter is incremented each time the buffer is enqueued in a port, an decremented when pop
            // from the port to be processed
            uint64_t enqueued_count : 20;
            // When listener processes start processing this buffer increments the processing_count. This way
            // the sender can know whether the buffer is been processed or is just only enqueued in some ports.
            uint64_t processing_count : 20;
        };

        std::atomic<Status> status;
        uint32_t data_size;
        eprosima::fastdds::rtps::SharedMemSegment::Offset data_offset;

        /**
         * Atomically invalidates a buffer.
         */
        inline void invalidate_buffer()
        {   
            std::cout << "@SharedMemManager::Buffer::Status::invalidate_buffer" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            while (!status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id + 1, (uint64_t)0u, (uint64_t)0u },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }
        }

        /**
         * Atomically invalidates a buffer, only, when the buffer is valid for the caller.
         * @return true when succeeded, false when the buffer was invalid for the caller.
         */
        inline bool invalidate_buffer(
                uint32_t listener_validity_id)
        {
            std::cout << "@SharedMemManager::Buffer::Status::invalidate_buffer with param" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id + 1, (uint64_t)0u, (uint64_t)0u },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        /**
         * Atomically invalidates a buffer, only, if the buffer is not being processed.
         * @return true when succeeded, false otherwise.
         */
        bool invalidate_if_not_processing()
        {
            std::cout << "@SharedMemManager::Buffer::Status::invalidate_if_not_processing" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            // If the buffer is not beeing processed by any listener => is invalidated
            while (s.processing_count == 0 &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id + 1, (uint64_t)0u, (uint64_t)0u},
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }
            printf("Buffer is being invalidated, segment_size may be insufficient");
            return (s.processing_count == 0);
        }

        /**
         * @return true if listener_validity_id == current buffer validity_id.
         */
        inline bool is_valid(
                uint32_t listener_validity_id) const
        {
            std::cout << "@SharedMemManager::Buffer::Status::is_valid" << std::endl;
            return (status.load(std::memory_order_relaxed).validity_id == listener_validity_id);
        }

        /**
         * Atomically decrease enqueued count & increase the buffer processing counts, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool dec_enqueued_inc_processing_counts(
                uint32_t listener_validity_id)
        {
            std::cout << "@SharedMemManager::Buffer::Status::dec_enqueued_inc_processing_counts" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count - 1, (uint64_t)s.processing_count + 1 },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        /**
         * Atomically increase the buffer processing count, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool inc_processing_count(
                uint32_t listener_validity_id)
        {
            std::cout << "@SharedMemManager::Buffer::Status::inc_processing_count" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count, (uint64_t)s.processing_count + 1 },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        /**
         * Atomically increase the buffer enqueued count, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool inc_enqueued_count(
                uint32_t listener_validity_id)
        {
            std::cout << "@SharedMemManager::Buffer::Status::inc_enqueued_count" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count + 1, (uint64_t)s.processing_count },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        /**
         * Atomically decrease the buffer enqueued count, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool dec_enqueued_count(
                uint32_t listener_validity_id)
        {
            std::cout << "@SharedMemManager::Buffer::Status::dec_enqueued_count" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count - 1, (uint64_t)s.processing_count },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

        inline bool is_not_referenced() const
        {
            std::cout << "@SharedMemManager::Buffer::Status::is_not_referenced" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            return (s.enqueued_count == 0) && (s.processing_count == 0);
        }

        /**
         * Atomically decrease the buffer processing count, only, if the buffer is valid.
         * @return true when succeeded, false when the buffer has been invalidated.
         */
        inline bool dec_processing_count(
                uint32_t listener_validity_id)
        {
            std::cout << "@SharedMemManager::Buffer::Status::dec_processing_count" << std::endl;
            auto s = status.load(std::memory_order_relaxed);
            while (listener_validity_id == s.validity_id &&
                    !status.compare_exchange_weak(s,
                    { (uint64_t)s.validity_id, (uint64_t)s.enqueued_count, (uint64_t)s.processing_count - 1 },
                    std::memory_order_release,
                    std::memory_order_relaxed))
            {
            }

            return (listener_validity_id == s.validity_id);
        }

    };
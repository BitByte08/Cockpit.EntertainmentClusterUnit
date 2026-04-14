#ifndef CAN_FRAME_STUB_HPP
#define CAN_FRAME_STUB_HPP

// Windows용 can_frame 스텁 — SocketCAN이 없는 환경에서 컴파일만 가능하게 함
#include <cstdint>

struct can_frame {
    uint32_t can_id;
    uint8_t  can_dlc;
    uint8_t  data[8];
};

#endif // CAN_FRAME_STUB_HPP

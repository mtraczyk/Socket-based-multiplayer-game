#include "client.h"

void Client::set_parameters(const int argc, char *argv[]) {
    if (argc > 1) {
        server_name = argv[1];
        for (;;) {
            switch (getopt(argc, argv, "n:p:i:r:")) {
                case 'n':
                    if (!validate_player_name(optarg)) {
                        throw std::string("invalid player name");
                    }
                    name = optarg;
                    break;

                case 'p':
                    server_port = optarg;
                    validate_parameter(optarg, 1, 65535);
                    break;

                case 'i':
                    gui_server = optarg;
                    break;

                case 'r':
                    validate_parameter(optarg, 1, 65535);
                    gui_port = optarg;
                    break;

                case -1:
                    if (argc > optind + 1) {
                        throw std::string("invalid parameter");
                    }
                    return;

                default:
                    throw std::string("invalid parameter");
            }
        }
    }
    throw std::string("no game server name");
}

int64_t Client::validate_parameter(const std::string &param, const int64_t lo, const int64_t hi) {
    if (param.size() == 1 && !isdigit(param[0])) {
        throw std::string("invalid parameter");
    }
    int64_t res = 0;
    int64_t sign = 1;
    for (size_t i = 0; i < param.size(); i++) {
        if (isdigit(param[i])) {
            res *= 10;
            res += param[i] - '0';
        } else if (i == 0 && param[i] == '-') {
            sign = -1;
        } else {
            throw std::string("invalid parameter");
        }
    }
    res *= sign;
    if (res >= lo && res <= hi) {
        return res;
    }

    throw std::string("parameter out of range");
}

void Client::test_parameters() {
    std::cerr << "name " << name << std::endl;
    std::cerr << "server_name " << server_name << std::endl;
    std::cerr << "server_port " << server_port << std::endl;
    std::cerr << "gui_server " << gui_server << std::endl;
    std::cerr << "gui_port " << gui_port << std::endl;
}

void Client::run() {
    set_timer_spec();
    launch_timer();
    open_server_sock();
    connect_to_gui();
    while (true) {
        receive_from_gui();
        send_to_game();
        receive_from_game();
    }
}

void Client::open_server_sock() {
    struct addrinfo addr_hints;
    struct addrinfo *addr_res;

    memset(&addr_hints, 0, sizeof(addr_hints));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = nullptr;
    addr_hints.ai_canonname = nullptr;
    addr_hints.ai_next = nullptr;
    addr_hints.ai_socktype = SOCK_DGRAM;
    addr_hints.ai_protocol = IPPROTO_UDP;

    if (getaddrinfo(server_name.c_str(), server_port.c_str(), &addr_hints, &addr_res) != 0)
        throw std::string("getting server address failure");

    server_sock = socket(addr_res->ai_family, addr_res->ai_socktype, addr_res->ai_protocol);
    if (server_sock < 0)
        throw std::string("socket failure");

    /** Make socket nonblocking. */
    struct timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    if (setsockopt(server_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) != 0)
        throw std::string("set socket nonblocking failure");

    if (connect(server_sock, addr_res->ai_addr, addr_res->ai_addrlen) < 0)
        throw std::string("server connect error");

    freeaddrinfo(addr_res);
}

void Client::connect_to_gui() {
    addrinfo addr_hints{};
    addrinfo *addr_result;

    memset(&addr_hints, 0, sizeof(addrinfo));
    addr_hints.ai_family = AF_UNSPEC;
    addr_hints.ai_flags = 0;
    addr_hints.ai_addrlen = 0;
    addr_hints.ai_addr = nullptr;
    addr_hints.ai_canonname = nullptr;
    addr_hints.ai_next = nullptr;
    addr_hints.ai_socktype = SOCK_STREAM;
    addr_hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(gui_server.c_str(), gui_port.c_str(), &addr_hints, &addr_result) != 0)
        throw std::string("get gui addr error");

    gui_sock = socket(addr_result->ai_family, addr_result->ai_socktype, addr_result->ai_protocol);
    if (gui_sock < 0)
        throw std::string("gui sock error");

    struct timeval tv{};
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    if (setsockopt(gui_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        throw std::string("set nonblock gui");

    int nagle = 1;
    if (setsockopt(gui_sock, IPPROTO_TCP, TCP_NODELAY, &nagle, sizeof(nagle)) < 0)
        throw std::string("nagle error");


    if (connect(gui_sock, addr_result->ai_addr, addr_result->ai_addrlen) < 0)
        throw std::string("gui connect error");

    freeaddrinfo(addr_result);
}

bool Client::validate_player_name(const std::string &player_name) {
    if (player_name.size() > PLAYER_NAME_MAX_LENGTH) {
        return false;
    }
    for (const auto &c: player_name) {
        if (c < 33 || c > 126) {
            return false;
        }
    }
    return true;
}

void Client::receive_from_gui() {
    char buff[70000];
    memset(buff, 0, sizeof(buff));
    int32_t rcv_len = read(gui_sock, buff, sizeof(buff));
    if (rcv_len > 0) {
        parse_gui_msg(buff);
    } else if ((rcv_len == 0)
               || (rcv_len == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        throw std::string("gui read error");
    }
}

void Client::parse_gui_msg(const std::string &msg) {
    static const int8_t del = 10;
    for (const auto &c: msg) {
        if (c == del) {
            move(gui_comm);
            gui_comm.clear();
        } else {
            gui_comm += c;
        }
    }
}

void Client::move(const std::string &comm) {
    if (comm == "LEFT_KEY_DOWN") {
        last_turn = LEFT;
    } else if (comm == "LEFT_KEY_UP") {
        if (last_turn == LEFT) {
            last_turn = STRAIGHT;
        }
    } else if (comm == "RIGHT_KEY_DOWN") {
        last_turn = RIGHT;
    } else if (comm == "RIGHT_KEY_UP") {
        if (last_turn == RIGHT) {
            last_turn = STRAIGHT;
        }
    }
}

void Client::send_to_game() {
    int32_t rv = poll(&timer, 1, 0);
    if (rv && timer.revents & POLLIN) {
        uint64_t buff;
        if (read(timer.fd, &buff, sizeof(buff)) < 0)
            throw std::string("timer error");
        timer.revents = 0;
        client_server_msg msg;
        memset(&msg, 0, sizeof(msg));
        msg.session_id = htobe64(session_id);
        msg.next_expected_event_no = htobe32(next_expected_event_no);
        msg.turn_direction = last_turn;
        for (size_t i = 0; i < name.size(); i++) {
            msg.player_name[i] = name[i];
        }
        ssize_t size = CLIENT_SERVER_MSG_MIN_LEN + name.size();
        if (write(server_sock, &msg, size) == -1)
            throw std::string("write to server sock fail");

    } else if (rv) {
        throw std::string("timer error");
    }
}

void Client::receive_from_game() {
    uint8_t buff[550];
    ssize_t rcv_len = read(server_sock, buff, sizeof(buff));
    if (rcv_len > MAX_UDP_SIZE) {
        throw std::string("Received more than MAX_UPD_MSG_SIZE.");
    } else if (rcv_len > 0) {
        size_t offset = 0;
        uint32_t game_id_ = get_dword(buff, offset);
        bool rv = true;
        while (offset < (size_t) rcv_len && rv) {
            rv = process_event(buff, offset, game_id_);
        }
    } else if (rcv_len == 0
               || (rcv_len == -1 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        throw std::string("read from game error");
    }
}

void Client::set_timer_spec() {
    timer_spec.it_value.tv_sec = 0;
    timer_spec.it_value.tv_nsec = 30000000;
    timer_spec.it_interval.tv_sec = 0;
    timer_spec.it_interval.tv_nsec = 30000000;
}

void Client::launch_timer() {
    timer.revents = 0;
    timerfd_settime(timer.fd, 0, &timer_spec, NULL);
}

uint32_t Client::get_dword(uint8_t *buff, size_t &offset) {
    uint32_t dword;
    memcpy(&dword, buff + offset, DWORD);
    offset += DWORD;
    return be32toh(dword);
}

uint64_t Client::get_qword(uint8_t *buff, size_t &offset) {
    uint64_t qword;
    memcpy(&qword, buff + offset, QWORD);
    offset += QWORD;
    return be64toh(qword);
}

uint8_t Client::get_byte(uint8_t *buff, size_t &offset) {
    return buff[offset++];
}

bool Client::process_event(uint8_t *buff, size_t &offset, uint32_t game_id_) {
    uint32_t len = get_dword(buff, offset);
    if (!check_crc(buff, offset - DWORD, len + DWORD)) {
        return false;
    }
    uint32_t ev_no = get_dword(buff, offset);
    len -= DWORD;
    uint8_t ev_type = get_byte(buff, offset);
    len--;
    if (game_id_ != game_id && ev_type == NEW_GAME) {
        game_id = game_id_;
        next_expected_event_no = NEW_GAME;
    } else if (game_id_ != game_id && ev_type != NEW_GAME) {
        return false;
    }

    if (ev_no != next_expected_event_no) {
        return false;
    }

    if (ev_type == NEW_GAME) {
        process_new_game_event(buff, offset, len);
    } else if (ev_type == PIXEL) {
        process_pixel_event(buff, offset, len);
    } else if (ev_type == PLAYER_ELIMINATED) {
        process_player_eliminated_event(buff, offset, len);
    } else if (ev_type > GAME_OVER) {
        offset += len + DWORD;
        return true;
    }

    next_expected_event_no++;
    offset += DWORD;
    return true;

}

void Client::process_new_game_event(uint8_t *buff, size_t &offset, uint32_t len) {
    size_t poz = offset;
    width = get_dword(buff, offset);
    height = get_dword(buff, offset);
    get_new_players(buff, offset, len - (DWORD + DWORD));
    if (offset - poz != len) {
        throw std::string("Invalid new game event!");
    }
    send_new_game_event();
}

void Client::process_pixel_event(uint8_t *buff, size_t &offset, uint32_t len) {
    size_t poz = offset;
    uint8_t player_num = get_byte(buff, offset);
    uint32_t x = get_dword(buff, offset);
    uint32_t y = get_dword(buff, offset);
    if (player_num >= player_name.size()
        || x >= width
        || y >= height
        || offset - poz != len) {
        throw std::string("Invalid pixel event!");
    }
    send_pixel_event(player_num, x, y);
}

void Client::process_player_eliminated_event(uint8_t *buff, size_t &offset, uint32_t len) {
    size_t poz = offset;
    uint8_t player = get_byte(buff, offset);
    if (player >= player_name.size() || offset - poz != len) {
        throw std::string("Invalid eliminate event!");
    }
    send_player_eliminated_event(player);
}

bool Client::check_crc(uint8_t *buff, size_t offset, uint32_t len) {
    static const uint32_t crc32_tab[] = {
            0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
            0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
            0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
            0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
            0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
            0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
            0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
            0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
            0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
            0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
            0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
            0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
            0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
            0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
            0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
            0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
            0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
            0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
            0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
            0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
            0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
            0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
            0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
            0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
            0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
            0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
            0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
            0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
            0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
            0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
            0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
            0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
            0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
            0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
            0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
            0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
            0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
            0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
            0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
            0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
            0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
            0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
            0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };
    uint32_t crc32 = 0xFFFFFFFF;
    for (size_t i = 0; i < len; i++) {
        uint32_t index = (crc32 ^ buff[offset++]) & 0xFF;
        crc32 = (crc32 >> BYTE) ^ crc32_tab[index];
    }
    crc32 = crc32 ^ 0xFFFFFFFF;
    uint32_t send_crc;
    memcpy(&send_crc, buff + offset, 4);
    send_crc = be32toh(send_crc);

    return crc32 == send_crc;
}

void Client::get_new_players(uint8_t *buff, size_t &offset, uint32_t len) {
    player_name.clear();
    std::string new_name = "";
    for (size_t i = 0; i < len; i++) {
        if (buff[offset] == 0
            && new_name.size() > 0
            && new_name.size() <= 20) {
            player_name.push_back(new_name);
            new_name.clear();
        } else if (buff[offset] >= 33 && buff[offset] <= 126) {
            new_name.push_back(buff[offset]);
        } else {
            throw std::string("Invalid player name.");
        }
        offset++;
    }
    if (new_name.size() > 0) {
        throw std::string("Invalid player name.");
    }
}

void Client::send_to_gui(std::string &msg) {
    auto *buff = reinterpret_cast<uint8_t *>(&msg[0]);
    if (write(gui_sock, buff, msg.size()) == -1)
        throw std::string("gui write fail");
}

void Client::send_new_game_event() {
    static const char del = 10;
    std::string msg{"NEW_GAME"};
    msg += " " + std::to_string(width) + " " + std::to_string(height);
    for (const auto &p: player_name) {
        msg += " " + p;
    }
    msg += del;
    send_to_gui(msg);
}

void Client::send_pixel_event(const uint8_t n, const uint32_t x, const uint32_t y) {
    static const char del = 10;
    std::string msg{"PIXEL"};
    msg += " " + std::to_string(x) + " "
           + std::to_string(y) + " " + player_name[n] + del;
    send_to_gui(msg);
}

void Client::send_player_eliminated_event(const uint8_t n) {
    static const char del = 10;
    std::string msg{"PLAYER_ELIMINATED"};
    msg += " " + player_name[n] + del;
    send_to_gui(msg);
}
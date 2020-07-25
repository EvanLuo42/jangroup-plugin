#include <iostream>
#include <set>
#include <sstream>
#include <fstream> 
#include <iterator>
#include <string>
#include <regex>

#include <cqcppsdk/cqcppsdk.h>
#include <nlohmann/json.hpp>
#include <curl/curl.h>

using namespace cq;
using namespace std;
using Message = cq::message::Message;
using MessageSegment = cq::message::MessageSegment;

static const set<int64_t> ENABLED_GROUPS = {738324937};

std::string get_content(const std::string &url) {
    std::string body;

    auto curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        auto receive = [](char *buf, size_t size, size_t count, void *data) {
            (*static_cast<std::string *>(data)) += std::string(buf, count);
            return size * count;
        };
        typedef size_t (*WriteFunction)(char *, size_t, size_t, void *);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, static_cast<WriteFunction>(receive));
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &body);

        curl_easy_perform(curl);

        curl_easy_cleanup(curl);
    }

    return body;
}


CQ_INIT {
    on_enable([] {
        logging::info("启用", "插件已启用"); 
        static vector<std::string> dirtyWords;
        ifstream file(dir::root()+"DirtyWords.txt");
        while (file) {
            std::string line;
            std::getline(file, line);
            dirtyWords.push_back(line);
        }
    });

    on_group_message([](const GroupMessageEvent &event) {
        if (ENABLED_GROUPS.count(event.group_id) == 0) return;
        if (event.message == "菜单") {
            send_group_message(event.group_id, "[CQ:at,qq=" + to_string(event.user_id) + "]\n---菜单---\n1.签到");
        }
        if (event.message == "签到") {
            send_group_message(event.group_id, "[CQ:at,qq=" + to_string(event.user_id) + "]\n签到成功AWA\n元气+1");
        }   
        if (event.message == "天气") {
            regex weather("[a - zA - Z]");
            smatch result;
            std::string message = event.message;
            if (regex_match(message, result, weather)) {
                try {
                    string request=get_content("http://api.k780.com/?app=weather.today&weaid=" + to_string(result[0])
                                        + "&appkey=48814&sign=76e8d37b8eecb72b6debf702a496a677&format=json");
                    nlohmann::json urlResult = request;
                    send_group_message(event.group_id, "[CQ:at,qq=" + to_string(event.user_id) +"]\n日期:" + to_string(urlResult["days"])+ "\n气温:" + to_string(urlResult["temperature_curr"])+ "\n湿度:" + to_string(urlResult["humidity"]));
                        
                } catch (ApiError &) {
                    send_group_message(event.group_id, "[CQ:at,qq=" + to_string(event.user_id) + "]\n天气查询失败");
                }
            } else {
                send_group_message(event.group_id, "格式错误!正确格式:天气 <城市全拼>");
            }
        }
        event.block();
    });

    on_group_upload([](const auto &event) {
        if (ENABLED_GROUPS.count(event.group_id) == 0) return;
        stringstream ss;
        ss << "有人上传了" << event.file.name << ", 大小: " << event.file.size << "Bytes";
        try {
            send_group_message(event.group_id, ss.str());
        } catch (ApiError &) {
        }
    });

    on_group_member_decrease([](const GroupMemberDecreaseEvent &event) {
        if (ENABLED_GROUPS.count(event.group_id) == 0) return;
        try {
            send_group_message(event.group_id, "有一名群成员退出了群聊!");
        } catch (ApiError &) {
        }
    });

    on_group_member_increase([](const GroupMemberIncreaseEvent &event) {
        if (ENABLED_GROUPS.count(event.group_id) == 0) return;
        try {
            send_group_message(event.group_id, "有一名群成员加入了群聊!");
        } catch (ApiError &) {
        }
    });

    on_private_message([](const PrivateMessageEvent &event) { 
        if (event.message == "hi") {
            try {
                send_private_message(event.user_id, "你好");
            } catch (ApiError &) {
            }
        }
    });

    on_group_message([](const GroupMessageEvent &event) {
        if (ENABLED_GROUPS.count(event.group_id) == 0) return;
        if (event.message == "hi") {
        }
    });
}

CQ_MENU(menu_demo_1) {
    logging::info("菜单", "为京都动画聚友社定制的插件。");
}

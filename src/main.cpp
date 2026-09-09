#include <cstdlib>
#include <iostream>
#include <string>

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    if (argc < 3 || std::string(argv[1]) != "-p") {
        std::cerr << "Expected first argument to be '-p'" << std::endl;
        return 1;
    }

    std::string prompt = argv[2];

    if (prompt.empty()) {
        std::cerr << "Prompt must not be empty" << std::endl;
        return 1;
    }

    const char* api_key_env = std::getenv("OPENROUTER_API_KEY");
    const char* base_url_env = std::getenv("OPENROUTER_BASE_URL");

    std::string api_key = api_key_env ? api_key_env : "";
    std::string base_url = base_url_env ? base_url_env : "https://openrouter.ai/api/v1";

    if (api_key.empty()) {
        std::cerr << "OPENROUTER_API_KEY is not set" << std::endl;
        return 1;
    }

    json request_body = {
        {"model", "anthropic/claude-haiku-4.5"},
        {"messages", json::array({
            {{"role", "user"}, {"content", prompt}}
        })},
        {"tools", json::array({
        {
            {"type", "function"},
            {"function", {
                {"name", "Read"},
                {"description", "Read & return the contents of a file"},
                {"parameters", {
                    {"type", "object"},
                    {"properties", {
                        {"file_path", {
                            {"type", "string"},
                            {"description", "The path to the file to read"}
                        }}
                    }},
                    {"required", json::array({
                        "file_path"
                    })}
                }}
            }}
        }
    })}
    };

    cpr::Response response = cpr::Post(
        cpr::Url{base_url + "/chat/completions"},
        cpr::Header{
            {"Authorization", "Bearer " + api_key},
            {"Content-Type", "application/json"}
        },
        cpr::Body{request_body.dump()}
    );

    if (response.status_code != 200) {
        std::cerr << "HTTP error: " << response.status_code << std::endl;
        return 1;
    }

    json result = json::parse(response.text);

    if (!result.contains("choices") || result["choices"].empty()) {
        std::cerr << "No choices in response" << std::endl;
        return 1;
    }
    json message = result["choices"][0]["message"];
    if(message.contains("tool_calls") && !message["tool_calls"].empty())
    {
        json tool_call = message["tool_calls"][0];
        json function = tool_call["function"];

        std::string function_name = function["name"].get<std::string>();

        if(function_name == "Read")
        {
            std:: string arguments_string = function["arguments"].get<std::string>();

            json arguments = json::parse(arguments_string);

            std::string file_path = arguments["file_path"].get<std::string>();

            std::ifstream file(file_path);

            if(!file.is_open())
            {
                std::cerr << "Failed to open file: " << file_path << std::endl;
                return 1;
            }

            std::string contents(
                (std::isstreambuf_iterator<char>(file)),
                std::isstreambuf_iterator<char>()
            );

            std::cout << contents;
        }
        else
        {
            std::cerr << "Unknown tool: " << function_name << std::endl;
            return 1;
        }
    }
    else
    {
        if(!message["content"].is_null())
        {
            std::cout << message["content"].get<std::string>();
        }
    }

    std::cerr << "Logs from your program will appear here!" << std::endl;
    
    return 0;
}

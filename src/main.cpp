#include "sdk.h"

#include <boost/program_options.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <chrono>
#include <optional>

#include "log.h"
#include "json_loader.h"
#include "request_handler.h"
#include "logging_request_handler.h"
#include "ticker.h"

using namespace std::literals;
namespace net = boost::asio;
namespace sys = boost::system;
namespace po = boost::program_options;
namespace fs = std::filesystem;

namespace {

// Запускает функцию fn на n потоках, включая текущий
template <typename Fn>
void RunWorkers(unsigned n, const Fn& fn) {
    n = std::max(1u, n);
    std::vector<std::jthread> workers;
    workers.reserve(n - 1);
    // Запускаем n-1 рабочих потоков, выполняющих функцию fn
    while (--n) {
        workers.emplace_back(fn);
    }
    fn();
}

}  // namespace

struct Args {
    std::optional<int> tick_period;
    std::string config_file;
    std::string www_root;
    bool randomize_spawn_points;
    std::string state_file;
    int save_state_period = 0;
};

[[nodiscard]] std::optional<Args> ParseCommandLine(int argc, const char* const argv[]) {
    po::options_description desc{"All options"s};
    Args args;
    desc.add_options()
        ("help,h", "Show help")
        ("tick-period,t", po::value<int>(), "Tick period of server update")
        ("config-file,c", po::value(&args.config_file), "Config file path")
        ("www-root,w", po::value(&args.www_root), "Static www files path")
        ("randomize-spawn-points", "Spawn dogs in random map point")
        ("state-file", po::value(&args.state_file), "Set state file path")
        ("save-state-period", po::value<int>(&args.save_state_period), "Set save state period");
    
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.contains("help"s)) {
        return std::nullopt;
    }

    if (!vm.contains("config-file"s)) {
        throw std::runtime_error("ERROR:Config file must be specified."s);
    }

    if (!vm.contains("www-root"s)) {
        throw std::runtime_error("ERROR:Source root must be specified."s);
    }

    if (vm.count("tick-period"s)) {
        args.tick_period = vm["tick-period"].as<int>();
    } else {
        args.tick_period = std::nullopt;
    }

    if (vm.count("randomize-spawn-points"s)) {
        args.randomize_spawn_points = true;
    } else {
        args.randomize_spawn_points = false;
    }
    
    // если не задан файл для сохранения
    if (!vm.contains("state-file")) {
        args.state_file = std::string();
        args.save_state_period = 0;
    }
    
    return args;
};

int main(int argc, const char* argv[]) {
    Args args;
    
    try {
        if (auto args_optional = ParseCommandLine(argc, argv)) {
            args = args_optional.value();
        } else {
            return EXIT_SUCCESS;
        }
    } catch (const std::exception& e) {
        std::cout << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
    try {
        InitBoostLogFilter();
        
        model::Game game = json_loader::LoadGame(args.config_file);
        
        game.SetSaveFile(args.state_file);
        
        if (args.state_file.size() != 0 && fs::exists(fs::path(args.state_file))) {
            try {
                game.LoadState();
            } catch (std::exception& e) {
                return EXIT_FAILURE;
            }
            std::cout << "trying to load state" << std::endl;
        }
        
        game.SetRandomizeSpawnPoint(args.randomize_spawn_points);
        game.SetTickPeriod(args.tick_period);
        
        // загрузка БД
        const unsigned num_threads = std::thread::hardware_concurrency();
        
        const char* db_url = std::getenv("GAME_DB_URL");
        if (!db_url) {
            throw std::runtime_error("GAME_DB_URL is not specified");
        }
        auto connection_pool = std::make_shared<ConnectionPool>(std::max(1u, num_threads), [db_url] {
            return std::make_shared<pqxx::connection>(db_url);
        });
        try{
            postgres::DB::Init(connection_pool);
            game.SetPool(connection_pool);
        } catch (std::exception& e) {
            return EXIT_FAILURE;
        }
        
        // 2. Инициализируем io_context
        net::io_context ioc(num_threads);

        // 3. Добавляем асинхронный обработчик сигналов SIGINT и SIGTERM
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const sys::error_code& ec, [[maybe_unused]] int signal_number) {
            if (!ec) {
                
                boost::json::value log_json = {{"code", 0}};
                BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json) << "server exited";
                
                ioc.stop();
            }
        });
        
        auto api_strand = net::make_strand(ioc);

        // Сервер автоматически обновляет время
        if (args.tick_period) {
            auto ticker = std::make_shared<Ticker>(
                api_strand, std::chrono::milliseconds(args.tick_period.value()), [&game](std::chrono::milliseconds delta) {
                game.UpdateTickState(delta);
            }
            );
            ticker->Start();
        }
        
        // Если нужна сериализация во время работы программы
        if (!args.state_file.empty() && args.save_state_period != 0) {
            
            // нужна автоматическая сериализация
            if (args.tick_period) {
                
                auto ticker = std::make_shared<Ticker>(
                    api_strand, std::chrono::milliseconds(args.save_state_period), [&game](std::chrono::milliseconds delta) {
                    game.UpdateTickState(delta);
                }
                );
                ticker->Start();
                
                game.SetManualSerialization(false);
                
            } else {
                game.SetManualSerialization(true);
            }

        }
        
        // 4. Создаём обработчик HTTP-запросов и связываем его с моделью игры
        auto handler_tmp = std::make_shared<http_handler::RequestHandler>(game, argv[2], api_strand);
        LoggingRequestHandler<http_handler::RequestHandler> handler{*handler_tmp};
        
        // 5. Запустить обработчик HTTP-запросов, делегируя их обработчику запросов
        const auto address = net::ip::make_address("0.0.0.0");
        constexpr net::ip::port_type port = 8080;
        
        http_server::ServeHttp(ioc, {address, port}, [&handler](auto&& req, const auto& remote_endpoint, auto&& send) {
            handler(std::forward<decltype(req)>(req), remote_endpoint, std::forward<decltype(send)>(send));
        });
        
        boost::json::value log_json = {{"port", 8080}, {"address", "0.0.0.0"}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json) << "server started";

        // 6. Запускаем обработку асинхронных операций
        RunWorkers(std::max(1u, num_threads), [&ioc] {
            ioc.run();
        });
        
        if (args.state_file.size() != 0) {
            try {
                game.SaveState();
            } catch (const std::exception& e) {
                std::cout << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }
        
    } catch (const std::exception& ex) {
        
        boost::json::value log_json = {{"code", EXIT_FAILURE}, {"exception", ex.what()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, log_json) << "server exited";
        
        return EXIT_FAILURE;
    }
}

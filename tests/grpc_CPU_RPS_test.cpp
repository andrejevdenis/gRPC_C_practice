#include <gtest/gtest.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <grpcpp/grpcpp.h>

#include "fake_user_server.h"  // Подключаем ООП-сервер
#include "user_grpc_client.h"  // Подключаем ООП-клиент
#include "service.pb.h"       
#include "service.grpc.pb.h"  

TEST(GrpcUserOopTest, PerformanceBenchmark) {
    // 1. Инициализируем компоненты одной строчкой
    FakeUserServer server;
    UserGrpcClient client(server.GetInProcessChannel());

    // 2. Достаем потокобезопасный stub из нашего ООП-клиента
    std::shared_ptr<UserService::Stub> client_stub = client.GetStub();

    const int TOTAL_ITERATIONS = 10000;
    const int NUM_THREADS = 12;          // Все 12 потоков вашего Xeon
    const int REQS_PER_THREAD = TOTAL_ITERATIONS / NUM_THREADS;

    // Засекаем высокоточное время СТАРТА
    auto start_time = std::chrono::high_resolution_clock::now();

    // Создаем вектор для управления потоками
    std::vector<std::thread> threads;

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([client_stub, REQS_PER_THREAD]() {
            UserRequest request;
            request.set_name("Tom");
            UserResponse response;

            for (int i = 0; i < REQS_PER_THREAD; ++i) {
                grpc::ClientContext context; // Свой контекст на каждый отдельный вызов
                client_stub->GetUserProfile(&context, request, &response);
            }
            });
    }

    // Ожидаем, пока все потоки завершат свою работу
    for (auto& thread : threads) {
        thread.join();
    }

    // Засекаем время ОКОНЧАНИЯ
    auto end_time = std::chrono::high_resolution_clock::now();

    // Считаем прошедшие миллисекунды
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    // Вычисляем RPS
    double seconds = duration / 1000.0;
    double rps = TOTAL_ITERATIONS / seconds;

    std::cout << "\n==========================================" << std::endl;
    std::cout << "[BENCHMARK " << NUM_THREADS << " THREADS] TOTAL TIME FOR " << TOTAL_ITERATIONS << " REQS: " << duration << " ms" << std::endl;
    std::cout << "[BENCHMARK " << NUM_THREADS << " THREADS] CALCULATED gRPC RPS: " << rps << " req/sec" << std::endl;
    std::cout << "==========================================\n" << std::endl;

    // ВСЯ ОЧИСТКА ТЕПЕРЬ ПРОИСХОДИТ АВТОМАТИЧЕСКИ!
    // client_stub уничтожится здесь.
    // server автоматически вызовет свой деструктор, остановит gRPC и очистит память.
}
